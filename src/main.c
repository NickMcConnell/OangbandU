/* File: main.c */

/*
 * Copyright (c) 1997 Ben Harrison, and others
 *
 * This software may be copied and distributed for educational, research,
 * and not for profit purposes provided that this copyright and statement
 * are included in all such copies.
 */

#include "angband.h"


/*
 * Some machines have a "main()" function in their "main-xxx.c" file,
 * all the others use this file for their "main()" function.
 */


#if !defined(MACINTOSH) && !defined(WINDOWS) && !defined(ACORN)

#ifdef USE_SCRIPT

#include "Python.h"

#endif /* USE_SCRIPT */

#include "main.h"

/*
 * List of the available modules in the order they are tried.
 */
static const struct module modules[] =
{
#ifdef USE_SDL
	{ "sdl", help_sdl, init_sdl },
#endif /* USE_SDL */

#ifdef USE_GTK
	{ "gtk", help_gtk, init_gtk },
#endif /* USE_GTK */

#ifdef USE_XAW
	{ "xaw", help_xaw, init_xaw },
#endif /* USE_XAW */

#ifdef USE_X11
	{ "x11", help_x11, init_x11 },
#endif /* USE_X11 */

#ifdef USE_GCU
	{ "gcu", help_gcu, init_gcu },
#endif /* USE_GCU */

#ifdef USE_CAP
	{ "cap", help_cap, init_cap },
#endif /* USE_CAP */
	    
#ifdef USE_DOS
	{ "dos", help_dos, init_dos },
#endif /* USE_DOS */
	    
#ifdef USE_IBM
	{ "ibm", help_ibm, init_ibm }, 
#endif /* USE_IBM */
	    
#ifdef USE_SLA
	{ "sla", help_sla, init_sla }, 
#endif /* USE_SLA */
	    
#ifdef USE_LSL
	{ "lsl", help_lsl, init_lsl }, 
#endif /* USE_LSL */
	    
#ifdef USE_AMI
	{ "ami", help_ami, init_ami }, 
#endif /* USE_AMI */
	    
#ifdef USE_VME
	{ "vme", help_vme, init_vme }, 
#endif /* USE_VME */
};

/*
 * List of sound modules in the order they should be tried.
 */
static const struct module sound_modules[] =
{
#ifdef SOUND_SDL
  { "sdl", "SDL_mixer sound module", init_sound_sdl },
#endif /* SOUND_SDL */
  
  { "dummy", "Dummy module", NULL },
};

#endif


/*
 * A hook for "quit()".
 *
 * Close down, then fall back into "quit()".
 */
static void quit_hook(cptr s)
{
  int j;
  
  /* Scan windows */
  for (j = TERM_WIN_MAX - 1; j >= 0; j--)
    {
      /* Unused */
      if (!angband_term[j]) continue;
      
      /* Nuke it */
      term_nuke(angband_term[j]);
    }
}



/*
 * Set the stack size (for the Amiga)
 */
#ifdef AMIGA
# include <dos.h>
__near long __stack = 32768L;
#endif


/*
 * SDL needs a look-in
 */
#ifdef USE_SDL
# include "SDL.h"
#endif


/*
 * Set the stack size and overlay buffer (see main-286.c")
 */
#ifdef USE_286
# include <dos.h>
extern unsigned _stklen = 32768U;
extern unsigned _ovrbuffer = 0x1500;
#endif

/*
 * Initialize and verify the file paths, and the score file.
 *
 * Use the ANGBAND_PATH environment var if possible, else use
 * DEFAULT_PATH, and in either case, branch off appropriately.
 *
 * First, we'll look for the ANGBAND_PATH environment variable,
 * and then look for the files in there.  If that doesn't work,
 * we'll try the DEFAULT_PATH constant.  So be sure that one of
 * these two things works...
 *
 * We must ensure that the path ends with "PATH_SEP" if needed,
 * since the "init_file_paths()" function will simply append the
 * relevant "sub-directory names" to the given path.
 *
 * Note that the "path" must be "Angband:" for the Amiga, and it
 * is ignored for "VM/ESA", so I just combined the two.
 *
 * Make sure that the path doesn't overflow the buffer.  We have
 * to leave enough space for the path separator, directory, and
 * filenames.
 */
static void init_stuff(void)
{
  char path[1024];
  
#if defined(AMIGA) || defined(VM)
  
  /* Hack -- prepare "path" */
  strcpy(path, "Angband:");
  
#else /* AMIGA / VM */
  
  cptr tail;
  
#ifdef _WIN32_WCE
  tail = NULL;
#else
  /* Get the environment variable */
  tail = getenv("ANGBAND_PATH");
#endif  

  /* Use the angband_path, or a default */
  strncpy(path, tail ? tail : DEFAULT_PATH, 511);

  
  /* Make sure it's terminated */
  path[511] = '\0';
  
  /* Hack -- Add a path separator (only if needed) */
  if (!suffix(path, PATH_SEP)) strcat(path, PATH_SEP);
  
#endif /* AMIGA / VM */
  
  /* Initialize */
  init_file_paths(path);
}



/*
 * Handle a "-d<what>=<path>" option
 *
 * The "<what>" can be any string starting with the same letter as the
 * name of a subdirectory of the "lib" folder (i.e. "i" or "info").
 *
 * The "<path>" can be any legal path for the given system, and should
 * not end in any special path separator (i.e. "/tmp" or "~/.ang-info").
 */
static void change_path(cptr info)
{
  cptr s;
  
  /* Find equal sign */
  s = strchr(info, '=');
  
  /* Verify equal sign */
  if (!s) quit_fmt("Try '-d<what>=<path>' not '-d%s'", info);
  
  /* Analyze */
  switch (tolower(info[0]))
    {
    case 'a':
      {
	string_free(ANGBAND_DIR_APEX);
	ANGBAND_DIR_APEX = string_make(s+1);
	break;
      }
      
    case 'f':
      {
	string_free(ANGBAND_DIR_FILE);
	ANGBAND_DIR_FILE = string_make(s+1);
	break;
      }
      
    case 'h':
      {
	string_free(ANGBAND_DIR_HELP);
	ANGBAND_DIR_HELP = string_make(s+1);
	break;
      }
      
    case 'i':
      {
	string_free(ANGBAND_DIR_INFO);
	ANGBAND_DIR_INFO = string_make(s+1);
	break;
      }
      
    case 'u':
      {
	string_free(ANGBAND_DIR_USER);
	ANGBAND_DIR_USER = string_make(s+1);
	break;
      }
      
    case 'x':
      {
	string_free(ANGBAND_DIR_XTRA);
	ANGBAND_DIR_XTRA = string_make(s+1);
	break;
      }
      
#ifdef VERIFY_SAVEFILE
      
    case 'b':
    case 'd':
    case 'e':
    case 's':
      {
	quit_fmt("Restricted option '-d%s'", info);
      }
      
#else /* VERIFY_SAVEFILE */
      
    case 'b':
      {
	string_free(ANGBAND_DIR_BONE);
	ANGBAND_DIR_BONE = string_make(s+1);
	break;
      }
      
    case 'd':
      {
	string_free(ANGBAND_DIR_DATA);
	ANGBAND_DIR_DATA = string_make(s+1);
	break;
      }
      
    case 'e':
      {
	string_free(ANGBAND_DIR_EDIT);
	ANGBAND_DIR_EDIT = string_make(s+1);
	break;
      }
      
    case 's':
      {
	string_free(ANGBAND_DIR_SAVE);
	ANGBAND_DIR_SAVE = string_make(s+1);
	break;
      }
      
    case 'z':
      {
	string_free(ANGBAND_DIR_SCRIPT);
	ANGBAND_DIR_SCRIPT = string_make(s+1);
	break;
      }
      
#endif /* VERIFY_SAVEFILE */
      
    default:
      {
	quit_fmt("Bad semantics in '-d%s'", info);
      }
    }
}


/*
 * Simple "main" function for multiple platforms.
 *
 * Note the special "--" option which terminates the processing of
 * standard options.  All non-standard options (if any) are passed
 * directly to the "init_xxx()" function.
 */
int main(int argc, char *argv[])
{
  int i, j;
  
  bool done = FALSE;
  
  bool new_game = FALSE;
  
  int show_score = 0;
  
  cptr mstr = NULL;
  
  bool args = TRUE;
  
  
  /* Save the "program name" XXX XXX XXX */
  argv0 = argv[0];
  
#ifdef USE_286
  /* Attempt to use XMS (or EMS) memory for swap space */
  if (_OvrInitExt(0L, 0L))
    {
      _OvrInitEms(0, 0, 64);
    }
#endif
  

#ifdef SET_UID

  /* Default permissions on files */
  (void)umask(022);
  
# ifdef SECURE
  /* Authenticate */
  Authenticate();
# endif

#endif


  /* Get the file paths */
  init_stuff();


#ifdef SET_UID

  /* Get the user id (?) */
  player_uid = getuid();

#ifdef VMS
  /* Mega-Hack -- Factor group id */
  player_uid += (getgid() * 1000);
#endif

# ifdef SAFE_SETUID

#  ifdef _POSIX_SAVED_IDS

  /* Save some info for later */
  player_euid = geteuid();
  player_egid = getegid();
  
#  endif

#  if 0	/* XXX XXX XXX */

	/* Redundant setting necessary in case root is running the game */
	/* If not root or game not setuid the following two calls do nothing */
  
  if (setgid(getegid()) != 0)
    {
      quit("setgid(): cannot set permissions correctly!");
    }
  
  if (setuid(geteuid()) != 0)
    {
      quit("setuid(): cannot set permissions correctly!");
    }

#  endif
  
# endif

#endif


#ifdef SET_UID

  /* Initialize the "time" checker */
  if (check_time_init() || check_time())
    {
      quit("The gates to Angband are closed (bad time).");
    }
  
  /* Acquire the "user name" as a default player name */
#ifdef ANGBAND_2_8_1
  user_name(player_name, player_uid);
#else /* ANGBAND_2_8_1 */
  user_name(op_ptr->full_name, sizeof(op_ptr->full_name), player_uid);
#endif /* ANGBAND_2_8_1 */
  
#ifdef PRIVATE_USER_PATH

  /* Create directories for the users files */
  create_user_dirs();

#endif /* PRIVATE_USER_PATH */

#endif /* SET_UID */
  
  
  /* Process the command line arguments */
  for (i = 1; args && (i < argc); i++)
    {
      /* Require proper options */
      if (argv[i][0] != '-') goto usage;
      
      /* Analyze option */
      switch (argv[i][1])
	{
	case 'N':
	case 'n':
	  {
	    new_game = TRUE;
	    break;
	  }
	  
	case 'F':
	case 'f':
	  {
	    arg_fiddle = TRUE;
	    break;
	  }
	  
	case 'W':
	case 'w':
	  {
	    arg_wizard = TRUE;
	    break;
	  }
	  
	case 'V':
	case 'v':
	  {
	    arg_sound = TRUE;
	    break;
	  }
	  
	case 'G':
	case 'g':
	  {
	    /* Default graphics tile */
	    arg_graphics = GRAPHICS_ADAM_BOLT;
	    break;
	  }
	  
	case 'R':
	case 'r':
	  {
	    arg_force_roguelike = TRUE;
	    break;
	  }
	  
	case 'O':
	case 'o':
	  {
	    arg_force_original = TRUE;
	    break;
	  }
	  
	case 'L':
	case 'l':
	  {
	    small_screen = TRUE;
	    break;
	  }
	  
	case 'S':
	case 's':
	  {
	    show_score = atoi(&argv[i][2]);
	    if (show_score <= 0) show_score = 10;
	    break;
	  }
	  
	case 'u':
	case 'U':
	  {
	    if (!argv[i][2]) goto usage;
#ifdef ANGBAND_2_8_1
	    strncpy(player_name, &argv[i][2], 32);
	    player_name[31] = '\0';
#else /* ANGBAND_2_8_1 */
	    strncpy(op_ptr->full_name, &argv[i][2], 32);
	    op_ptr->full_name[31] = '\0';
#endif /* ANGBAND_2_8_1 */
	    break;
	  }
	  
	case 'm':
	  {
	    if (!argv[i][2]) goto usage;
	    mstr = &argv[i][2];
	    break;
	  }
	  
	case 'M':
	  {
	    arg_monochrome = TRUE;
	    break;
	  }
	  
	case 'd':
	case 'D':
	  {
	    change_path(&argv[i][2]);
	    break;
	  }
	  
	case '-':
	  {
	    argv[i] = argv[0];
	    argc = argc - i;
	    argv = argv + i;
	    args = FALSE;
	    break;
	  }
	  
	default:
	usage:
	  {
	    /* Dump usage information */
	    puts("Usage: angband [options] [-- subopts]");
	    puts("  -n       Start a new character");
	    puts("  -f       Request fiddle mode");
	    puts("  -w       Request wizard mode");
	    puts("  -v       Request sound mode");
	    puts("  -g       Request graphics mode");
	    puts("  -o       Request original keyset");
	    puts("  -r       Request rogue-like keyset");
	    puts("  -l       Request small screen mode");
	    puts("  -M       Request monochrome mode");
	    puts("  -s<num>  Show <num> high scores");
	    puts("  -u<who>  Use your <who> savefile");
	    puts("  -d<def>  Define a 'lib' dir sub-path");
	    puts("  -m<sys>  use Module <sys>, where <sys> can be:");

#ifndef _WIN32_WCE	    
	    /* Print the name and help for each available module */
	    for (j = 0; j < (int)N_ELEMENTS(modules); j++)
	      {
		printf("     %s   %s\n",
		       modules[j].name, modules[j].help);
	      }
#endif
	    
	    /* Actually abort the process */
	    quit(NULL);
	  }
	}
    }
  
  /* Hack -- Forget standard args */
  if (args)
    {
      argc = 1;
      argv[1] = NULL;
    }
  
  
  /* Process the player name */
  process_player_name(TRUE);
  
  
  
  /* Install "quit" hook */
  quit_aux = quit_hook;
  
  
  /* Drop privs (so X11 will work correctly), unless we are running */
  /* the Linux-SVGALib version. */
#ifndef USE_LSL
  safe_setuid_drop();
#endif
  
  
  /* Grab privs (dropped above for X11) */
#ifndef USE_LSL
  safe_setuid_grab();
#endif
  
#ifndef _WIN32_WCE  
  /* Try the modules in the order specified by modules[] */
  for (i = 0; i < (int)N_ELEMENTS(modules); i++)
    {
      /* User requested a specific module? */
      if (!mstr || (streq(mstr, modules[i].name)))
	{
	  if (0 == modules[i].init(argc, argv))
	    {
	      ANGBAND_SYS = modules[i].name;
	      done = TRUE;
	      break;
	    }
	}
    }
  
  /* Make sure we have a display! */
  if (!done) quit("Unable to prepare any 'display module'!");
#endif

  /* Catch nasty signals */
  signals_init();
  
      /* Initialize */
      init_angband();
      
      /* Hack -- If requested, display scores and quit */
      if (show_score > 0) 
	{
	  display_scores(0, show_score);
	}
      
      /* Wait for response */
      pause_line(23);
      
      /* Play the game */
      play_game(new_game);
      
      /* Free resources */
      cleanup_angband();

  /* Quit */
  quit(NULL);
  
  /* Exit */
  return (0);
}



