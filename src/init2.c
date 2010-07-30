/* File: init2.c */

/* Paths, initializiation of *_info arrays from the binary files, control
 * of what items are sold in the stores, prepare stores, inventory, and
 * many other things, some error text, startup initializations.
 *
 * Copyright (c) 1997 Ben Harrison
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.  Other copyrights may also apply.
 */

#include "angband.h"

#include "init.h"
#include "cmds.h"

/*
 * This file is used to initialize various variables and arrays for the
 * Angband game.  Note the use of "fd_read()" and "fd_write()" to bypass
 * the common limitation of "read()" and "write()" to only 32767 bytes
 * at a time.
 *
 * Several of the arrays for Angband are built from "template" files in
 * the "lib/file" directory, from which quick-load binary "image" files
 * are constructed whenever they are not present in the "lib/data"
 * directory, or if those files become obsolete, if we are allowed.
 *
 * Warning -- the "ascii" file parsers use a minor hack to collect the
 * name and text information in a single pass.  Thus, the game will not
 * be able to load any template file with more than 20K of names or 60K
 * of text, even though technically, up to 64K should be legal.
 *
 * The "init1.c" file is used only to parse the ascii template files,
 * to create the binary image files.  If you include the binary image
 * files instead of the ascii template files, then you can undefine
 * "ALLOW_TEMPLATES", saving about 20K by removing "init1.c".  Note
 * that the binary image files are extremely system dependant.
 */



/*
 * Find the default paths to all of our important sub-directories.
 *
 * The purpose of each sub-directory is described in "variable.c".
 *
 * All of the sub-directories should, by default, be located inside
 * the main "lib" directory, whose location is very system dependant.
 *
 * This function takes a writable buffer, initially containing the
 * "path" to the "lib" directory, for example, "/pkg/lib/angband/",
 * or a system dependant string, for example, ":lib:".  The buffer
 * must be large enough to contain at least 32 more characters.
 *
 * Various command line options may allow some of the important
 * directories to be changed to user-specified directories, most
 * importantly, the "info" and "user" and "save" directories,
 * but this is done after this function, see "main.c".
 *
 * In general, the initial path should end in the appropriate "PATH_SEP"
 * string.  All of the "sub-directory" paths (created below or supplied
 * by the user) will NOT end in the "PATH_SEP" string, see the special
 * "path_build()" function in "util.c" for more information.
 *
 * Hack -- first we free all the strings, since this is known
 * to succeed even if the strings have not been allocated yet,
 * as long as the variables start out as "NULL".  This allows
 * this function to be called multiple times, for example, to
 * try several base "path" values until a good one is found.
 */
void init_file_paths(const char *path)
{

#ifdef PRIVATE_USER_PATH
        char buf[1024];
	char dirpath[1024];
#endif /* PRIVATE_USER_PATH */
	
	/*** Free everything ***/
	
	/* Free the main path */
	string_free(ANGBAND_DIR);

	/* Free the sub-paths */
	string_free(ANGBAND_DIR_APEX);
	string_free(ANGBAND_DIR_BONE);
	string_free(ANGBAND_DIR_DATA);
	string_free(ANGBAND_DIR_EDIT);
	string_free(ANGBAND_DIR_FILE);
	string_free(ANGBAND_DIR_HELP);
	string_free(ANGBAND_DIR_INFO);
	string_free(ANGBAND_DIR_SAVE);
	string_free(ANGBAND_DIR_PREF);
	string_free(ANGBAND_DIR_USER);
	string_free(ANGBAND_DIR_XTRA);
	
  
	/*** Prepare the paths ***/
	
	/* Save the main directory */
	ANGBAND_DIR = string_make(path);
	
#ifdef VM
  
  
	/*** Use "flat" paths with VM/ESA ***/

	/* Use "blank" path names */
	ANGBAND_DIR_APEX = string_make("");
	ANGBAND_DIR_BONE = string_make("");
	ANGBAND_DIR_DATA = string_make("");
	ANGBAND_DIR_EDIT = string_make("");
	ANGBAND_DIR_FILE = string_make("");
	ANGBAND_DIR_HELP = string_make("");
	ANGBAND_DIR_INFO = string_make("");
	ANGBAND_DIR_PREF = string_make("");
	ANGBAND_DIR_SAVE = string_make("");
	ANGBAND_DIR_USER = string_make("");
	ANGBAND_DIR_XTRA = string_make("");
	

#else /* VM */

  
	/*** Build the sub-directory names ***/
	
	/* Build path names */
	ANGBAND_DIR_EDIT = string_make(format("%sedit", path));
	ANGBAND_DIR_FILE = string_make(format("%sfile", path));
	ANGBAND_DIR_HELP = string_make(format("%shelp", path));
	ANGBAND_DIR_INFO = string_make(format("%sinfo", path));
	ANGBAND_DIR_PREF = string_make(format("%spref", path));
	ANGBAND_DIR_XTRA = string_make(format("%sxtra", path));

  

#ifdef PRIVATE_USER_PATH

	/* Get an absolute path from the filename */
	path_parse(dirpath, sizeof(dirpath), PRIVATE_USER_PATH);
	/* Build the path to the user specific directory */
	path_build(buf, sizeof(buf), dirpath, VERSION_NAME);
	
	/* Build a relative path name */
	ANGBAND_DIR_USER = string_make(buf);
	
#else /* PRIVATE_USER_PATH */
	
	ANGBAND_DIR_USER = string_make(format("%suser", path));
  
#endif /* PRIVATE_USER_PATH */

#ifdef USE_PRIVATE_PATHS
	
	/* Build the path to the user specific sub-directory */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "scores");
	ANGBAND_DIR_APEX = string_make(buf);
	
	/* Build the path to the user specific sub-directory */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "bone");
	ANGBAND_DIR_BONE = string_make(buf);
	
	/* Build the path to the user specific sub-directory */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "data");
	ANGBAND_DIR_DATA = string_make(buf);
	
	/* Build the path to the user specific sub-directory */
	path_build(buf, sizeof(buf), ANGBAND_DIR_USER, "save");
	ANGBAND_DIR_SAVE = string_make(buf);
	
#else /* USE_PRIVATE_PATHS */
	
	/* Build pathnames */
	ANGBAND_DIR_APEX = string_make(format("%sapex", path));
	ANGBAND_DIR_BONE = string_make(format("%sbone", path));
	ANGBAND_DIR_DATA = string_make(format("%sdata", path));
	ANGBAND_DIR_SAVE = string_make(format("%ssave", path));
	
#endif /* USE_PRIVATE_PATHS */
	
#endif /* VM */


#ifdef NeXT

	/* Allow "fat binary" usage with NeXT */
	if (TRUE)
	{
		cptr next = NULL;

# if defined(m68k)
		next = "m68k";
# endif

# if defined(i386)
		next = "i386";
# endif

# if defined(sparc)
		next = "sparc";
# endif

# if defined(hppa)
		next = "hppa";
# endif

		/* Use special directory */
		if (next)
		{
			/* Forget the old path name */
			string_free(ANGBAND_DIR_DATA);

			/* Build a new path name */
			sprintf(tail, "data-%s", next);
			ANGBAND_DIR_DATA = string_make(path);
		}
	}

#endif /* NeXT */

}


#ifdef PRIVATE_USER_PATH

/*
 * Create an ".angband/" directory in the users home directory.
 *
 * ToDo: Add error handling.
 * ToDo: Only create the directories when actually writing files.
 */
void create_user_dirs(void)
{
        char dirpath[1024];
	char subdirpath[1024];
	
	
	/* Get an absolute path from the filename */
	path_parse(dirpath, sizeof(dirpath), PRIVATE_USER_PATH);
	
	/* Create the ~/.angband/ directory */
	mkdir(dirpath, 0700);
	
	/* Build the path to the variant-specific sub-directory */
	path_build(subdirpath, sizeof(subdirpath), dirpath, VERSION_NAME);
	
	/* Create the directory */
	mkdir(subdirpath, 0700);
	
#ifdef USE_PRIVATE_PATHS
	/* Build the path to the scores sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "scores");
	
	/* Create the directory */
	mkdir(dirpath, 0700);
	
	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "bone");
	
	/* Create the directory */
	mkdir(dirpath, 0700);
	
	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "data");
	
	/* Create the directory */
	mkdir(dirpath, 0700);
	
	/* Build the path to the savefile sub-directory */
	path_build(dirpath, sizeof(dirpath), subdirpath, "save");
	
	/* Create the directory */
	mkdir(dirpath, 0700);
#endif /* USE_PRIVATE_PATHS */
}

#endif /* PRIVATE_USER_PATH */



#ifdef ALLOW_TEMPLATES


/*
 * Hack -- help give useful error messages
 */
int error_idx;
int error_line;


/*
 * Standard error message text
 */
static cptr err_str[PARSE_ERROR_MAX] =
{
        NULL,
	"parse error",
	"obsolete file",
	"missing record header",
	"non-sequential records",
	"invalid flag specification",
	"undefined directive",
	"out of memory",
	"value out of bounds",
	"too few arguments",
	"too many arguments",
	"too many allocation entries",
	"invalid spell frequency",
	"invalid number of items (0-99)",
	"too many entries",
	"vault too big",
};


#endif /* ALLOW_TEMPLATES */



/*
 * File headers
 */
header z_head;
header v_head;
header f_head;
header k_head;
header a_head;
header e_head;
header r_head;
header p_head;
header ch_head;
header mp_head;
header c_head;
header h_head;
header b_head;
header g_head;
header flavor_head;
header s_head;


/*** Initialize from binary image files ***/


/*
 * Initialize a "*_info" array, by parsing a binary "image" file
 */
static errr init_info_raw(int fd, header *head)
{
        header test;


        /* Read and verify the header */
        if (fd_read(fd, (char*)(&test), sizeof(header)) ||
            (test.v_major != head->v_major) ||
            (test.v_minor != head->v_minor) ||
            (test.v_patch != head->v_patch) ||
            (test.v_extra != head->v_extra) ||
            (test.info_num != head->info_num) ||
            (test.info_len != head->info_len) ||
            (test.head_size != head->head_size) ||
            (test.info_size != head->info_size))
        {
                /* Error */
                return (-1);
	}


        /* Accept the header */
        (void)COPY(head, &test, header);


        /* Allocate the "*_info" array */
        C_MAKE(head->info_ptr, head->info_size, char);

        /* Read the "*_info" array */
        fd_read(fd, head->info_ptr, head->info_size);

        if (head->name_size)
        {
                /* Allocate the "*_name" array */
                C_MAKE(head->name_ptr, head->name_size, char);

                /* Read the "*_name" array */
                fd_read(fd, head->name_ptr, head->name_size);
        }

        if (head->text_size)
        {
                /* Allocate the "*_text" array */
                C_MAKE(head->text_ptr, head->text_size, char);

                /* Read the "*_text" array */
                fd_read(fd, head->text_ptr, head->text_size);
        }

        /* Success */
        return (0);
}


/*
 * Initialize the header of an *_info.raw file.
 */
static void init_header(header *head, int num, int len)
{
        /* Save the "version" */
        head->v_major = O_VERSION_MAJOR;
        head->v_minor = O_VERSION_MINOR;
        head->v_patch = O_VERSION_PATCH;
        head->v_extra = O_VERSION_EXTRA;

        /* Save the "record" information */
        head->info_num = num;
        head->info_len = len;

        /* Save the size of "*_head" and "*_info" */
        head->head_size = sizeof(header);
        head->info_size = head->info_num * head->info_len;

        /* Clear post-parsing evaluation function */
        head->eval_info_power = NULL;
        
        /* Clear the template emission functions */
        head->emit_info_txt_index = NULL;
        head->emit_info_txt_always = NULL;
}


#ifdef ALLOW_TEMPLATES

/*
 * Display a parser error message.
 */
static void display_parse_error(cptr filename, errr err, cptr buf)
{
        cptr oops;

        /* Error string */
        oops = (((err > 0) && (err < PARSE_ERROR_MAX)) ? err_str[err] : "unknown");

        /* Oops */
        msg_format("Error at line %d of '%s.txt'.", error_line, filename);
        msg_format("Record %d contains a '%s' error.", error_idx, oops);
        msg_format("Parsing '%s'.", buf);
        message_flush();

        /* Quit */
        quit_fmt("Error in '%s.txt' file.", filename);
}

#endif /* ALLOW_TEMPLATES */


/*
 * Initialize a "*_info" array
 *
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
static errr init_info(cptr filename, header *head)
{
        int fd;
  
	errr err = 1;
	
	FILE *fp;
	
#ifdef ALLOW_TEMPLATES_OUTPUT
	FILE *fpout;
#endif /* ALLOW_TEMPLATES_OUTPUT */
	
	/* General buffer */
	char buf[1024];
	
	
#ifdef ALLOW_TEMPLATES
	
	/*** Load the binary image file ***/
	
	/* Build the filename */
	path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, format("%s.raw", filename));
	
	/* Attempt to open the "raw" file */
	fd = fd_open(buf, O_RDONLY);
	
	/* Process existing "raw" file */
#ifdef _WIN32_WCE
	if (fd != -1)
#else
	        if (fd >= 0)
#endif
		{
#ifdef CHECK_MODIFICATION_TIME
	      
	                err = check_modification_date(fd, format("%s.txt", filename));
	      
#endif /* CHECK_MODIFICATION_TIME */
      
			/* Attempt to parse the "raw" file */
			if (!err) err = init_info_raw(fd, head);
			
			/* Close it */
			fd_close(fd);
		}
	
	/* Do we have to parse the *.txt file? */
	if (err)
	{
	        /*** Make the fake arrays ***/
	    
	        /* Allocate the "*_info" array */
	        C_MAKE(head->info_ptr, head->info_size, char);
	    
		/* MegaHack -- make "fake" arrays */
		if (z_info)
		{
		        C_MAKE(head->name_ptr, z_info->fake_name_size, char);
			C_MAKE(head->text_ptr, z_info->fake_text_size, char);
		}
		
      
		/*** Load the ascii template file ***/
      
		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_EDIT, 
			   format("%s.txt", filename));
      
		/* Open the file */
		fp = my_fopen(buf, "r");
      
		/* Parse it */
		if (!fp) quit(format("Cannot open '%s.txt' file.", filename));
      
		/* Parse the file */
		err = init_info_txt(fp, buf, head, head->parse_info_txt);
      
		/* Close it */
		my_fclose(fp);
      
		/* Errors */
		if (err) display_parse_error(filename, err, buf);
      
#ifdef ALLOW_TEMPLATES_OUTPUT
      
		/*** Output a 'parsable' ascii template file ***/
		if ((head->emit_info_txt_index) || (head->emit_info_txt_always))
		{
			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_EDIT, format("%s.txt", filename));
          
			/* Open the file */
			fp = my_fopen(buf, "r");
          
			/* Parse it */
			if (!fp) quit(format("Cannot open '%s.txt' file for re-parsing.", 
					     filename));
          
			/* Build the filename */
			path_build(buf, 1024, ANGBAND_DIR_USER, format("%s.txt", filename));
          
			/* Open the file */
			fpout = my_fopen(buf, "w");
          
			/* Parse it */
			if (!fpout) quit(format("Cannot open '%s.txt' file for output.", 
						filename));
          
			/* Parse and output the files */
			err = emit_info_txt(fpout, fp, buf, head, head->emit_info_txt_index, 
					    head->emit_info_txt_always);
          
			/* Close both files */
			my_fclose(fpout);
			my_fclose(fp);
		}
      
#endif
      
      
		/*** Dump the binary image file ***/
      
		/* File type is "DATA" */
		FILE_TYPE(FILE_TYPE_DATA);
      
		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, 
			   format("%s.raw", filename));
      
      
		/* Attempt to open the file */
		fd = fd_open(buf, O_RDONLY);
      
		/* Failure */
#ifdef _WIN32_WCE
		if (fd == -1)
#else
			if (fd < 0)
#endif
			{
				int mode = 0644;
          
				/* Grab permissions */
				safe_setuid_grab();
          
				/* Create a new file */
				fd = fd_make(buf, mode);
          
				/* Drop permissions */
				safe_setuid_drop();
          
				/* Failure */
#ifdef _WIN32_WCE
				if (fd == -1)
#else
					if (fd < 0)
#endif
					{
						/* Complain */
						plog_fmt("Cannot create the '%s' file!", buf);
              
						/* Continue */
						return (0);
					}
			}
      
		/* Close it */
		fd_close(fd);
      
		/* Grab permissions */
		safe_setuid_grab();
      
		/* Attempt to create the raw file */
		fd = fd_open(buf, O_WRONLY);
      
		/* Drop permissions */
		safe_setuid_drop();
      
		/* Failure */
#ifdef _WIN32_WCE
		if (fd == -1)
#else
			if (fd < 0)
#endif
			{
				/* Complain */
				plog_fmt("Cannot write the '%s' file!", buf);
          
				/* Continue */
				return (0);
			}
      
		/* Dump to the file */
#ifdef _WIN32_WCE
		if (fd != -1)
#else
			if (fd >= 0)
#endif
			{
				/* Dump it */
				fd_write(fd, (cptr)head, head->head_size);
          
				/* Dump the "*_info" array */
				if (head->info_size > 0)
					fd_write(fd, head->info_ptr, head->info_size);
          
				/* Dump the "*_name" array */
				if (head->name_size > 0)
					fd_write(fd, head->name_ptr, head->name_size);
          
				/* Dump the "*_text" array */
				if (head->text_size > 0)
					fd_write(fd, head->text_ptr, head->text_size);
          
				/* Close */
				fd_close(fd);
			}

      
		/*** Kill the fake arrays ***/
      
		/* Free the "*_info" array */
		KILL(head->info_ptr);

		/* MegaHack -- Free the "fake" arrays */
		if (z_info)
		{
			KILL(head->name_ptr);
			KILL(head->text_ptr);
		}
      
#endif /* ALLOW_TEMPLATES */
      

		/*** Load the binary image file ***/
      
		/* Build the filename */
		path_build(buf, sizeof(buf), ANGBAND_DIR_DATA, 
			   format("%s.raw", filename));
      
		/* Attempt to open the "raw" file */
		fd = fd_open(buf, O_RDONLY);
      
		/* Process existing "raw" file */
#ifdef _WIN32_WCE
		if (fd == -1) quit("Cannot load '%s.raw' file.");
#else
		if (fd < 0) quit(format("Cannot load '%s.raw' file.", filename));
#endif      

		/* Attempt to parse the "raw" file */
		err = init_info_raw(fd, head);
      
		/* Close it */
		fd_close(fd);
      
		/* Error */
		if (err) quit(format("Cannot parse '%s.raw' file.", filename));
      
#ifdef ALLOW_TEMPLATES
	}
#endif /* ALLOW_TEMPLATES */
  
	/* Success */
	return (0);
}


/*
 * Free the allocated memory for the info-, name-, and text- arrays.
 */
static errr free_info(header *head)
{
	if (head->info_size)
		(void)FREE(head->info_ptr);
  
	if (head->name_size)
		(void)FREE(head->name_ptr);
  
	if (head->text_size)
		(void)FREE(head->text_ptr);
  
	/* Success */
	return (0);
}


/*
 * Initialize the "z_info" array
 */
static errr init_z_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&z_head, 1, sizeof(maxima));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	z_head.parse_info_txt = parse_z_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("limits", &z_head);
  
	/* Set the global variables */
	z_info = z_head.info_ptr;
  
	return (err);
}


/*
 * Initialize the "f_info" array
 */
static errr init_f_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&f_head, z_info->f_max, sizeof(feature_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	f_head.parse_info_txt = parse_f_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("terrain", &f_head);
  
	/* Set the global variables */
	f_info = f_head.info_ptr;
	f_name = f_head.name_ptr;
	f_text = f_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "k_info" array
 */
static errr init_k_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&k_head, z_info->k_max, sizeof(object_kind));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	k_head.parse_info_txt = parse_k_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("object", &k_head);
  
	/* Set the global variables */
	k_info = k_head.info_ptr;
	k_name = k_head.name_ptr;
	k_text = k_head.text_ptr;
  
	return (err);
}



/*
 * In Angband, all of the "special" artifacts are first in the
 * artifact list. The first non-special artifact is at ART_MIN_NORMAL.
 * OAngband adds several new special artifacts greater than ART_MIN_NORMAL.
 * As a further catch, Morgoth's Crown and Hammer are special artifacts
 * greater than ART_MIN_NORMAL which should not be created until Morgoth
 * is defeated. -TNB-
 */

/* Lists of normal and special a_info[] indexes */
int *artifact_normal, *artifact_special;
int artifact_normal_cnt, artifact_special_cnt;

/*
 * This routine separates all the normal and special artifacts into
 * separate lists for easy allocation later.
 */
void init_artifacts(void)
{
	int loop;
  
	/* First: count. Second: build lists */
	for (loop = 0; loop <= 1; loop++)
	{
		int a_idx;
      
		artifact_normal_cnt = 0;
		artifact_special_cnt = 0;
      
		/* Check every artifact (including randoms) */
		for (a_idx = 1; a_idx < z_info->a_max; a_idx++)
		{
			/* Access this artifact */
			artifact_type *a_ptr = &a_info[a_idx];
          
			/* Require "real" artifacts */
			if (!a_ptr->name) continue;
          
			/* This is a "special" artifact */
			if ((a_idx < ART_MIN_NORMAL) ||
			    (a_ptr->tval == TV_AMULET) ||
			    (a_ptr->tval == TV_RING) ||
			    (a_ptr->tval == TV_ROD) ||
			    (a_ptr->tval == TV_STAFF) ||
			    (a_ptr->tval == TV_WAND))
			{
				if (loop == 1)
				{
					artifact_special[artifact_special_cnt] = a_idx;
				}
              
				/* Count the special artifacts */
				artifact_special_cnt++;
			}
          
			/*
			 * This is a "normal" artifact. Notice we must skip
			 * Morgoth's Crown and Hammer.
			 */
			else if (!(a_ptr->flags3 & TR3_INSTA_ART))
			{
				if (loop == 1)
				{
					artifact_normal[artifact_normal_cnt] = a_idx;
				}
              
				/* Count the normal artifacts */
				artifact_normal_cnt++;
			}
		}
      
		/* Allocate the lists the first time through */
		if (loop == 0)
		{
			C_MAKE(artifact_normal, artifact_normal_cnt, int);
			C_MAKE(artifact_special, artifact_special_cnt, int);
		}
	}
}


/*
 * Initialize the "a_info" array
 */
static errr init_a_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&a_head, z_info->a_max, sizeof(artifact_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	a_head.parse_info_txt = parse_a_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("artifact", &a_head);
  
	/* Set the global variables */
	a_info = a_head.info_ptr;
	a_name = a_head.name_ptr;
	a_text = a_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "e_info" array
 */
static errr init_e_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&e_head, z_info->e_max, sizeof(ego_item_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	e_head.parse_info_txt = parse_e_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("ego_item", &e_head);
  
	/* Set the global variables */
	e_info = e_head.info_ptr;
	e_name = e_head.name_ptr;
	e_text = e_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "r_info" array
 */
static errr init_r_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&r_head, z_info->r_max, sizeof(monster_race));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	r_head.parse_info_txt = parse_r_info;
  
#ifdef ALLOW_TEMPLATES_PROCESS
	/* Save a pointer to the evaluate power function*/
	r_head.eval_info_power = eval_r_power;
#endif
  
#ifdef ALLOW_TEMPLATES_OUTPUT
  
	/* Save a pointer to the evaluate power function*/
	r_head.emit_info_txt_index = emit_r_info_index;
#endif /* ALLOW_TEMPLATES_OUTPUT */
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("monster", &r_head);
  
	/* Set the global variables */
	r_info = r_head.info_ptr;
	r_name = r_head.name_ptr;
	r_text = r_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "v_info" array
 */
static errr init_v_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&v_head, z_info->v_max, sizeof(vault_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	v_head.parse_info_txt = parse_v_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("vault", &v_head);
  
	/* Set the global variables */
	v_info = v_head.info_ptr;
	v_name = v_head.name_ptr;
	v_text = v_head.text_ptr;
  
	return (err);
}





/*
 * Initialize the "p_info" array
 */
static errr init_p_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&p_head, z_info->p_max, sizeof(player_race));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	p_head.parse_info_txt = parse_p_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("p_race", &p_head);
  
	/* Set the global variables */
	p_info = p_head.info_ptr;
	p_name = p_head.name_ptr;
	p_text = p_head.text_ptr;
  
	return (err);
}





/*
 * Initialize the "ch_info" array
 */
static errr init_ch_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&ch_head, z_info->ch_max, sizeof(chest_drops));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	ch_head.parse_info_txt = parse_ch_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("p_chest", &ch_head);
  
	/* Set the global variables */
	ch_info = ch_head.info_ptr;
	ch_name = ch_head.name_ptr;
	ch_text = ch_head.text_ptr;
  
	return (err);
}

/*
 * Initialize the "mp_info" array
 */
static errr init_mp_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&mp_head, z_info->mp_max, sizeof(player_magic));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	mp_head.parse_info_txt = parse_mp_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("p_magic", &mp_head);
  
	/* Set the global variables */
	mp_info = mp_head.info_ptr;
	mp_name = mp_head.name_ptr;
	mp_text = mp_head.text_ptr;
  
	return (err);
}

/*
 * Initialize the "c_info" array
 */
static errr init_c_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&c_head, z_info->c_max, sizeof(player_class));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	c_head.parse_info_txt = parse_c_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("p_class", &c_head);
  
	/* Set the global variables */
	c_info = c_head.info_ptr;
	c_name = c_head.name_ptr;
	c_text = c_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "h_info" array
 */
static errr init_h_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&h_head, z_info->h_max, sizeof(hist_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	h_head.parse_info_txt = parse_h_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("p_hist", &h_head);
  
	/* Set the global variables */
	h_info = h_head.info_ptr;
	h_text = h_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "b_info" array
 */
static errr init_b_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&b_head, (u16b)(MAX_STORES * z_info->b_max), sizeof(owner_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	b_head.parse_info_txt = parse_b_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("shop_own", &b_head);
  
	/* Set the global variables */
	b_info = b_head.info_ptr;
	b_name = b_head.name_ptr;
  
	return (err);
}



/*
 * Initialize the "g_info" array
 */
static errr init_g_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&g_head, (u16b)(z_info->p_max * z_info->p_max), sizeof(byte));

#ifdef ALLOW_TEMPLATES

	/* Save a pointer to the parsing function */
	g_head.parse_info_txt = parse_g_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("cost_adj", &g_head);
  
	/* Set the global variables */
	g_info = g_head.info_ptr;
  
	return (err);
}


/*
 * Initialize the "flavor_info" array
 */
static errr init_flavor_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&flavor_head, z_info->flavor_max, sizeof(flavor_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	flavor_head.parse_info_txt = parse_flavor_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("flavor", &flavor_head);
  
	/* Set the global variables */
	flavor_info = flavor_head.info_ptr;
	flavor_name = flavor_head.name_ptr;
	flavor_text = flavor_head.text_ptr;
  
	return (err);
}



/*
 * Initialize the "s_info" array
 */
static errr init_s_info(void)
{
	errr err;
  
	/* Init the header */
	init_header(&s_head, z_info->s_max, sizeof(set_type));
  
#ifdef ALLOW_TEMPLATES
  
	/* Save a pointer to the parsing function */
	s_head.parse_info_txt = parse_s_info;
  
#endif /* ALLOW_TEMPLATES */
  
	err = init_info("set_item", &s_head);
  
	/* Set the global variables */
	s_info = s_head.info_ptr;
	s_name = s_head.name_ptr;
	s_text = s_head.text_ptr;
  
	return (err);
}


/*
 * Note that we let each entry have a unique "name" and "text" string,
 * even if the string happens to be empty (everyone has a unique '\0').
 */
errr init_t_info(byte chosen_level)
{
	errr err;

	FILE *fp;

	/* General buffer */
	char buf[1024];

#ifndef NO_THEMED_LEVELS	/* Themed levels and old machines don't mix. */

	/*** Make the header ***/

	/* Allocate the "header" */
	MAKE(t_head, header);
  
	/* Save the "version" */
	t_head->v_major = VERSION_MAJOR;
	t_head->v_minor = VERSION_MINOR;
	t_head->v_patch = VERSION_PATCH;
	t_head->v_extra = 0;
  
	/* Save the "record" information */
	t_head->info_num = z_info->v_max;
	t_head->info_len = sizeof(vault_type);
  
	/* Save the size of "v_head" and "v_info" */
	t_head->head_size = sizeof(header);
	t_head->info_size = t_head->info_num * t_head->info_len;


	/*** Make the fake arrays ***/

	/* Allocate the "t_info" array */
	C_MAKE(t_info, t_head->info_num, vault_type);

	/* Hack -- make "fake" arrays */
	C_MAKE(t_name, FAKE_NAME_SIZE, char);
	C_MAKE(t_text, FAKE_TEXT_SIZE, char);


	/*** Load the ascii template file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_EDIT, "themed.txt");

	/* Open the file */
	fp = my_fopen(buf, "r");

	/* Parse it, canceling on failure. */
	if (!fp) return(TRUE);

	/* Parse the file */
	err = init_t_info_txt(fp, buf, chosen_level);

	/* Close it */
	my_fclose(fp);

	/* Errors */
	if (err)
	{
		cptr oops;

		/* Error string */
		oops = (((err > 0) && (err < 8)) ? err_str[err] : "unknown");

		/* Oops */
		msg_format("Non-critical error: themed.txt is unusable.");
		msg_format("Error %d at line %d of 'themed.txt'.", err, error_line);
		msg_format("Record %d contains a '%s' error.", error_idx, oops);
		msg_format("Parsing '%s'.", buf);
		msg_print(NULL);

		/* Failure */
		return(TRUE);
	}




	/* Success */
	return (0);

#else

	/* No themed levels allowed if compiler option forbids them. */
	return(1);

#endif
}

/*
 * Release memory used to store information about a themed level. -LM-
 */
void kill_t_info(void)
{

	/*** Kill the fake arrays ***/
  
	/* Free the "t_info" array */
	FREE(t_info);
  
	/* Hack -- Free the "fake" arrays */
	FREE(t_name);
	FREE(t_text);
}







/*** Initialize others ***/


/*
 * Hack -- Objects sold in the stores -- by tval/sval pair.
 * Note code for initializing the stores, below.
 */
static byte store_table[MAX_STORES][STORE_CHOICES][2] =
{
	{
		/* General Store. */

		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_RATION },
		{ TV_FOOD, SV_FOOD_BISCUIT },
		{ TV_FOOD, SV_FOOD_BISCUIT },
		{ TV_FOOD, SV_FOOD_JERKY },
		{ TV_FOOD, SV_FOOD_JERKY },

		{ TV_FOOD, SV_FOOD_PINT_OF_WINE },
		{ TV_FOOD, SV_FOOD_PINT_OF_ALE },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_TORCH },
		{ TV_LITE, SV_LITE_LANTERN },
		{ TV_LITE, SV_LITE_LANTERN },

		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_FLASK, 0 },
		{ TV_SPIKE, 0 },
		{ TV_SPIKE, 0 },

		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_DIGGING, SV_SHOVEL },
		{ TV_DIGGING, SV_PICK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_CLOAK },
		{ TV_CLOAK, SV_CLOAK }
	},

	{
		/* Armoury */

		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_SOFT_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_BOOTS, SV_PAIR_OF_HARD_LEATHER_BOOTS },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_HARD_LEATHER_CAP },
		{ TV_HELM, SV_METAL_CAP },
		{ TV_HELM, SV_IRON_HELM },

		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_ROBE },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_SOFT_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_LEATHER_ARMOR },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },
		{ TV_SOFT_ARMOR, SV_HARD_STUDDED_LEATHER },

		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ TV_SOFT_ARMOR, SV_LEATHER_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_METAL_SCALE_MAIL },
		{ TV_HARD_ARMOR, SV_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_AUGMENTED_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_BAR_CHAIN_MAIL },
		{ TV_HARD_ARMOR, SV_DOUBLE_CHAIN_MAIL },

		{ TV_HARD_ARMOR, SV_METAL_BRIGANDINE_ARMOUR },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_LEATHER_GLOVES },
		{ TV_GLOVES, SV_SET_OF_MAIL_GAUNTLETS },
		{ TV_SHIELD, SV_WICKER_SHIELD },
		{ TV_SHIELD, SV_SMALL_LEATHER_SHIELD },
		{ TV_SHIELD, SV_LARGE_LEATHER_SHIELD },
		{ TV_SHIELD, SV_SMALL_METAL_SHIELD }
	},

	{
		/* Weaponsmith */

		{ TV_SWORD, SV_DAGGER },
		{ TV_SWORD, SV_DAGGER },
		{ TV_SWORD, SV_MAIN_GAUCHE },
		{ TV_SWORD, SV_RAPIER },
		{ TV_SWORD, SV_SMALL_SWORD },
		{ TV_SWORD, SV_SHORT_SWORD },
		{ TV_SWORD, SV_SABRE },
		{ TV_SWORD, SV_CUTLASS },

		{ TV_SWORD, SV_BROAD_SWORD },
		{ TV_SWORD, SV_LONG_SWORD },
		{ TV_SWORD, SV_SCIMITAR },
		{ TV_SWORD, SV_KATANA },
		{ TV_SWORD, SV_BASTARD_SWORD },
		{ TV_SWORD, SV_TWO_HANDED_SWORD },
		{ TV_POLEARM, SV_SPEAR },
		{ TV_POLEARM, SV_TRIDENT },

		{ TV_POLEARM, SV_PIKE },
		{ TV_POLEARM, SV_BEAKED_AXE },
		{ TV_POLEARM, SV_BROAD_AXE },
		{ TV_POLEARM, SV_DART },
		{ TV_POLEARM, SV_BATTLE_AXE },
		{ TV_BOW, SV_SLING },
		{ TV_BOW, SV_SLING },
		{ TV_BOW, SV_SHORT_BOW },

		{ TV_BOW, SV_LONG_BOW },
		{ TV_BOW, SV_LIGHT_XBOW },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_SHOT, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_ARROW, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
		{ TV_BOLT, SV_AMMO_NORMAL },
	},

	{
		/* Temple. */

		{ TV_HAFTED, SV_WHIP },
		{ TV_HAFTED, SV_QUARTERSTAFF },
		{ TV_HAFTED, SV_MACE },
		{ TV_HAFTED, SV_MACE },
		{ TV_SCROLL, SV_SCROLL_PROTECTION_FROM_EVIL },
		{ TV_HAFTED, SV_WAR_HAMMER },
		{ TV_HAFTED, SV_WAR_HAMMER },
		{ TV_HAFTED, SV_MORNING_STAR },

		{ TV_HAFTED, SV_FLAIL },
		{ TV_HAFTED, SV_FLAIL },
		{ TV_HAFTED, SV_LEAD_FILLED_MACE },
		{ TV_SCROLL, SV_SCROLL_REMOVE_CURSE },
		{ TV_SCROLL, SV_SCROLL_BLESSING },
		{ TV_SCROLL, SV_SCROLL_HOLY_CHANT },
		{ TV_POTION, SV_POTION_BOLDNESS },
		{ TV_POTION, SV_POTION_HEROISM },

		{ TV_POTION, SV_POTION_CURE_LIGHT },
		{ TV_SCROLL, SV_SCROLL_RECHARGING },
		{ TV_POTION, SV_POTION_RESIST_ACID_ELEC },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_CURE_POISON },
		{ TV_POTION, SV_POTION_SLOW_POISON },
		{ TV_POTION, SV_POTION_RESIST_HEAT_COLD },
		{ TV_POTION, SV_POTION_RESTORE_EXP },

		{ TV_POTION, SV_POTION_CURE_LIGHT },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_SERIOUS },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_CURE_CRITICAL },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
		{ TV_POTION, SV_POTION_RESTORE_EXP },
		{ TV_POTION, SV_POTION_RESTORE_EXP }
	},

	{
		/* Alchemy shop.  All the general-purpose scrolls and potions. */

		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_HIT },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_WEAPON_TO_DAM },
		{ TV_SCROLL, SV_SCROLL_ENCHANT_ARMOR },
		{ TV_SCROLL, SV_SCROLL_TELEPORT },
		{ TV_SCROLL, SV_SCROLL_TELEPORT_LEVEL },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_IDENTIFY },
		{ TV_SCROLL, SV_SCROLL_LIGHT },

		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_PHASE_DOOR },
		{ TV_SCROLL, SV_SCROLL_MONSTER_CONFUSION },
		{ TV_SCROLL, SV_SCROLL_MAPPING },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_DETECT_TRAP },

		{ TV_SCROLL, SV_SCROLL_DETECT_DOOR },
		{ TV_SCROLL, SV_SCROLL_DETECT_INVIS },
		{ TV_SCROLL, SV_SCROLL_RECHARGING },
		{ TV_SCROLL, SV_SCROLL_SATISFY_HUNGER },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },
		{ TV_SCROLL, SV_SCROLL_WORD_OF_RECALL },

		{ TV_SCROLL, SV_SCROLL_DISPEL_UNDEAD },
		{ TV_POTION, SV_POTION_HEROISM },
		{ TV_POTION, SV_POTION_RES_STR },
		{ TV_POTION, SV_POTION_RES_INT },
		{ TV_POTION, SV_POTION_RES_WIS },
		{ TV_POTION, SV_POTION_RES_DEX },
		{ TV_POTION, SV_POTION_RES_CON },
		{ TV_POTION, SV_POTION_RES_CHR }
	},

	{
		/* Magic-User store. */

		{ TV_RING, SV_RING_SEARCHING },
		{ TV_RING, SV_RING_FEATHER_FALL },
		{ TV_RING, SV_RING_PROTECTION },
		{ TV_AMULET, SV_AMULET_CHARISMA },
		{ TV_AMULET, SV_AMULET_SLOW_DIGEST },
		{ TV_AMULET, SV_AMULET_RESIST_ACID },
		{ TV_WAND, SV_WAND_SLOW_MONSTER },
		{ TV_WAND, SV_WAND_CONFUSE_MONSTER },

		{ TV_WAND, SV_WAND_SLEEP_MONSTER },
		{ TV_WAND, SV_WAND_MAGIC_MISSILE },
		{ TV_WAND, SV_WAND_STINKING_CLOUD },
		{ TV_RING, SV_RING_RESIST_COLD },
		{ TV_RING, SV_RING_RESIST_FIRE },
		{ TV_RING, SV_RING_SLOW_DIGESTION },
		{ TV_STAFF, SV_STAFF_DETECT_TRAP },
		{ TV_STAFF, SV_STAFF_DETECT_DOOR },
		
		{ TV_STAFF, SV_STAFF_DETECT_GOLD },
		{ TV_STAFF, SV_STAFF_DETECT_ITEM },
		{ TV_STAFF, SV_STAFF_DETECT_INVIS },
		{ TV_STAFF, SV_STAFF_DETECT_EVIL },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_TELEPORTATION },
		{ TV_STAFF, SV_STAFF_IDENTIFY },
		{ TV_STAFF, SV_STAFF_IDENTIFY },

		{ TV_WAND, SV_WAND_DOOR_DEST },
		{ TV_WAND, SV_WAND_STONE_TO_MUD },
		{ TV_WAND, SV_WAND_STINKING_CLOUD },
		{ TV_WAND, SV_WAND_POLYMORPH },
		{ TV_STAFF, SV_STAFF_LITE },
		{ TV_STAFF, SV_STAFF_MAPPING },
		{ TV_STAFF, SV_ROD_DETECT_TRAP },
		{ TV_STAFF, SV_ROD_DETECT_DOOR }
	},

	{
		/* Black Market (unused) */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	},

	{
		/* Home (unused) */
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 },
		{ 0, 0 }
	},

	{
		/* Bookseller. */

		{ TV_MAGIC_BOOK, 0 },
		{ TV_MAGIC_BOOK, 0 },
		{ TV_MAGIC_BOOK, 0 },
		{ TV_MAGIC_BOOK, 1 },
		{ TV_MAGIC_BOOK, 1 },
		{ TV_MAGIC_BOOK, 2 },
		{ TV_MAGIC_BOOK, 2 },
		{ TV_MAGIC_BOOK, 3 },

		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 0 },
		{ TV_PRAYER_BOOK, 1 },
		{ TV_PRAYER_BOOK, 1 },
		{ TV_PRAYER_BOOK, 2 },
		{ TV_PRAYER_BOOK, 2 },
		{ TV_PRAYER_BOOK, 3 },

		{ TV_DRUID_BOOK, 0 },
		{ TV_DRUID_BOOK, 0 },
		{ TV_DRUID_BOOK, 0 },
		{ TV_DRUID_BOOK, 1 },
		{ TV_DRUID_BOOK, 1 },
		{ TV_DRUID_BOOK, 2 },
		{ TV_DRUID_BOOK, 2 },
		{ TV_DRUID_BOOK, 3 },

		{ TV_NECRO_BOOK, 0 },
		{ TV_NECRO_BOOK, 0 },
		{ TV_NECRO_BOOK, 0 },
		{ TV_NECRO_BOOK, 1 },
		{ TV_NECRO_BOOK, 1 },
		{ TV_NECRO_BOOK, 2 },
		{ TV_NECRO_BOOK, 2 },
		{ TV_NECRO_BOOK, 3 }
	}  

};





/*** Initialize others ***/

static void autoinscribe_init(void)
{
        if (inscriptions) FREE(inscriptions);
  
	inscriptions = 0;
	inscriptions_count = 0;
	
	C_MAKE(inscriptions, AUTOINSCRIPTIONS_MAX, autoinscription);
}


/*
 * Initialize some other arrays
 */
static errr init_other(void)
{
        int i = 0, k, n;
  
	/*** Prepare the various "bizarre" arrays ***/
	
	(void)macro_init();
	
	/* Initialize the "quark" package */
	(void)quarks_init();
	
	/* Initialize squelch things */
	autoinscribe_init();
	
	/* Initialize the "message" package */
	(void)messages_init();
	
	
	/*** Prepare grid arrays ***/
	C_MAKE(view_g, VIEW_MAX, u16b);

	/* Array of grids */
	C_MAKE(temp_g, TEMP_MAX, u16b);

	/* Hack -- use some memory twice */
	temp_y = ((byte*)(temp_g)) + 0;
	temp_x = ((byte*)(temp_g)) + TEMP_MAX;

  
	/*** Prepare dungeon arrays ***/
	
	/* Padded info array */
	C_MAKE(cave_info, DUNGEON_HGT, byte_256);
	C_MAKE(cave_info2, DUNGEON_HGT, byte_256);
	
	/* Feature array */
	C_MAKE(cave_feat, DUNGEON_HGT, byte_wid);
	
	/* Entity arrays */
	C_MAKE(cave_o_idx, DUNGEON_HGT, s16b_wid);
	C_MAKE(cave_m_idx, DUNGEON_HGT, s16b_wid);
	
	/* Lore */
	C_MAKE(l_list, z_info->r_max, monster_lore);
	
	/* Flow arrays */
	C_MAKE(cave_cost, DUNGEON_HGT, byte_wid);
	C_MAKE(cave_when, DUNGEON_HGT, byte_wid);
	
	
	/*** Prepare entity arrays ***/
	
	/* Objects */
	C_MAKE(o_list, z_info->o_max, object_type);
	
	/* Monsters */
	C_MAKE(m_list, z_info->m_max, monster_type);
  
	/*** Prepare character display arrays ***/
  
	/* Lines of character screen/dump */
	C_MAKE(dumpline, DUMP_MAX_LINES, char_attr_line);

	/* Lines of character subwindows */
	C_MAKE(pline0, 30, char_attr_line);
	C_MAKE(pline1, 30, char_attr_line);
  
	/*** Prepare mouse button arrays ***/
	C_MAKE(mse_button, MAX_MOUSE_BUTTONS, mouse_button);
	C_MAKE(backup_button, MAX_MOUSE_BUTTONS, mouse_button);
	
	/* Initialise the hooks */
	add_button_hook = add_button_text;
	kill_button_hook = kill_button_text;
	
	/*** Prepare quest array ***/
	
	/* Quests */
	C_MAKE(q_list, MAX_Q_IDX, quest);
	
	
	/*** Prepare notes array ***/
	
	/* Notes */
	C_MAKE(notes, NOTES_MAX_LINES, note_info);
	
	for (n = 0; n < NOTES_MAX_LINES; n++)
	{
		notes[n].turn = 0;
	}
	
	/*** Prepare the inventory ***/
	
	/* Allocate it */
	C_MAKE(inventory, INVEN_TOTAL, object_type);
	
	
	/*** Prepare the stores ***/
	
	/* Allocate the stores */
	C_MAKE(store, MAX_STORES, store_type);
	
	for (i = 0; i < MAX_STORES; i++)
	{
	        /* Access the store */
	        store_type *st_ptr = &store[i];
	    
		/* Assume full stock */
		st_ptr->stock_size = STORE_INVEN_MAX;
		
		/* Allocate the stock */
		C_MAKE(st_ptr->stock, st_ptr->stock_size, object_type);
		
		/* No table for the black market or home */
		if ((i == 6) || (i == 7)) continue;
		
		/* Assume full table */
		st_ptr->table_size = STORE_CHOICES;
		
		/* Nothing there yet */
		st_ptr->table_num = 0;
		
		/* Allocate the stock */
		C_MAKE(st_ptr->table, st_ptr->table_size, s16b);
		
		/* Scan the choices */
		for (k = 0; k < STORE_CHOICES; k++)
		{
		        int k_idx;
		
			/* Extract the tval/sval codes */
			int tv = store_table[i][k][0];
			int sv = store_table[i][k][1];
			
			/* Look for it */
			for (k_idx = 1; k_idx < z_info->k_max; k_idx++)
			{
		                object_kind *k_ptr = &k_info[k_idx];
		    
				/* Found a match */
				if ((k_ptr->tval == tv) && (k_ptr->sval == sv)) break;
			}
			
			/* Catch errors */
			if (k_idx == z_info->k_max) continue;
			
			/* Add that item index to the table */
			st_ptr->table[st_ptr->table_num++] = k_idx;
		}
	}


	/*** Pre-allocate the basic "auto-inscriptions" ***/

	/* Some extra strings */
	(void)quark_add("on sale");


	/*** Prepare the options ***/

	/* Initialize the options */
	for (i = 0; i < OPT_MAX; i++)
	{
		/* Default value */
		op_ptr->opt[i] = option_norm[i];
	}

	/* Initialize the window flags */
	for (n = 0; n < 8; n++)
	{
		/* Assume no flags */
		op_ptr->window_flag[n] = 0L;
	}

	/* Initialize the delay_factor */
	op_ptr->delay_factor = 1;

	/*** Pre-allocate space for the "format()" buffer ***/
	
	/* Hack -- Just call the "format()" function */
        (void)format("%s (%s).", "Bahman Rabii", MAINTAINER);
	
	
	/* Success */
	return (0);
}



/*
 * Initialize some other arrays
 */
static errr init_alloc(void)
{
	int i, j;

	object_kind *k_ptr;

	monster_race *r_ptr;

	ego_item_type *e_ptr;

	alloc_entry *table;

	s16b num[MAX_DEPTH];

	s16b aux[MAX_DEPTH];


	/*** Analyze object allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(&aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(&num, MAX_DEPTH, s16b);

	/* Size of "alloc_kind_table" */
	alloc_kind_size = 0;
	
	/* Scan the objects */
	for (i = 1; i < z_info->k_max; i++)
	{
	        k_ptr = &k_info[i];
      
		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				/* Count the entries */
				alloc_kind_size++;

				/* Group by level */
				num[k_ptr->locale[j]]++;
			}
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town objects!");


	/*** Initialize object allocation info ***/

	/* Allocate the alloc_kind_table */
	C_MAKE(alloc_kind_table, alloc_kind_size, alloc_entry);

	/* Access the table entry */
	table = alloc_kind_table;
	
	/* Scan the objects */
	for (i = 1; i < z_info->k_max; i++)
	{
	        k_ptr = &k_info[i];
      
		/* Scan allocation pairs */
		for (j = 0; j < 4; j++)
		{
			/* Count the "legal" entries */
			if (k_ptr->chance[j])
			{
				int p, x, y, z;

				/* Extract the base level */
				x = k_ptr->locale[j];

				/* Extract the base probability */
				p = (100 / k_ptr->chance[j]);

				/* Skip entries preceding our locale */
				y = (x > 0) ? num[x-1] : 0;

				/* Skip previous entries at this locale */
				z = y + aux[x];

				/* Load the entry */
				table[z].index = i;
				table[z].level = x;
				table[z].prob1 = p;
				table[z].prob2 = p;
				table[z].prob3 = p;

				/* Another entry complete for this locale */
				aux[x]++;
			}
		}
	}


	/*** Analyze monster allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(&aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(&num, MAX_DEPTH, s16b);

	/* Size of "alloc_race_table" */
	alloc_race_size = 0;
	
	/* Scan the monsters */
	for (i = 1; i < z_info->r_max; i++)
	{
	        /* Get the i'th race */
	        r_ptr = &r_info[i];

		/* Legal monsters */
		if (r_ptr->rarity)
		{
			/* Count the entries */
			alloc_race_size++;

			/* Group by level */
			num[r_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/* Paranoia */
	if (!num[0]) quit("No town monsters!");


	/*** Initialize monster allocation info ***/

	/* Allocate the alloc_race_table */
	C_MAKE(alloc_race_table, alloc_race_size, alloc_entry);

	/* Access the table entry */
	table = alloc_race_table;
	
	/* Scan the monsters */
	for (i = 1; i < z_info->r_max; i++)
	{
	        /* Get the i'th race */
	        r_ptr = &r_info[i];

		/* Count valid pairs */
		if (r_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = r_ptr->level;

			/* Extract the base probability */
			p = (100 / r_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}


	/*** Analyze ego_item allocation info ***/

	/* Clear the "aux" array */
	(void)C_WIPE(aux, MAX_DEPTH, s16b);

	/* Clear the "num" array */
	(void)C_WIPE(num, MAX_DEPTH, s16b);

	/* Size of "alloc_ego_table" */
	alloc_ego_size = 0;
	
	/* Scan the ego items */
	for (i = 1; i < z_info->e_max; i++)
	{
	        /* Get the i'th ego item */
	        e_ptr = &e_info[i];

		/* Legal items */
		if (e_ptr->rarity)
		{
			/* Count the entries */
			alloc_ego_size++;

			/* Group by level */
			num[e_ptr->level]++;
		}
	}

	/* Collect the level indexes */
	for (i = 1; i < MAX_DEPTH; i++)
	{
		/* Group by level */
		num[i] += num[i-1];
	}

	/*** Initialize ego-item allocation info ***/

	/* Allocate the alloc_ego_table */
	C_MAKE(alloc_ego_table, alloc_ego_size, alloc_entry);

	/* Get the table entry */
	table = alloc_ego_table;
	
	/* Scan the ego-items */
	for (i = 1; i < z_info->e_max; i++)
	{
	        /* Get the i'th ego item */
                e_ptr = &e_info[i];

		/* Count valid pairs */
		if (e_ptr->rarity)
		{
			int p, x, y, z;

			/* Extract the base level */
			x = e_ptr->level;

			/* Extract the base probability */
			p = (100 / e_ptr->rarity);

			/* Skip entries preceding our locale */
			y = (x > 0) ? num[x-1] : 0;

			/* Skip previous entries at this locale */
			z = y + aux[x];

			/* Load the entry */
			table[z].index = i;
			table[z].level = x;
			table[z].prob1 = p;
			table[z].prob2 = p;
			table[z].prob3 = p;

			/* Another entry complete for this locale */
			aux[x]++;
		}
	}
	
	
	/* Success */
	return (0);
}



/*
 * Hack -- take notes on line 23
 */
static void note(cptr str)
{
        int col = MAX(0, (Term->wid - strlen(str)) / 2);
	int row = Term->hgt - 1;
	Term_erase(0, row, 255);
	Term_putstr(col, row, -1, TERM_WHITE, str);
	Term_fresh();
}

/*
 * Hack -- Explain a broken "lib" folder and quit (see below).
 *
 * XXX XXX XXX This function is "messy" because various things
 * may or may not be initialized, but the "plog()" and "quit()"
 * functions are "supposed" to work under any conditions.
 */
static void init_angband_aux(cptr why)
{
	/* Why */
	plog(why);

	/* Explain */
	plog("The 'lib' directory is probably missing or broken.");

	/* More details */
	plog("Perhaps the archive was not extracted correctly.");

	/* Explain */
	plog("See the 'README' file for more information.");

	/* Quit with error */
	quit("Fatal Error.");
}

/*
 * Hack - identify set item artifacts.
 *
 * Go through the list of Set Items and identify all artifacts in each set
 * as belonging to that set. By GS
 */
void update_artifact_sets()
{
 	byte i;
	byte j;
	set_type *s_ptr;
	set_element *se_ptr;
	artifact_type *a_ptr;
	
	for (i = 0; i < z_info->s_max; i++)
	{
      
	        s_ptr = &s_info[i];
		for (j=0;j<s_ptr->no_of_items;j++)
		{
			se_ptr= &s_ptr->set_items[j];
			a_ptr = &a_info[se_ptr->a_idx];
			a_ptr->set_no = i;
		}
	}
}

/*
 * Encode the screen colors for the opening screen
 */
static char hack[17] = "dwsorgbuDWvyRGBU";

/*
 * Hack -- main Angband initialization entry point
 *
 * Verify some files, display the "news.txt" file, create
 * the high score file, initialize all internal arrays, and
 * load the basic "user pref files".
 *
 * Be very careful to keep track of the order in which things
 * are initialized, in particular, the only thing *known* to
 * be available when this function is called is the "z-term.c"
 * package, and that may not be fully initialized until the
 * end of this function, when the default "user pref files"
 * are loaded and "Term_xtra(TERM_XTRA_REACT,0)" is called.
 *
 * Note that this function attempts to verify the "news" file,
 * and the game aborts (cleanly) on failure, since without the
 * "news" file, it is likely that the "lib" folder has not been
 * correctly located.  Otherwise, the news file is displayed for
 * the user.
 *
 * Note that this function attempts to verify (or create) the
 * "high score" file, and the game aborts (cleanly) on failure,
 * since one of the most common "extraction" failures involves
 * failing to extract all sub-directories (even empty ones), such
 * as by failing to use the "-d" option of "pkunzip", or failing
 * to use the "save empty directories" option with "Compact Pro".
 * This error will often be caught by the "high score" creation
 * code below, since the "lib/apex" directory, being empty in the
 * standard distributions, is most likely to be "lost", making it
 * impossible to create the high score file.
 *
 * Note that various things are initialized by this function,
 * including everything that was once done by "init_some_arrays".
 *
 * This initialization involves the parsing of special files
 * in the "lib/data" and sometimes the "lib/edit" directories.
 *
 * Note that the "template" files are initialized first, since they
 * often contain errors.  This means that macros and message recall
 * and things like that are not available until after they are done.
 *
 * We load the default "user pref files" here in case any "color"
 * changes are needed before character creation.
 *
 * Note that the "graf-xxx.prf" file must be loaded separately,
 * if needed, in the first (?) pass through "TERM_XTRA_REACT".
 */
void init_angband(void)
{
	int fd = -1;

	int mode = 0644;

	FILE *fp;

	char buf[1024];

	/*** Verify the "news" file ***/

	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_FILE, "news.txt");

	/* Attempt to open the file */
	fd = fd_open(buf, O_RDONLY);
	
	/* Failure */
#ifdef _WIN32_WCE
	if (fd == -1)
#else
	        if (fd < 0)
#endif
		{
		        char why[1024];
      
			/* Message */
			sprintf(why, "Cannot access the '%s' file!", buf);
			
			/* Crash and burn */
			init_angband_aux(why);
		}
	
	/* Close it */
	fd_close(fd);
	
	
	/*** Display the "news" file ***/
	
	/* Clear screen */
	Term_clear();
	
	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_FILE, (small_screen ? "news_s.txt" : "news.txt"));
  
	/* Open the News file */
	fp = my_fopen(buf, "r");
  
	/* Dump */
	if (fp)
	{
	        int i, y, x;
      
		byte a = 0;
		char c = ' ';
		
		bool okay = TRUE;
		
		int len;
		
		
		/* Load the screen */
		for (y = 0; okay; y++)
		{
		        /* Get a line of data */
		        if (my_fgets(fp, buf, 1024)) okay = FALSE;
		  
			/* Stop on blank line */
			if (!buf[0]) break;
			
			/* Get the width */
			len = strlen(buf);
			
			/* XXX Restrict to current screen size */
			if (len >= Term->wid) len = Term->wid;
			
			/* Show each row */
			for (x = 0; x < len; x++)
			{
			        /* Put the attr/char */
			        Term_draw(x, y, TERM_WHITE, buf[x]);
			}
		}
		
		/* Load the screen */
		for (y = 0; okay; y++)
		{
		        /* Get a line of data */
		        if (my_fgets(fp, buf, 1024)) okay = FALSE;
		    
			/* Stop on blank line */
			if (!buf[0]) break;
			
			/* Get the width */
			len = strlen(buf);
			
			/* XXX Restrict to current screen size */
			if (len >= Term->wid) len = Term->wid;
			
			/* Show each row */
			for (x = 0; x < len; x++)
			{
			        /* Get the attr/char */
			        (void)(Term_what(x, y, &a, &c));
              
				/* Look up the attr */
				for (i = 0; i < 16; i++)
				{
				        /* Use attr matches */
				        if (hack[i] == buf[x]) a = i;
				}
				
				/* Put the attr/char */
				Term_draw(x, y, a, c);
			}
			
			/* End the row */
			/*            fprintf(fp, "\n"); */
		}
		
		
		/* Get the blank line */
		/* if (my_fgets(fp, buf, 1024)) okay = FALSE; */
		
		
		/* Close it */
		my_fclose(fp);
	}
	
	/* Flush it */
	Term_fresh();
	
	/*** Verify (or create) the "high score" file ***/
	
	/* Build the filename */
	path_build(buf, 1024, ANGBAND_DIR_APEX, "scores.raw");
	
	/* Attempt to open the high score file */
	fd = fd_open(buf, O_RDONLY);
	
	/* Failure */
#ifdef _WIN32_WCE
	if (fd == -1)
#else
	        if (fd < 0)
#endif
		{
		        /* File type is "DATA" */
		        FILE_TYPE(FILE_TYPE_DATA);

			/* Create a new high score file */
			fd = fd_make(buf, mode);
			
			/* Failure */
#ifdef _WIN32_WCE
			if (fd == -1) 
#else
			        if (fd < 0)
#endif
				{
				        char why[1024];
          
					/* Message */
					sprintf(why, "Cannot create the '%s' file!", buf);
					
					/* Crash and burn */
					init_angband_aux(why);
				}
		}
	
	/* Close it */
	fd_close(fd);
	
	
	/* Initialize the menus */
	/* This must occur before preference files are read */
	init_cmd4_c();
	
	/*** Initialize some arrays ***/
	
	/* Initialize size info */
	note("[Initializing array sizes...]");
	if (init_z_info()) quit("Cannot initialize sizes");
	
	/* Initialize feature info */
	note("[Initializing arrays... (features)]");
	if (init_f_info()) quit("Cannot initialize features");
	
	/* Initialize object info */
	note("[Initializing arrays... (objects)]");
	if (init_k_info()) quit("Cannot initialize objects");

	/* Initialize artifact info */
	note("[Initializing arrays... (artifacts)]");
	if (init_a_info()) quit("Cannot initialize artifacts");

	/* Initialize set item info */
	note("[Initializing arrays... (set items)]");
	if (init_s_info()) quit("Cannot initialize set items");
	update_artifact_sets();

	/* Initialize ego-item info */
	note("[Initializing arrays... (ego-items)]");
	if (init_e_info()) quit("Cannot initialize ego-items");

	/* Initialize monster info */
	note("[Initializing arrays... (monsters)]");
	if (init_r_info()) quit("Cannot initialize monsters");

	/* Initialize feature info */
	note("[Initializing arrays... (vaults)]");
	if (init_v_info()) quit("Cannot initialize vaults");

	/* Initialize history info */
	note("[Initializing arrays... (histories)]");
	if (init_h_info()) quit("Cannot initialize histories");
  
	/* Initialize race info */
	note("[Initializing arrays... (races)]");
	if (init_p_info()) quit("Cannot initialize races");
	
	/* Initialize class info */
	note("[Initializing arrays... (classes)]");
	if (init_c_info()) quit("Cannot initialize classes");
	
	/* Initialize class info */
	note("[Initializing arrays... (flavors)]");
	if (init_flavor_info()) quit("Cannot initialize flavors");
	
	/* Initialize chest info */
	note("[Initializing arrays... (chests)]");
	if (init_ch_info()) quit("Cannot initialize chests");

	/* Initialize magic info */
	note("[Initializing arrays... (magic)]");
	if (init_mp_info()) quit("Cannot initialize magic");

	/* Initialize price info */
	note("[Initializing arrays... (prices)]");
	if (init_g_info()) quit("Cannot initialize prices");

	/* Initialize owner info */
	note("[Initializing arrays... (owners)]");
	if (init_b_info()) quit("Cannot initialize owners");

	/* Initialize some other arrays */
	note("[Initializing arrays... (other)]");
	if (init_other()) quit("Cannot initialize other stuff");

	/* Initialize some other arrays */
	note("[Initializing arrays... (alloc)]");
	if (init_alloc()) quit("Cannot initialize alloc stuff");


	/*** Load default user pref files ***/

	/* Initialize feature info */
	note("[Loading basic user pref file...]");

	/* Process that file */
	(void)process_pref_file("pref.prf");
  
	/* Done */
	note("[Initialization complete]");
	
	/* Sneakily init command list */
	cmd_init();
}



void cleanup_angband(void)
{
        int i;
  
  
	/* Free the macros */
	macro_free();
	
	/* Free the macro triggers */
	macro_trigger_free();
	
	/* Free the allocation tables */
	FREE(alloc_ego_table);
	FREE(alloc_race_table);
	FREE(alloc_kind_table);
	
	if (store)
	{
	        /* Free the store inventories */
	        for (i = 0; i < MAX_STORES; i++)
		{
		        /* Get the store */
		        store_type *st_ptr = &store[i];
          
			/* Free the store inventory */
			FREE(st_ptr->stock);
			
			/* Free the store table */
			FREE(st_ptr->table);
			
		}
	}
	
	/* Free the stores */
	FREE(store);
	
	/* Free the player inventory */
	FREE(inventory);
	
	
	/* Free the character display arrays */
	FREE(dumpline);
	FREE(pline0);
	FREE(pline1);
	
	/* Free the mouse button array */
	FREE(mse_button);
	FREE(backup_button);
	
	/* Free the quest list */
	FREE(q_list);
	
	/* Free the lore, monster, and object lists */
	FREE(l_list);
	FREE(m_list);
	FREE(o_list);
	
#ifdef MONSTER_FLOW
  
	/* Flow arrays */
	FREE(cave_when);
	FREE(cave_cost);
	
#endif /* MONSTER_FLOW */
	
	/* Free the cave */
	FREE(cave_o_idx);
	FREE(cave_m_idx);
	FREE(cave_feat);
	FREE(cave_info);
	
	/* Free the "update_view()" array */
	FREE(view_g);
	
	/* Free the temp array */
	FREE(temp_g);
	
	/* Free the messages */
	messages_free();
	
	/* Free the "quarks" */
	quarks_free();
	
	/* Free the info, name, and text arrays */
	free_info(&flavor_head);
	free_info(&g_head);
	free_info(&b_head);
	free_info(&c_head);
	free_info(&p_head);
	free_info(&h_head);
	free_info(&v_head);
	free_info(&r_head);
	free_info(&e_head);
	//free_info(&a_head);
	free_info(&k_head);
	free_info(&f_head);
	free_info(&z_head);
	free_info(&s_head);
	
	/* Free the format() buffer */
	vformat_kill();
	
	/* Free the directories */
	string_free(ANGBAND_DIR);
	string_free(ANGBAND_DIR_APEX);
	string_free(ANGBAND_DIR_BONE);
	string_free(ANGBAND_DIR_DATA);
	string_free(ANGBAND_DIR_EDIT);
	string_free(ANGBAND_DIR_FILE);
	string_free(ANGBAND_DIR_HELP);
	string_free(ANGBAND_DIR_INFO);
	string_free(ANGBAND_DIR_SAVE);
	string_free(ANGBAND_DIR_PREF);
	string_free(ANGBAND_DIR_USER);
	string_free(ANGBAND_DIR_XTRA);
}
