/*
 * File: z-file.c
 * Purpose: Low-level file (and directory) handling
 *
 * Copyright (c) 1997-2007 Ben Harrison, pelpel, Andrew Sidwell
 *
 * This work is free software; you can redistribute it and/or modify it
 * under the terms of either:
 *
 * a) the GNU General Public License as published by the Free Software
 *    Foundation, version 2, or
 *
 * b) the "Angband licence":
 *    This software may be copied and distributed for educational, research,
 *    and not for profit purposes provided that this copyright and statement
 *    are included in all such copies.  Other copyrights may also apply.
 */
#ifdef NDS
# include <fat.h>
# include <unistd.h>
# include <reent.h>
# include <sys/iosupport.h>
# include <errno.h>
#endif

#include "angband.h"
#include "z-file.h"

#ifndef RISCOS
#ifdef _WIN32_WCE
#else
# include <sys/types.h>
# include <sys/stat.h>
#endif
#endif

#ifdef WINDOWS
# include <windows.h>
#ifdef _WIN32_WCE
#else
# include <io.h>
#endif
#endif

/*
 * Drop permissions
 */
void safe_setuid_drop(void)
{
#ifdef SET_UID
# if defined(HAVE_SETRESGID)

	if (setresgid(-1, getgid(), -1) != 0)
		quit("setegid(): cannot drop permissions correctly!");

# else

	if (setegid(getgid()) != 0)
		quit("setegid(): cannot drop permissions correctly!");

# endif
#endif /* SET_UID */
}


/*
 * Grab permissions
 */
void safe_setuid_grab(void)
{
#ifdef SET_UID
# if defined(HAVE_SETRESGID)

	if (setresgid(-1, player_egid, -1) != 0)
		quit("setegid(): cannot grab permissions correctly!");

# elif defined(HAVE_SETEGID)

	if (setegid(player_egid) != 0)
		quit("setegid(): cannot grab permissions correctly!");

# endif
#endif /* SET_UID */
}



/*
 * The concept of the file routines is that all file handling should be done
 * using as few routines as possible, since every machine is slightly
 * different, but these routines always have the same semantics.
 *
 * Prhaps we should use "path_parse()" to convert from "canonical" filenames
 * (optional leading tildes, internal wildcards, slash as the path seperator,
 * etc) to "system" filenames (no special symbols, system-specific path
 * seperator, etc).  This would allow the program itself to assume that all
 * filenames are "Unix" filenames, and explicitly "extract" such filenames if
 * needed (by "path_parse()", or perhaps "path_canon()"). XXX
 *
 * path_temp() should probably return a "canonical" filename.  XXX
 *
 * Note that "my_fopen()" and "my_open()" and "my_make()" and "my_kill()"
 * and "my_move()" and "my_copy()" should all take "canonical" filenames.
 *
 * Canonical filenames use a leading slash to indicate an absolute path, and a
 * leading tilde to indicate a special directory.  They default to a relative
 * path.  DOS/Windows uses a leading "drivename plus colon" to indicate the
 * use of a "special drive", and then the rest of the path is parsed normally,
 * and MACINTOSH uses a leading colon to indicate a relative path, and an
 * embedded colon to indicate a "drive plus absolute path", and finally
 * defaults to a file in the current working directory, which may or may
 * not be defined.
 */


#ifdef HAVE_MKSTEMP

FILE *my_fopen_temp(char *buf, size_t max)
{
	int fd;

	/* Prepare the buffer for mkstemp */
	my_strcpy(buf, "/tmp/anXXXXXX", max);

	/* Secure creation of a temporary file */
	fd = mkstemp(buf);

	/* Check the file-descriptor */
	if (fd < 0) return (NULL);

	/* Return a file stream */
	return (fdopen(fd, "w"));
}

#else /* HAVE_MKSTEMP */

/*
 * Consider rewriting this so it uses its own buffer.
 */
FILE *my_fopen_temp(char *buf, size_t max)
{
#ifdef _WIN32_WCE
  TCHAR wcBuf[1024]; 
  TCHAR wcTmp[1024];
  TCHAR wcPref[1024];
  mbstowcs(wcPref, "ang", 1024);
  /* Get the temp path */
  if (GetTempPath(max, wcTmp) > max) return (NULL);
  /* Generate a temporary filename */
  if (GetTempFileName(wcTmp, wcPref, 0, wcBuf) == 0) return (NULL);
  wcstombs(buf, wcBuf, 1024);
  /* Open the file */
  return (my_fopen(buf, "w"));
#else
	const char *s;

	/* Temp file */
	s = tmpnam(NULL);

	/* Oops */
	if (!s) return (NULL);

	/* Copy to buffer */
	my_strcpy(buf, s, max);

	/* Open the file */
	return (my_fopen(buf, "w"));
#endif /* WCE */
}

#endif /* HAVE_MKSTEMP */


/*
 * Hack -- replacement for "fgets()"
 *
 * Read a string, without a newline, to a file
 *
 * Process tabs, strip internal non-printables
 */
#define TAB_COLUMNS_1   8

errr my_fgets(FILE *fff, char *buf, size_t n)
{
	u16b i = 0;
	char *s = buf;
	int len;
	bool check_encodes = FALSE;


	/* Paranoia */
	if (n <= 0) return (1);

	/* Enforce historical upper bound */
	if (n > 1024) n = 1024;

	/* Leave a byte for terminating null */
	len = n - 1;

	/* While there's room left in the buffer */
	while (i < len)
	{
		int c;

		/*
		 * Read next character - stdio buffers I/O, so there's no
		 * need to buffer it again using fgets.
		 */
		c = fgetc(fff);

		/* End of file */
		if (c == EOF)
		{
			/* No characters read -- signal error */
			if (i == 0) break;

			/*
			 * Be nice to DOS/Windows, where a last line of a file isn't
			 * always \n terminated.
			 */
			*s = '\0';

			/* Translate encodes if necessary */
			if (check_encodes) xstr_trans(buf, 0);

			/* Success */
			return (0);
		}

#ifdef MACH_O_CARBON

		/*
		 * Be nice to the Macintosh, where a file can have Mac or Unix
		 * end of line, especially since the introduction of OS X.
		 * MPW tools were also very tolerant to the Unix EOL.
		 */
		if (c == '\r') c = '\n';

#endif /* MACH_O_CARBON */

		/* End of line */
		if (c == '\n')
		{
			/* Null terminate */
			*s = '\0';

			/* Translate encodes if necessary */
			if (check_encodes) xstr_trans(buf, 0);

			/* Success */
			return (0);
		}

		/* Expand a tab into spaces */
		if (c == '\t')
		{
			int tabstop;

			/* Next tab stop */
			tabstop = ((i + TAB_COLUMNS_1) / TAB_COLUMNS_1) * TAB_COLUMNS_1;

			/* Bounds check */
			if (tabstop >= len) break;

			/* Convert it to spaces */
			while (i < tabstop)
			{
				/* Store space */
				*s++ = ' ';

				/* Count */
				i++;
			}
		}

		/* Ignore non-printables */
		else if (my_isprint((unsigned char)c))
		{
			/* Store character in the buffer */
			*s++ = c;

			/* Count number of characters in the buffer */
			i++;

 			/* Notice possible encode */
 			if (c == '[') check_encodes = TRUE;
		}
	}

	/* Buffer overflow or EOF - return an empty string */
	buf[0] = '\0';

	/* Error */
	return (1);
}


/*
 * Hack -- replacement for "fputs()"
 *
 * Dump a string, plus a newline, to a file
 *
 * Perhaps this function should handle internal weirdness.
 */
errr my_fputs(FILE *fff, cptr buf, size_t n)
{
	/* Unused paramter */
	(void)n;

	/* Dump, ignore errors */
	(void)fprintf(fff, "%s\n", buf);

	/* Success */
	return (0);
}


/*
 * Check to see if a file exists, by opening it read-only.
 *
 * Return TRUE if it does, FALSE if it does not.
 */
bool my_fexists(const char *fname)
{
	int fd;

	/* Try to open it */
	fd = fd_open(fname, O_RDONLY);

	/* It worked */
	if (fd >= 0)
	{
		fd_close(fd);
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}



/* The file routines for RISC OS are in main-ros.c. */
#ifndef RISCOS


#ifdef SET_UID

/*
 * Find a default user name from the system.
 */
void user_name(char *buf, size_t len, int id)
{
	struct passwd *pw;

	/* Look up the user name */
	if ((pw = getpwuid(id)))
	{
		/* Get the first 15 characters of the user name */
		my_strcpy(buf, pw->pw_name, len);

		/* Capitalize the user name */
		buf[0] = toupper((unsigned char)buf[0]);

		return;
	}

	/* Oops.  Hack -- default to "PLAYER" */
	my_strcpy(buf, "PLAYER", len);
}

#endif /* SET_UID */


#if defined(SET_UID) || defined(USE_PRIVATE_PATHS)

/*
 * Extract a "parsed" path from an initial filename
 * Normally, we simply copy the filename into the buffer
 * But leading tilde symbols must be handled in a special way
 * Replace "~user/" by the home directory of the user named "user"
 * Replace "~/" by the home directory of the current user
 */
errr path_parse(char *buf, size_t max, cptr file)
{
	cptr u, s;
	struct passwd	*pw;
	char user[128];


	/* Assume no result */
	buf[0] = '\0';

	/* No file? */
	if (!file) return (-1);

	/* File needs no parsing */
	if (file[0] != '~')
	{
		my_strcpy(buf, file, max);
		return (0);
	}

	/* Point at the user */
	u = file+1;

	/* Look for non-user portion of the file */
	s = strstr(u, PATH_SEP);

	/* Hack -- no long user names */
	if (s && (s >= u + sizeof(user))) return (1);

	/* Extract a user name */
	if (s)
	{
		int i;
		for (i = 0; u < s; ++i) user[i] = *u++;
		user[i] = '\0';
		u = user;
	}

	/* Look up the "current" user */
	if (u[0] == '\0') u = getlogin();

	/* Look up a user (or "current" user) */
	if (u) pw = getpwnam(u);
	else pw = getpwuid(getuid());

	/* Nothing found? */
	if (!pw) return (1);

	/* Make use of the info */
	my_strcpy(buf, pw->pw_dir, max);

	/* Append the rest of the filename, if any */
	if (s) my_strcat(buf, s, max);

	/* Success */
	return (0);
}


#else /* SET_UID */


/*
 * Extract a "parsed" path from an initial filename
 *
 * This requires no special processing on simple machines,
 * except for verifying the size of the filename.
 */
errr path_parse(char *buf, size_t max, cptr file)
{
	/* Accept the filename */
	my_strcpy(buf, file, max);

# ifdef MACH_O_CARBON

	/* Fix it according to the current operating system */
	convert_pathname(buf);

# endif

	/* Success */
	return (0);
}


#endif /* SET_UID */




/*
 * Create a new path by appending a file (or directory) to a path
 *
 * This requires no special processing on simple machines, except
 * for verifying the size of the filename, but note the ability to
 * bypass the given "path" with certain special file-names.
 *
 * Note that the "file" may actually be a "sub-path", including
 * a path and a file.
 *
 * Note that this function yields a path which must be "parsed"
 * using the "parse" function above.
 */
errr path_build(char *buf, size_t max, cptr path, cptr file)
{
	/* Special file */
	if (file[0] == '~')
	{
		/* Use the file itself */
		my_strcpy(buf, file, max);
	}

	/* Absolute file, on "normal" systems */
	else if (prefix(file, PATH_SEP) && !streq(PATH_SEP, ""))
	{
		/* Use the file itself */
		my_strcpy(buf, file, max);
	}

	/* No path given */
	else if (!path[0])
	{
		/* Use the file itself */
		my_strcpy(buf, file, max);
	}

	/* Path and File */
	else
	{
		/* Build the new path */
		strnfmt(buf, max, "%s%s%s", path, PATH_SEP, file);
	}

	/* Success */
	return (0);
}


/*
 * Hack -- replacement for "fopen()"
 */
FILE *my_fopen(cptr file, cptr mode)
{
	char buf[1024];
	FILE *fff;

	/* Hack -- Try to parse the path */
	if (path_parse(buf, sizeof(buf), file)) return (NULL);

	/* Attempt to fopen the file anyway */
	fff = fopen(buf, mode);

#if defined(MACH_O_CARBON)

	/* Set file creator and type */
	if (fff && strchr(mode, 'w')) fsetfileinfo(buf, _fcreator, _ftype);

#endif

	/* Return open file or NULL */
	return (fff);
}


/*
 * Hack -- replacement for "fclose()"
 */
errr my_fclose(FILE *fff)
{
	/* Require a file */
	if (!fff) return (-1);

	/* Close, check for error */
	if (fclose(fff) == EOF) return (1);

	/* Success */
	return (0);
}


/*
 * Hack -- attempt to delete a file
 */
errr fd_kill(cptr file)
{
	char buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, sizeof(buf), file)) return (-1);

#ifdef _WIN32_WCE
  {
    TCHAR wcBuf[1024];
    mbstowcs( wcBuf, buf, 1024);
    
    DeleteFile(wcBuf);
  }

  /* Assume success XXX XXX XXX */
  return (0);
  
#else

	/* Remove, return 0 on success, non-zero on failure */
	return (remove(buf));

#endif
}


/*
 * Hack -- attempt to move a file
 */
errr fd_move(cptr file, cptr what)
{
	char buf[1024];
	char aux[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, sizeof(buf), file)) return (-1);

	/* Hack -- Try to parse the path */
	if (path_parse(aux, sizeof(aux), what)) return (-1);

#ifdef _WIN32_WCE
  {
    TCHAR wcBuf[1024];
    TCHAR wcAux[1024];
    mbstowcs( wcBuf, buf, 1024);
    mbstowcs( wcAux, aux, 1024);
    
    MoveFile(wcBuf, wcAux);
  }
  
  /* Assume success XXX XXX XXX */
  return (0);

#else
  /* Rename, return 0 on success, non-zero on failure */
  return (rename(buf, aux));
#endif
}


/*
 * Hack -- attempt to open a file descriptor (create file)
 *
 * This function should fail if the file already exists
 *
 * Note that we assume that the file should be "binary"
 */
int fd_make(cptr file, int mode)
{
	char buf[1024];
	int fd;

	/* Hack -- Try to parse the path */
	if (path_parse(buf, sizeof(buf), file)) return (-1);

#ifdef _WIN32_WCE
  {
    TCHAR wcBuf[1024];
    mbstowcs( wcBuf, buf, 1024);
    
    return (CreateFile(wcBuf, GENERIC_WRITE, 0, NULL, CREATE_NEW, 
		       FILE_ATTRIBUTE_NORMAL, 0));
  }
#else

	/* Create the file, fail if exists, write-only, binary */
	fd = open(buf, O_CREAT | O_EXCL | O_WRONLY | O_BINARY, mode);

#ifdef MACH_O_CARBON

	/* Set file creator and type */
	if (fd >= 0) fsetfileinfo(buf, _fcreator, _ftype);

#endif

	/* Return descriptor */
	return (fd);
#endif /* WCE */
}


/*
 * Hack -- attempt to open a file descriptor (existing file)
 *
 * Note that we assume that the file should be "binary"
 */
int fd_open(cptr file, int flags)
{
	char buf[1024];

	/* Hack -- Try to parse the path */
	if (path_parse(buf, sizeof(buf), file)) return (-1);

#ifdef _WIN32_WCE

  {
    TCHAR wcBuf[1024];
    mbstowcs( wcBuf, buf, 1024);
    
    return (CreateFile(wcBuf, GENERIC_READ | GENERIC_WRITE, 
		       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, 
		       OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0));
  }
#else

	/* Attempt to open the file */
	return (open(buf, flags | O_BINARY, 0));
#endif /* WCE */
}


/*
 * Attempt to lock a file descriptor
 *
 * Legal lock types -- F_UNLCK, F_RDLCK, F_WRLCK
 */
errr fd_lock(int fd, int what)
{
#if defined(HAVE_FCNTL_H) && defined(SET_UID)

	struct flock lock;

	/* Verify the fd */
#ifdef _WIN32_WCE
	if (fd == -1) return (-1);
#else
	if (fd < 0) return (-1);
#endif

	lock.l_type = what;
	lock.l_start = 0; /* Lock the entire file */
	lock.l_whence = SEEK_SET; /* Lock the entire file */
	lock.l_len = 0; /* Lock the entire file */

	/* Wait for access and set lock status */
	/*
	 * Change F_SETLKW to F_SETLK if it's preferable to return
	 * without locking and reporting an error instead of waiting.
	 */
	return (fcntl(fd, F_SETLKW, &lock));

#else /* HAVE_FCNTL_H */

	/* Unused parameters */
	(void)fd;
	(void)what;

	/* Success */
	return (0);

#endif /* SET_UID */

}


/*
 * Hack -- attempt to seek on a file descriptor
 */
errr fd_seek(int fd, long n)
{
	long p;

#ifdef _WIN32_WCE
  /* Verify fd */
  if (fd == INVALID_HANDLE_VALUE) return (-1);
  
  /* Seek to the given position */
  p = SetFilePointer(fd, n, NULL, FILE_BEGIN); 
  
  /* Failure */
  if (p < 0) return (1);
  
  /* Failure */
  if (p != n) return (1);
  
  /* Success */
  return (0);
#else

	/* Verify fd */
	if (fd < 0) return (-1);

	/* Seek to the given position */
	p = lseek(fd, n, SEEK_SET);

	/* Failure */
	if (p < 0) return (1);

	/* Failure */
	if (p != n) return (1);

	/* Success */
	return (0);
#endif
}


#ifndef SET_UID
#define FILE_BUF_SIZE 16384
#endif


/*
 * Hack -- attempt to read data from a file descriptor
 */
errr fd_read(int fd, char *buf, size_t n)
{
#ifdef _WIN32_WCE
  DWORD numBytesRead;
  
  /* Verify the fd */
  if (fd == INVALID_HANDLE_VALUE) return (-1);
  
#ifndef SET_UID
  
  /* Read pieces */
  while (n >= FILE_BUF_SIZE)
    {
      /* Read a piece */
      if (!ReadFile(fd, buf, FILE_BUF_SIZE, &numBytesRead, NULL))
	{
	  return (1);
	}
      
      if (numBytesRead != FILE_BUF_SIZE)
	{
	  return (1);
	}
      
      /* Shorten the task */
      buf += FILE_BUF_SIZE;
      
      /* Shorten the task */
      n -= FILE_BUF_SIZE;
    }
  
#endif
  
  /* Read the final piece */
  if (!ReadFile(fd, buf, n, &numBytesRead, NULL))
    {
      return (1);
    }
  
  if (numBytesRead != n)
    {
      return (1);
    }
  
  /* Success */
  return (0);

#else
	/* Verify the fd */
	if (fd < 0) return (-1);

#ifndef SET_UID

	/* Read pieces */
	while (n >= FILE_BUF_SIZE)
	{
		/* Read a piece */
		if (read(fd, buf, FILE_BUF_SIZE) != FILE_BUF_SIZE) return (1);

		/* Shorten the task */
		buf += FILE_BUF_SIZE;

		/* Shorten the task */
		n -= FILE_BUF_SIZE;
	}

#endif

	/* Read the final piece */
	if (read(fd, buf, n) != (int)n) return (1);

	/* Success */
	return (0);
#endif /* WCE */
}


/*
 * Hack -- Attempt to write data to a file descriptor
 */
errr fd_write(int fd, cptr buf, size_t n)
{
#ifdef _WIN32_WCE
  DWORD numBytesWrite;
  
  /* Verify the fd */
  if (fd == INVALID_HANDLE_VALUE) return (-1);
  
#ifndef SET_UID
  
  /* Write pieces */
  while (n >= FILE_BUF_SIZE)
    {
      /* Write a piece */
      if (!WriteFile(fd, buf, FILE_BUF_SIZE, &numBytesWrite, NULL))
	{
	  return (1);
	}
      
      if (numBytesWrite != FILE_BUF_SIZE)
	{
	  return (1);
	}
      
      /* Shorten the task */
      buf += FILE_BUF_SIZE;
      
      /* Shorten the task */
      n -= FILE_BUF_SIZE;
    }
  
#endif
  
  /* Write the final piece */
  if (!WriteFile(fd, buf, n, &numBytesWrite, NULL))
    {
      return (1);
    }
  
  if (numBytesWrite != n)
    {
      return (1);
    }
  
  /* Success */
  return (0);
  
#else
	/* Verify the fd */
	if (fd < 0) return (-1);

#ifndef SET_UID

	/* Write pieces */
	while (n >= FILE_BUF_SIZE)
	{
		/* Write a piece */
		if (write(fd, buf, FILE_BUF_SIZE) != FILE_BUF_SIZE) return (1);

		/* Shorten the task */
		buf += FILE_BUF_SIZE;

		/* Shorten the task */
		n -= FILE_BUF_SIZE;
	}

#endif

	/* Write the final piece */
	if (write(fd, buf, n) != (int)n) return (1);

	/* Success */
	return (0);
#endif
}


/*
 * Hack -- attempt to close a file descriptor
 */
errr fd_close(int fd)
{
#ifdef _WIN32_WCE
  /* Verify the fd */
  if (fd == INVALID_HANDLE_VALUE) return (-1);
  
  /* Close */
  if (!CloseHandle(fd))
    {
      return (-1);
    }
  
  return (0);
#else
	/* Verify the fd */
	if (fd < 0) return (-1);

	/* Close, return 0 on success, -1 on failure */
	return (close(fd));
#endif
}


#if defined(CHECK_MODIFICATION_TIME)

errr check_modification_date(int fd, cptr template_file)
{
#ifdef _WIN32_WCE
#else
	char buf[1024];

	struct stat txt_stat, raw_stat;

	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, template_file);

	/* Access stats on text file */
	if (stat(buf, &txt_stat))
	{
		/* No text file - continue */
	}

	/* Access stats on raw file */
	else if (fstat(fd, &raw_stat))
	{
		/* Error */
		return (-1);
	}

	/* Ensure text file is not newer than raw file */
	else if (txt_stat.st_mtime > raw_stat.st_mtime)
	{
		/* Reprocess text file */
		return (-1);
	}
#endif  

	return (0);
}

#endif /* CHECK_MODIFICATION_TIME */

#endif /* RISCOS */


/*
 * Format and translate a string, then print it out to file.
 */
void x_fprintf(FILE *fff, int encoding, cptr fmt, ...)
{
	va_list vp;

 	char buf[1024];

 	/* Begin the Varargs Stuff */
 	va_start(vp, fmt);

 	/* Format the args, save the length */
 	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);

 	/* End the Varargs Stuff */
 	va_end(vp);

 	/* Translate */
 	xstr_trans(buf, encoding);

 	fputs(buf, fff);
}




/*** File-handling API ***/

/* On Windows, fwrite() and fread() are broken. */
#if defined(WINDOWS) || defined(SET_UID)
# define HAVE_WRITE
# define HAVE_READ
#endif


/* Private structure to hold file pointers and useful info. */
struct ang_file
{
	FILE *fh;
	char *fname;
	file_mode mode;
};



/** Utility functions **/

/*
 * Delete file 'fname'.
 */
bool file_delete(const char *fname) 
{
	char buf[1024];

	/* Get the system-specific paths */
	path_parse(buf, sizeof(buf), fname);

#ifdef _WIN32_WCE
  {
    TCHAR wcBuf[1024];
    mbstowcs( wcBuf, buf, 1024);
    
    DeleteFile(wcBuf);
  }

  /* Assume success XXX XXX XXX */
  return (0);
  
#else

	return (remove(buf) == 0);
#endif
}

/*
 * Delete file 'fname' to 'newname'.
 */
bool file_move(const char *fname, const char *newname)
{
	char buf[1024];
	char aux[1024];

	/* Get the system-specific paths */
	path_parse(buf, sizeof(buf), fname);
	path_parse(aux, sizeof(aux), newname);
#ifdef _WIN32_WCE
  {
    TCHAR wcBuf[1024];
    TCHAR wcAux[1024];
    mbstowcs( wcBuf, buf, 1024);
    mbstowcs( wcAux, aux, 1024);
    
    MoveFile(wcBuf, wcAux);
  }
  
  /* Assume success XXX XXX XXX */
  return (0);

#else
  return (rename(buf, aux) == 0);
#endif
}


/*
 * Decide whether a file exists or not.
 */
bool file_exists(const char *fname);

#if defined(HAVE_STAT)

bool file_exists(const char *fname)
{
	struct stat st;
	return (stat(fname, &st) == 0);
}

#elif defined(WINDOWS)

/*
 * Hack -- Fake declarations from "dos.h" XXX XXX XXX
 */
#define INVALID_FILE_NAME (DWORD)0xFFFFFFFF

bool file_exists(const char *fname)
{
	char path[MAX_PATH];
	DWORD attrib;

	/* API says we mustn't pass anything larger than MAX_PATH */
	my_strcpy(path, fname, sizeof(path));

	attrib = GetFileAttributes(path);
	if (attrib == INVALID_FILE_NAME) return FALSE;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY) return FALSE;

	return TRUE;
}

#else

bool file_exists(const char *fname)
{
	ang_file *f = file_open(fname, MODE_READ, 0);

	if (f) file_close(f);
	return (f ? TRUE : FALSE);
}

#endif


#ifndef RISCOS
#ifdef HAVE_STAT

/*
 * Return TRUE if first is newer than second, FALSE otherwise.
 */
bool file_newer(const char *first, const char *second)
{
	struct stat first_stat, second_stat;

	bool second_exists = stat(second, &second_stat) ? FALSE : TRUE;
	bool first_exists = stat(first, &first_stat) ? FALSE : TRUE;

	/*
	 * If the first doesn't exist, the first is not newer;
	 * If the second doesn't exist, the first is always newer.
	 */
	if (!first_exists)  return FALSE;
	if (!second_exists) return TRUE;

	if (first_stat.st_mtime >= second_stat.st_mtime)
		return TRUE;

	return FALSE;
}

#else /* HAVE_STAT */

bool file_newer(const char *first, const char *second)
{
	/* Assume newer */
	return FALSE;
}

#endif /* HAVE_STAT */
#endif /* RISCOS */




/** File-handle functions **/

/*
 * Open file 'fname', in mode 'mode', with filetype 'ftype'.
 * Returns file handle or NULL.
 */
ang_file *file_open(const char *fname, file_mode mode, file_type ftype)
{
	ang_file *f = ZNEW(ang_file);
	char modestr[3] = "__";
	char buf[1024];

	/* Get the system-specific path */
	path_parse(buf, sizeof(buf), fname);

	switch (mode)
	{
		case MODE_WRITE:
			modestr[0] = 'w';
			modestr[1] = 'b';
			break;
		case MODE_READ:
			modestr[0] = 'r';
			modestr[1] = 'b';
			break;
		case MODE_APPEND:
			modestr[0] = 'w';
			modestr[1] = 'a';
			break;
		default:
			break;
	}

	f->fh = fopen(buf, modestr);

	if (f->fh == NULL)
	{
		FREE(f);
		return NULL;
	}

	f->fname = (char *)string_make(buf);
	f->mode = mode;

#ifdef MACH_O_CARBON
	extern void fsetfileinfo(cptr path, u32b fcreator, u32b ftype);

	/* OS X uses its own kind of filetypes */
	if (mode != MODE_READ)
	{
		char mac_type = 'TEXT';

		if (ftype == FTYPE_RAW) mac_type = 'DATA';
		else if (ftype == FTYPE_SAVE) mac_type = 'SAVE';

		fsetfileinfo(buf, 'A271', ftype);
	}
#endif

#if defined(RISCOS) && 0
	/* do something for RISC OS here? */
	if (mode != MODE_READ)
		File_SetType(n, ftype);
#endif

	return f;
}

/*
 * Close file handle 'f'.
 */
bool file_close(ang_file *f)
{
	if (fclose(f->fh) != 0)
		return FALSE;

	FREE(f->fname);
	FREE(f);

	return TRUE;
}



/** Locking functions **/

/*
 * Lock a file using POSIX locks, on platforms where this is supported.
 */
void file_lock(ang_file *f)
{
#if defined(HAVE_FCNTL_H) && defined(SET_UID)
	struct flock lock;
	lock.l_type = (f->mode == MODE_READ ? F_RDLCK : F_WRLCK);
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = 0;
	fcntl(fileno(f->fh), F_SETLKW, &lock);
#endif /* HAVE_FCNTL_H && SET_UID */
}

/*
 * Unlock a file locked using file_lock().
 */
void file_unlock(ang_file *f)
{
#if defined(HAVE_FCNTL_H) && defined(SET_UID)
	struct flock lock;
	lock.l_type = F_UNLCK;
	lock.l_whence = SEEK_SET;
	lock.l_start = 0;
	lock.l_len = 0;
	lock.l_pid = 0;
	fcntl(fileno(f->fh), F_SETLK, &lock);
#endif /* HAVE_FCNTL_H && SET_UID */
}


/** Byte-based IO and functions **/

/*
 * Seek to location 'pos' in file 'f'.
 */
bool file_seek(ang_file *f, u32b pos)
{
	return (fseek(f->fh, pos, SEEK_SET) == 0);
}

/*
 * Read a single, 8-bit character from file 'f'.
 */
bool file_readc(ang_file *f, byte *b)
{
	int i = fgetc(f->fh);

	if (i == EOF)
		return FALSE;

	*b = (byte)i;
	return TRUE;
}

/* 
 * Write a single, 8-bit character 'b' to file 'f'.
 */
bool file_writec(ang_file *f, byte b)
{
	return (fputc((int)b, f->fh) != EOF);
}

/*
 * Read 'n' bytes from file 'f' into array 'buf'.
 */
size_t file_read(ang_file *f, char *buf, size_t n);

#ifdef HAVE_READ

#ifndef SET_UID
# define READ_BUF_SIZE 16384
#endif

size_t file_read(ang_file *f, char *buf, size_t n)
{
#ifdef _WIN32_WCE
  DWORD numBytesRead;
  
#endif
	int fd = fileno(f->fh);

#ifndef SET_UID

	while (n >= READ_BUF_SIZE)
	{

#ifdef _WIN32_WCE
	  /* Read a piece */
	  if (!ReadFile(fd, buf, FILE_BUF_SIZE, &numBytesRead, NULL))
	    {
	      return FALSE;
	    }
	  
	  if (numBytesRead != FILE_BUF_SIZE)
	    {
	      return FALSE;
	    }
#else
	  if (read(fd, buf, READ_BUF_SIZE) != READ_BUF_SIZE)
	    return FALSE;
#endif	  

	  buf += READ_BUF_SIZE;
	  n -= READ_BUF_SIZE;
	}

#endif /* !SET_UID */

#ifdef _WIN32_WCE
	  if (!ReadFile(fd, buf, n, &numBytesRead, NULL))
	    {
	      return FALSE;
	    }
	  
	  if (numBytesRead != (int)n)
	    {
	      return FALSE;
	    }
#else
	if (read(fd, buf, n) != (int)n)
		return FALSE;
#endif

	return TRUE;
}

#else

size_t file_read(ang_file *f, char *buf, size_t n)
{
	return fread(buf, 1, n, f->fh);
}

#endif


/*
 * Append 'n' bytes of array 'buf' to file 'f'.
 */
bool file_write(ang_file *f, const char *buf, size_t n);

#ifdef HAVE_WRITE

#ifndef SET_UID
# define WRITE_BUF_SIZE 16384
#endif

bool file_write(ang_file *f, const char *buf, size_t n)
{
#ifdef _WIN32_WCE
  DWORD numBytesWrite;
  
#endif
	int fd = fileno(f->fh);

#ifndef SET_UID

	while (n >= WRITE_BUF_SIZE)
	{
#ifdef _WIN32_WCE
	  /* Write a piece */
	  if (!WriteFile(fd, buf, FILE_BUF_SIZE, &numBytesWrite, NULL))
	    {
	      return FALSE;
	    }
	  
	  if (numBytesWrite != FILE_BUF_SIZE)
	    {
	      return FALSE;
	    }
#else

		if (write(fd, buf, WRITE_BUF_SIZE) != WRITE_BUF_SIZE)
			return FALSE;

#endif
		buf += WRITE_BUF_SIZE;
		n -= WRITE_BUF_SIZE;
	}

#endif /* !SET_UID */

#ifdef _WIN32_WCE
	  /* Write a piece */
	  if (!WriteFile(fd, buf, n, &numBytesWrite, NULL))
	    {
	      return FALSE;
	    }
	  
	  if (numBytesWrite != (int)n)
	    {
	      return FALSE;
	    }
#else
	if (write(fd, buf, n) != (int)n)
		return FALSE;
#endif

	return TRUE;
}

#else

bool file_write(ang_file *f, const char *buf, size_t n)
{
	return (fwrite(buf, 1, n, f->fh) == n);
}

#endif 


/** Line-based IO **/

/*
 * Read a line of text from file 'f' into buffer 'buf' of size 'n' bytes.
 *
 * Support both \r\n and \n as line endings, but not the outdated \r that used
 * to be used on Macs.  Replace non-printables with '?', and \ts with ' '.
 */
#define TAB_COLUMNS 4

bool file_getl(ang_file *f, char *buf, size_t len)
{
	bool seen_cr = FALSE;
	byte b;
	size_t i = 0;

	/* Leave a byte for the terminating 0 */
	size_t max_len = len - 1;

	while (i < max_len)
	{
		char c;

		if (!file_readc(f, &b))
		{
			buf[i] = '\0';
			return (i == 0) ? FALSE : TRUE;
		}

		c = (char) b;

		if (c == '\r')
		{
			seen_cr = TRUE;
			continue;
		}

		if (seen_cr && c != '\n')
		{
			fseek(f->fh, -1, SEEK_CUR);
			buf[i] = '\0';
			return TRUE;
		}

		if (c == '\n')
		{
			buf[i] = '\0';
			return TRUE;
		}

		/* Expand tabs */
		if (c == '\t')
		{
			/* Next tab stop */
			size_t tabstop = ((i + TAB_COLUMNS) / TAB_COLUMNS) * TAB_COLUMNS;
			if (tabstop >= len) break;

			/* Convert to spaces */
			while (i < tabstop)
				buf[i++] = ' ';

			continue;
		}

		/* Ignore non-printables */
		if (!isprint((unsigned char) c))
		{
			buf[i++] = '?';
			continue;
		}

		buf[i++] = c;
	}

	return TRUE;
}

/*
 * Append a line of text 'buf' to the end of file 'f', using system-dependent
 * line ending.
 */
bool file_put(ang_file *f, const char *buf)
{
	return file_write(f, buf, strlen(buf));
}

/*
 * Append a formatted line of text to the end of file 'f'.
 */
bool file_putf(ang_file *f, const char *fmt, ...)
{
	char buf[1024];
	va_list vp;

	va_start(vp, fmt);
	(void)vstrnfmt(buf, sizeof(buf), fmt, vp);
	va_end(vp);

	return file_put(f, buf);
}


/*** Directory scanning code ***/

/*
 * This code was originally written for the SDL port so it could scan for fonts
 * without needing a fontlist text file.
 */


/*
 * Opens a directory handle.
 * 
 * `dirname` must be a system-specific pathname to the directory
 * you want scanned.
 *
 * Returns a valid directory handle on success, NULL otherwise.
 */
ang_dir *my_dopen(const char *dirname);


/*
 * Reads a directory entry.
 *
 * `dir` must point to a directory handle previously returned by my_dopen().
 * `fname` must be a pointer to a writeable chunk of memory `len` long.
 *
 * Returns TRUE on successful reading, FALSE otherwise.
 * (FALSE generally indicates that there are no more files to be read.)
 */
bool my_dread(ang_dir *dir, char *fname, size_t len);


/*
 * Close a directory handle.
 */
void my_dclose(ang_dir *dir);




#ifdef WINDOWS

/* Include Windows header */
#include <windows.h>

/* System-specific struct */
struct ang_dir
{
	HANDLE h;
	const char *first_file;
};

/* Specified above */
ang_dir *my_dopen(const char *dirname)
{
	WIN32_FIND_DATA fd;
	HANDLE h;
   	ang_dir *dir;
	
	/* Try to open it */
	h = FindFirstFile(format("%s\\*", dirname), &fd);

	/* Abort */
	if (h == INVALID_HANDLE_VALUE)
		return NULL;

	/* Allocate for the handle */
	dir = ralloc(sizeof dir);
	if (!dir) return NULL;

	/* Remember details */
	dir->h = h;
	dir->first_file = string_make(fd.cFileName);

	/* Success */
	return dir;
}

/* Specified above */
bool my_dread(ang_dir *dir, char *fname, size_t len)
{
	WIN32_FIND_DATA fd;
	BOOL ok;

	/* Try the first file */
	if (dir->first_file)
	{
		/* Copy the string across, then free it */
		my_strcpy(fname, dir->first_file, len);
		string_free(dir->first_file);
		dir->first_file = NULL;

		/* Wild success */
		return TRUE;
	}

	/* Try the next file */
	while (1)
	{
		ok = FindNextFile(dir->h, &fd);
		if (!ok) return FALSE;

		/* Skip directories */
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY ||
		    strcmp(fd.cFileName, ".") == 0 ||
		    strcmp(fd.cFileName, "..") == 0)
			continue;

		/* Take this one */
		break;
	}

	/* Copy name */
	my_strcpy(fname, fd.cFileName, len);

	return TRUE;
}

void my_dclose(ang_dir *dir)
{
	/* Close directory */
	if (dir->h)
		FindClose(dir->h);

	/* Free memory */
	FREE(dir);
}

#endif /* WINDOWS */


#ifdef HAVE_DIRENT_H

/* Include relevant types */
#include <sys/types.h>
#include <dirent.h>

/* Define our ang_dir type */
struct ang_dir
{
	DIR *d;
	const char *dirname;
};

/* Specified above */
ang_dir *my_dopen(const char *dirname)
{
	ang_dir *dir;
	DIR *d;

	/* Try to open the directory */
	d = opendir(dirname);
	if (!d) return NULL;

	/* Allocate memory for the handle */
	dir = ralloc(sizeof dir);
	if (!dir) 
	{
		closedir(d);
		return NULL;
	}

	/* Set up the handle */
	dir->d = d;
	dir->dirname = string_make(dirname);

	if (!dir->dirname)
	{
		closedir(d);
		rnfree(dir);
		return NULL;
	}

	/* Success */
	return dir;
}

/* Specified above */
bool my_dread(ang_dir *dir, char *fname, size_t len)
{
	struct dirent *entry;
	struct stat filedata;
	char path[1024] = "";

	assert(dir != NULL);

	/* Try reading another entry */
	while (1)
	{
		entry = readdir(dir->d);
		if (!entry) return FALSE;

		path_build(path, sizeof path, dir->dirname, entry->d_name);
            
		/* Check to see if it exists */
		if (stat(path, &filedata) != 0)
			continue;

		/* Check to see if it's a directory */
		if (S_ISDIR(filedata.st_mode))
			continue;

		/* We've found something worth returning */
		break;
	}

	/* Copy the filename */
	my_strcpy(fname, entry->d_name, len);

	return TRUE;
}

void my_dclose(ang_dir *dir)
{
	/* Close directory */
	if (dir->d)
	{
		closedir(dir->d);
		string_free(dir->dirname);
	}

	/* Free memory */
	FREE(dir);
}

#endif /* HAVE_DIRENT_H */

