/* File: main-nds.c */

/*
  Immir's TODO list:

  Figure out how the palettes work to make the cursor inverse-video
  somehow.

  Add translucent top layer with a directional pad bottom right.

  Bind "B" or "Y" to repeat?

  Add customizable bindings.

  Add graphical buttons down right-hand side. Perhaps initially just 
  triggering the F1 through F12 keys (so macro bindings can be triggered
  quickly).
  
  Add more terms, and provide sub-screen term setup a la the X11 port.

  Hack the status display to allow it to be gobbled by a different term
  rather than just main.

*/
  

/* Purpose: Main file for playing on the Nintendo DS */

/*
 * Original version by Nick McConnell
 * Improvements/extensions by Michael 'Immir' Smith
 * Various DS stuff borrowed from http://frodo.dyn.gno.org/~brettk/NetHackDS.
 *
 * Template main-xxx.c by Ben Harrison (benh@phial.com).
 *
 */

#define MJS_TEST

#include <main-nds.h>

#include <main.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

//#include <nds/subpix.h>
#include <nds/tiles.h>
#include <nds/buttons.h>
#include <nds/kbd.h>
#include <nds/misc.h>
#include <nds/font-bdf.h>
#include <nds/text.h>

// flag for lid-closed
int power_state = 0;

int TILE_WIDTH;
int TILE_HEIGHT;

#define UNUSED __attribute__((unused))

/*
 * Extra data to associate with each "window"
 *
 * Each "window" is represented by a "term_data" structure, which
 * contains a "term" structure, which contains a pointer (t->data)
 * back to the term_data structure.
 */

typedef struct term_data term_data;

struct term_data
{
  term t;
  byte num;
  byte rows;
  byte cols;
  int  tile_width; 
  int  tile_height;
  struct font *font;
  u16b *frame_buffer;
};

struct font *normal_font;
struct font *zoom_font;

void nds_log(const char *fmt, ...) {
  char p[256];
  va_list ap;
  va_start(ap, fmt);
  (void) vsnprintf(p, 256, fmt, ap);
  va_end(ap);
  printf(p);// to console
}


void nds_fatal_err(const char* msg) {
  nds_log(msg);
  nds_show_console();
  exit(1);
}

/*
 * Number of "term_data" structures to support XXX XXX XXX
 *
 * You MUST support at least one "term_data" structure, and the
 * game will currently use up to eight "term_data" structures if
 * they are available.
 *
 * If only one "term_data" structure is supported, then a lot of
 * the things that would normally go into a "term_data" structure
 * could be made into global variables instead.
 */
#define MAX_TERM_DATA 2


/*
 * An array of "term_data" structures, one for each "sub-window"
 */
static term_data data[MAX_TERM_DATA];

/*
 * Colour data
 */

u16b color_data[] = {
	RGB15(  0,  0,  0), 		/* TERM_DARK */
	RGB15( 31, 31, 31), 		/* TERM_WHITE */
	RGB15( 15, 15, 15), 		/* TERM_SLATE */
	RGB15( 31, 15,  0),		/* TERM_ORANGE */ 
	RGB15( 23,  0,  0), 		/* TERM_RED */
	RGB15(  0, 15,  9), 		/* TERM_GREEN */
	RGB15(  0,  0, 31), 		/* TERM_BLUE */
	RGB15( 15,  9,  0), 		/* TERM_UMBER */
	RGB15(  9,  9,  9), 		/* TERM_L_DARK */
	RGB15( 23, 23, 23), 		/* TERM_L_WHITE */
	RGB15( 31,  0, 31), 		/* TERM_VIOLET */
	RGB15( 31, 31,  0), 		/* TERM_YELLOW */
	RGB15( 31,  0,  0), 		/* TERM_L_RED */
	RGB15(  0, 31,  0), 		/* TERM_L_GREEN */
	RGB15(  0, 31, 31), 		/* TERM_L_BLUE */
	RGB15( 23, 15,  9)		/* TERM_L_UMBER */
};



/*** Function hooks needed by "Term" ***/


/*
 * Init a new "term"
 *
 * This function should do whatever is necessary to prepare a new "term"
 * for use by the "term.c" package.  This may include clearing the window,
 * preparing the cursor, setting the font/colors, etc.  Usually, this
 * function does nothing, and the "init_xxx()" function does it all.
 */
static void Term_init_nds(term *t)
{
  term_data *td UNUSED = (term_data*)(t->data);

  /* XXX XXX XXX */
}



/*
 * Nuke an old "term"
 *
 * This function is called when an old "term" is no longer needed.  It should
 * do whatever is needed to clean up before the program exits, such as wiping
 * the screen, restoring the cursor, fixing the font, etc.  Often this function
 * does nothing and lets the operating system clean up when the program quits.
 */
static void Term_nuke_nds(term *t)
{
  term_data *td UNUSED = (term_data*)(t->data);

  /* XXX XXX XXX */
}



/*
 * Do a "user action" on the current "term"
 *
 * This function allows the visual module to do implementation defined
 * things when the user activates the "system defined command" command.
 *
 * This function is normally not used.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_user_nds(int n)
{
  term_data *td UNUSED = (term_data*)(Term->data);
  /* XXX XXX XXX */
  /* Unknown */
  return (1);
}



/*
 * Find the square a particular pixel is part of.
 */
static void pixel_to_square(int * const x, int * const y,
			    const int ox, const int oy) {
  (*x) = ox / TILE_WIDTH;
  (*y) = oy / TILE_HEIGHT;
}


int map_top = 0;
void window_swap () {
  map_top ^= 1;
  lcdSwap();
}


/*
 * All event handling 
 * MJS: Where did this stuff come from? Do we really need this stuff?
 */
struct event_s { u16b flags; u16b data; } ebuf[MAX_EBUF];
u16b ebuf_read = 0, ebuf_write = 0;

bool has_event() {
  return ((ebuf[ebuf_read].flags)||(ebuf_read != ebuf_write));
}

void get_event(u16b * flags, u16b * data) {
  *flags = 0; *data = 0;
  if (!has_event()) return;
  *flags = ebuf[ebuf_read].flags;
  *data  = ebuf[ebuf_read].data;
  ebuf[ebuf_read].flags = 0;
  ebuf[ebuf_read].data  = 0;
  ebuf_read++;
  if (ebuf_read > ebuf_write) {
    ebuf_write++;
    if (ebuf_write >= MAX_EBUF) ebuf_write = 0;
  }
  if (ebuf_read >= MAX_EBUF) ebuf_read = 0;
}

void put_key_event(byte c) {
  ebuf[ebuf_write].flags = KEVENT_FLAG;
  ebuf[ebuf_write].data  = (u16) c;
  ++ebuf_write;
  if (ebuf_write >= MAX_EBUF) ebuf_write = 0;
}

void put_mouse_event(byte x, byte y) {
  ebuf[ebuf_write].flags = MEVENT_FLAG;
  ebuf[ebuf_write].data  = (((u16b)x)<<8)|((u16b)y);
  ++ebuf_write;
  if (ebuf_write >= MAX_EBUF) ebuf_write = 0;
}


/*
 * Handle a touch on the touch screen.
 */
static void handle_touch(int x, int y, int button, bool press) {
  /* The co-ordinates are only used in Angband format. */
  pixel_to_square(&x, &y, x, y);
  Term_mousepress(x, y, (char)button);
}


void do_map_touch () {
  static u16b touched = 0;// frames the stylus has been held down for
  static s16b xarr[3],yarr[3];
  
  // if screen is being touched...
  if (keysHeld() & KEY_TOUCH) {
    if (touched < 3) {
      xarr[touched] = IPC->touchXpx;	// add this to the array for
      yarr[touched] = IPC->touchYpx;	// finding the median
      touched++;			// add to counter
    }
  } else 
    touched = 0;	// so reset the counter for next time
  
  // if the screen has been touched for 3 frames...
  if (touched == 3) {
    touched++; // don't send another event
    // also, not setting to zero prevents the keysHeld() thing
    //  from starting the process over and getting 3 more samples
      
    u16b i, tmp, x=0, y=0;
      
    // x/yarr now contains 3 values from each of the 3 frames
    // take the median of each array and put into the_x/y
      
    // sort the array
    // bubble sort, ugh
    for (i = 1; i < 3; i++) {
      if (xarr[i] < xarr[i-1]) {
	tmp = xarr[i];
	xarr[i] = xarr[i-1];
	xarr[i-1] = tmp;
      }
      if (yarr[i] < yarr[i-1]) {
	tmp = yarr[i];
	yarr[i] = yarr[i-1];
	yarr[i-1] = tmp;
      }
    }
    
    // get the middle value (median)
    // if it's -1, take the top value
    if (xarr[1] == -1) x = xarr[2];
    else x = xarr[1];
    if (yarr[1] == -1) y = yarr[2];
    else y = yarr[1];

    put_mouse_event(x,y);

  }
}


void do_vblank() {

  while (power_state == POWER_STATE_ASLEEP)
    swiWaitForVBlank();

  swiWaitForVBlank();

  // ---------------------------
  //  Handle the arrow buttons
  scanKeys();
  u32b kd = keysDown();
  u32b kr = keysDownRepeat();
  u32b kh = keysHeld();
  u32b ku = keysUp();

  if (kr & (KEY_RIGHT | KEY_LEFT | KEY_UP | KEY_DOWN)) {
    int i, key;
    u32b kk = kh|kr; // allow diags once a key starts repeating
    int mx = kk & KEY_RIGHT ? 1 : kk & KEY_LEFT ? -1 : 0;
    int my = kk & KEY_UP    ? 1 : kk & KEY_DOWN ? -1 : 0;
    key = '5' + mx + 3*my;
    put_key_event(key);
  }
  
  /* MJS HACK */
  if (kd & KEY_L) {
    window_swap(); 
    SUB_DISPLAY_CR ^= DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE;
    return;
  }
  if (kd & KEY_R) {
#if 1
    // MJS: Experimental zooming mode...
    term_data *td = &data[0];
    int minwd,avgwd,maxwd,ht;
    struct font *font;
    font = (td->font == normal_font) ? zoom_font : normal_font;
    bdf_dims(font, &minwd, &avgwd, &maxwd, &ht);
    td->font = font;
    td->cols = 256/avgwd;
    td->rows = 192/ht;
    td->tile_width = avgwd;
    td->tile_height = ht;
    TILE_WIDTH = avgwd;
    TILE_HEIGHT = ht;
    { term *old = Term; // do I need to worry about this??
      Term_activate(&td->t);
      p_ptr->redraw |= ~0; // mark everything as needed for update
      Term_resize(td->cols, td->rows);
      verify_panel(); // center on player and...
      handle_stuff(); // ...do various updates
      Term_redraw();  // actually redraw?
      Term_flush();   // flush events? needed?
      Term_activate(old); // necessary??
    }
#else    
    nds_show_console();
#endif
    return;
  }

  if (kd & KEY_Y) {
    put_key_event('y');
    return;
  }

  if (kr & KEY_B) {
    // HACK: attack/move towards nearest visible monster
    int py=p_ptr->py, px=p_ptr->px, my, mx, key;
    // TODO: better way than this? Get prototype for this...
    get_closest_los_monster(1, py, px, &my, &mx, TRUE);
    if (mx==0 && my==0) {
      put_key_event(','); // rest
      return;
    }
    key =  px>mx ? '4' : px==mx ? '5' : '6';
    key += py<my ? -3  : py==my ?  0  :  3;
    put_key_event(key);
    return;
  }

  // ---------------------------
  //  Check for button macros
  nds_check_buttons(kd, kh); // TODO: kr == repeat??
  
  if (map_top) {

    // ---------------------------
    //  Check for typing on the touchscreen kbd
    byte keycode = kbd_vblank();
    if ((keycode & 0x7F) != 0) {	// it's an actual keystroke, return it
      put_key_event(keycode & 0xFF);
    }

  } else {

    do_map_touch();

  }

}

//END JUST MOVED

/*
 * An event handler XXX XXX XXX
 *
 * You may need an event handler, which can be used by both
 * by the "TERM_XTRA_BORED" and "TERM_XTRA_EVENT" entries in
 * the "Term_xtra_xxx()" function, and also to wait for the
 * user to perform whatever user-interface operation is needed
 * to request the start of a new game or the loading of an old
 * game, both of which should launch the "play_game()" function.
 */
static errr CheckEvents(bool wait)
{
  u16b e = 0;
  u16b data;

  do_vblank();

  if (!wait && !has_event()) return (1);

  while (!e) {
    get_event(&e,&data);
    do_vblank();
  }

  /* Mouse */
  if (e == MEVENT_FLAG)
    handle_touch(data>>8, data&0xff, 1, TRUE);

  /* Undefined */
  else if ((data & 0x7F) == 0)
    return (1);

  /* Key */
  else
    Term_keypress(data & 0xff);

  return (0);
}


/*
 * Do a "special thing" to the current "term"
 *
 * This function must react to a large number of possible arguments, each
 * corresponding to a different "action request" by the "z-term.c" package,
 * or by the application itself.
 *
 * The "action type" is specified by the first argument, which must be a
 * constant of the form "TERM_XTRA_*" as given in "term.h", and the second
 * argument specifies the "information" for that argument, if any, and will
 * vary according to the first argument.
 *
 * In general, this function should return zero if the action is successfully
 * handled, and non-zero if the action is unknown or incorrectly handled.
 */
static errr Term_xtra_nds(int n, int v)
{
  term_data *td UNUSED = (term_data*)(Term->data);

  /* Analyze */
  switch (n)
    {
    case TERM_XTRA_EVENT:
      {
	/*
	 * Process some pending events 
	 */
	return (CheckEvents(v));
      }
      
    case TERM_XTRA_FLUSH:
      {
	/*
	 * Flush all pending events 
	 */
	while (!CheckEvents(FALSE)); 
	
	return (0);
      }
      
    case TERM_XTRA_CLEAR:
      {
	/*
	 * Clear the entire window 
	 */
	u16b *fb = td->frame_buffer;
	int i;

	for (i = 0; i < 256 * 192 / 2; i++)
	  fb[i] = 0;

	return (0);
      }
      
    case TERM_XTRA_SHAPE:
      {
	/*
	 * Set the cursor visibility XXX XXX XXX
	 *
	 * This action should change the visibility of the cursor,
	 * if possible, to the requested value (0=off, 1=on)
	 *
	 * This action is optional, but can improve both the
	 * efficiency (and attractiveness) of the program.
	 */
	
	return (0);
      }
      
    case TERM_XTRA_FROSH:
      {
	return (0);
      }
      
    case TERM_XTRA_FRESH:
      {
	return (0);
      }
      
    case TERM_XTRA_NOISE:
      {
	/*
	 * Make a noise XXX XXX XXX
	 *
	 * This action should produce a "beep" noise.
	 *
	 * This action is optional, but convenient.
	 */
	
	return (0);
      }
      
    case TERM_XTRA_SOUND:
      {
	/*
	 * Make a sound XXX XXX XXX
	 *
	 * This action should produce sound number "v", where the
	 * "name" of that sound is "sound_names[v]".  This method
	 * is still under construction.
	 *
	 * This action is optional, and not very important.
	 */
	
	return (0);
      }
      
    case TERM_XTRA_BORED:
      {
	/*
	 * Handle random events when bored 
	 */
	return (CheckEvents(0));
      }
      
    case TERM_XTRA_REACT:
      {
	/*
	 * React to global changes XXX XXX XXX
	 *
	 * For example, this action can be used to react to
	 * changes in the global "color_table[256][4]" array.
	 *
	 * This action is optional, but can be very useful for
	 * handling "color changes" and the "arg_sound" and/or
	 * "arg_graphics" options.
	 */
	
	return (0);
      }
      
    case TERM_XTRA_ALIVE:
      {
	/*
	 * Change the "hard" level XXX XXX XXX
	 *
	 * This action is used if the program changes "aliveness"
	 * by being either "suspended" (v=0) or "resumed" (v=1)
	 * This action is optional, unless the computer uses the
	 * same "physical screen" for multiple programs, in which
	 * case this action should clean up to let other programs
	 * use the screen, or resume from such a cleaned up state.
	 *
	 * This action is currently only used by "main-gcu.c",
	 * on UNIX machines, to allow proper "suspending".
	 */
	
	return (0);
      }
      
    case TERM_XTRA_LEVEL:
      {
	/*
	 * Change the "soft" level XXX XXX XXX
	 *
	 * This action is used when the term window changes "activation"
	 * either by becoming "inactive" (v=0) or "active" (v=1)
	 *
	 * This action can be used to do things like activate the proper
	 * font / drawing mode for the newly active term window.  This
	 * action should NOT change which window has the "focus", which
	 * window is "raised", or anything like that.
	 *
	 * This action is optional if all the other things which depend
	 * on what term is active handle activation themself, or if only
	 * one "term_data" structure is supported by this file.
	 */
	
	return (0);
      }
      
    case TERM_XTRA_DELAY:
      {
	/*
	 * Delay for some milliseconds 
	 */
	int i;
	for (i = 0; i < v; i++)
	  swiWaitForVBlank();
	
	return (0);
      }
    }
  
  /* Unknown or Unhandled action */
  return (1);
}


/*
 * Display the cursor
 */
static errr Term_curs_nds(int x, int y)
{
  term_data *td UNUSED = (term_data*)(Term->data);
  u16b *fb = td->frame_buffer;
  struct font *font = td->font;

  if (y >= td->rows) return (0);
  if (x >= td->cols) return (0);
  draw_color_char(x*td->tile_width, y*td->tile_height, 32, 0, 1, fb,
		  font);

  /* Success */
  return (0);
}


/*
 * Erase some characters
 *
 * This function should erase "n" characters starting at (x,y).
 *
 * You may assume "valid" input if the window is properly sized.
 */
static errr Term_wipe_nds(int x, int y, int n) {
  term_data *td UNUSED = (term_data*)(Term->data);
  u16b *fb = td->frame_buffer;
  struct font *font = td->font;
  int tile_width = td->tile_width;
  int tile_height = td->tile_height;

  int i;

  if (y >= td->rows) return (0);

  for (i = 0; i < n; i++)
    if (x + i < td->cols)
      draw_color_char((x + i)*tile_width, y*tile_height, 32, 0, -1, fb,
		      font);
  
  /* Success */
  return (0);
}


/*
 * Draw some text on the screen
 *
 * This function should actually display an array of characters
 * starting at the given location, using the given "attribute",
 * and using the given string of characters, which contains
 * exactly "n" characters and which is NOT null-terminated.
 *
 * You may assume "valid" input if the window is properly sized.
 *
 * You must be sure that the string, when written, erases anything
 * (including any visual cursor) that used to be where the text is
 * drawn.  On many machines this happens automatically, on others,
 * you must first call "Term_wipe_xxx()" to clear the area.
 *
 * In color environments, you should activate the color contained
 * in "color_data[a & 0x0F]", if needed, before drawing anything.
 *
 * You may ignore the "attribute" if you are only supporting a
 * monochrome environment, since this routine is normally never
 * called to display "black" (invisible) text, including the
 * default "spaces", and all other colors should be drawn in
 * the "normal" color in a monochrome environment.
 *
 * Note that if you have changed the "attr_blank" to something
 * which is not black, then this function must be able to draw
 * the resulting "blank" correctly.
 *
 * Note that this function must correctly handle "black" text if
 * the "always_text" flag is set, if this flag is not set, all the
 * "black" text will be handled by the "Term_wipe_xxx()" hook.
 */
static errr Term_text_nds(int x, int y, int n, byte a, const char *cp)
{
  term_data *td UNUSED = (term_data*)(Term->data);
  u16b *fb = td->frame_buffer;
  struct font *font = td->font;
  int tile_width = td->tile_width;
  int tile_height = td->tile_height;
  int i;
  
  /* Do nothing if the string is null */
  if (!cp || !*cp) return (-1);
  
  /* Get the length of the string */
  if ((n > strlen(cp)) || (n < 0)) n = strlen(cp);

  /* Put the characters directly */
  for (i = 0; i < n && *cp; i++) {
      /* Check it's the right attr */
      if ((x + i < Term->wid) && (Term->scr->a[y][x + i] == a))
	/* Put the char */
	draw_color_char((x + i)*tile_width, y*tile_height, *cp++, a, -1, fb,
			font);
      else 
	break;
    }
  /* Success */
  return (0);
}

/*
 * Draw some attr/char pairs on the screen
 *
 * This routine should display the given "n" attr/char pairs at
 * the given location (x,y).  This function is only used if one
 * of the flags "always_pict" or "higher_pict" is defined.
 *
 * You must be sure that the attr/char pairs, when displayed, will
 * erase anything (including any visual cursor) that used to be at
 * the given location.  On many machines this is automatic, but on
 * others, you must first call "Term_wipe_xxx(x, y, 1)".
 *
 * With the "higher_pict" flag, this function can be used to allow
 * the display of "pseudo-graphic" pictures, for example, by using
 * the attr/char pair as an encoded index into a pixmap of special
 * "pictures".
 *
 * With the "always_pict" flag, this function can be used to force
 * every attr/char pair to be drawn by this function, which can be
 * very useful if this file can optimize its own display calls.
 *
 * This function is often associated with the "arg_graphics" flag.
 *
 * This function is only used if one of the "higher_pict" and/or
 * "always_pict" flags are set.
 */
static errr Term_pict_nds(int x, int y, int n, const byte *ap, const char *cp,
  const byte *tap, const char *tcp)

{
  term_data *td UNUSED = (term_data*)(Term->data);
  
  return (0);
}


/*** Internal Functions ***/

/*
 * Instantiate a "term_data" structure
 *
 * This is one way to prepare the "term_data" structures and to
 * "link" the various informational pieces together.
 *
 * This function assumes that every window should be 80x24 in size
 * (the standard size) and should be able to queue 256 characters.
 * Technically, only the "main screen window" needs to queue any
 * characters, but this method is simple.  One way to allow some
 * variation is to add fields to the "term_data" structure listing
 * parameters for that window, initialize them in the "init_xxx()"
 * function, and then use them in the code below.
 *
 * Note that "activation" calls the "Term_init_xxx()" hook for
 * the "term" structure, if needed.
 */

static void term_data_link(int i) {
  term_data *td UNUSED = &data[i];
  
  term *t = &td->t;
  
  /* Initialize the term */
  term_init(t, td->cols, td->rows, 256);

  t->soft_cursor = TRUE;
  t->never_bored = TRUE;
  
  /* Prepare the init/nuke hooks */
  t->init_hook = Term_init_nds;
  t->nuke_hook = Term_nuke_nds;
  
  /* Prepare the template hooks */
  t->user_hook = Term_user_nds;
  t->xtra_hook = Term_xtra_nds;
  t->curs_hook = Term_curs_nds;
  t->wipe_hook = Term_wipe_nds;
  t->text_hook = Term_text_nds;
  t->pict_hook = Term_pict_nds;

  /* Remember where we came from */
  t->data = (vptr)(td);
  
  /* Activate it */
  Term_activate(t);

}



/*
 * Initialization function
 */
errr init_nds(void) {
  /* Initialize globals */

  /* Initialize "term_data" structures */
  
  int i;
  bool none = TRUE;
  
  term_data *td;

  /* Main window */
  td = &data[0];
  WIPE(td, term_data);
  td->num  = 0;
  td->cols = 256 / TILE_WIDTH;
  td->rows = 192 / TILE_HEIGHT;
  td->tile_width = TILE_WIDTH;
  td->tile_height = TILE_HEIGHT;
  td->font = normal_font;
  td->frame_buffer = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN);

  /* Sub window */
  td = &data[1];
  WIPE(td, term_data);
  td->num  = 1;
  td->cols = 256 / TILE_WIDTH;
  td->rows = 192 / TILE_HEIGHT;
  td->tile_width = TILE_WIDTH;
  td->tile_height = TILE_HEIGHT;
  td->font = normal_font;
  td->frame_buffer = (u16b*) BG_BMP_RAM_SUB(ANG_BMP_BASE_SUB);
        
   /* Create windows (backwards!) -- MJS: WHY? for final term_activate?? */
  for (i = MAX_TERM_DATA - 1; i >= 0; i--) {
    /* Link */
    term_data_link(i);
    none = FALSE;
      
    /* Set global pointer?? Is this right??? */
    angband_term[i] = Term;
  }
  
  if (none) return (1);
  
  /* Success */
  return (0);
}


/*
 * Init some stuff
 *
 * This function is used to keep the "path" variable off the stack.
 */
static void init_stuff(void) {
  char path[1024];
  
  /* Prepare the path */
  strcpy(path, "/faangband/lib/");
  
  /* Prepare the filepaths */
  init_file_paths(path);
  
  /* Hack */
  strcpy(savefile, "/faangband/lib/save/PLAYER");
  
  small_screen = TRUE;
}

/*
 * Display warning message (see "z-util.c")
 */
static void hook_plog(cptr str) {
  if (str)
    printf(str);
}


void nds_exit(int code) {
  u16b i;
  for (i = 0; i < 60; i++) // wait 1 sec.
    do_vblank();	
  IPC->mailData = 0xDEADC0DE;	// tell arm7 to shut down the DS
}

/*
 * Display error message and quit (see "z-util.c")
 */

static void hook_quit(cptr str) {
  /* Give a warning */
  if (str)
    nds_fatal_err(str);
  /* Bail */
  nds_exit(0);
}


// From NH
int console_enabled = 0;
int was_console_layer_visible = 0;

/*
 * Right now, the main thing we do here is check for the lid state, so we
 * can power on/off as appropriate.
 */
void vsyncHandler() {
  int lid_closed = (((~IPC->buttons)<<6) & KEY_LID ) ^ KEY_LID;
  switch (power_state) {
    case POWER_STATE_ON:
      if (lid_closed) {
        powerOFF(POWER_ALL_2D);
        REG_IPC_FIFO_TX = SET_LEDBLINK_ON;
        power_state = POWER_STATE_TRANSITIONING;
      }
      
      break;

    case POWER_STATE_TRANSITIONING:
      REG_IPC_FIFO_TX = SET_LEDBLINK_SLOW;
      power_state = POWER_STATE_ASLEEP;

      break;

    case POWER_STATE_ASLEEP:
      if (! lid_closed) {
        powerON(POWER_ALL_2D);
        REG_IPC_FIFO_TX = SET_LEDBLINK_OFF;
        power_state = POWER_STATE_ON;
      }

      break;

    default:
      power_state = POWER_STATE_ON;
      break;
  }
}

void nds_show_console() {
  was_console_layer_visible = DISPLAY_CR & DISPLAY_BG0_ACTIVE;
  DISPLAY_CR ^= DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE;
  console_enabled = 1;
}

void nds_hide_console() {
  if (! was_console_layer_visible)
    DISPLAY_CR ^= DISPLAY_BG0_ACTIVE | DISPLAY_BG3_ACTIVE;
  console_enabled = 0;
}

/*
 * Get the power state of the DS.
 */
int nds_power_state() {
  return power_state;
}


/*
 * Key interrupt handler.  Right now, I only use this to toggle the console
 * layer on and off.
 */
void keysInterruptHandler()
{
  if (! console_enabled) {
    nds_show_console();
    iprintf("Console...\n");
  } else {
    nds_hide_console();
  }
}

void init_screen() {

  powerON(POWER_ALL_2D | POWER_SWAP_LCDS);
  lcdMainOnBottom();

  videoSetMode   (MODE_5_2D | DISPLAY_BG3_ACTIVE | DISPLAY_BG2_ACTIVE);
  videoSetModeSub(MODE_5_2D | DISPLAY_BG3_ACTIVE);

  vramSetBankA(VRAM_A_MAIN_BG_0x06000000);
  vramSetBankB(VRAM_B_MAIN_BG_0x06020000);
  vramSetBankC(VRAM_C_SUB_BG_0x06200000);
  vramSetBankD(VRAM_D_LCD);

  BG0_CR = BG_MAP_BASE(ANG_CONSOLE_MAP) 
    | BG_TILE_BASE(ANG_CONSOLE_TILE) | BG_16_COLOR | BG_PRIORITY_0;

  BG2_CR = BG_BMP16_256x256 | BG_BMP_BASE(ANG_BMP_BASE_MAIN_OVER)
    | BG_PRIORITY_1;

  BG3_CR = BG_BMP8_256x256 | BG_BMP_BASE(ANG_BMP_BASE_MAIN) 
    | BG_PRIORITY_2;

  SUB_BG0_CR = BG_MAP_BASE(ANG_KBD_MAP_BASE) 
    | BG_TILE_BASE(ANG_KBD_TILE_BASE) | BG_16_COLOR 
    | BG_PRIORITY_0;

  SUB_BG3_CR = BG_BMP8_256x256 | BG_BMP_BASE(ANG_BMP_BASE_SUB)
    | BG_PRIORITY_1;

  // blend BG2 and BG3 on the main screen
  BLEND_CR = BLEND_ALPHA | BLEND_SRC_BG2 | BLEND_DST_BG3;
  BLEND_AB = 0x0F | (0x0F << 8);

  BG3_XDX = 1 << 8;      BG3_XDY = 0;
  BG3_YDX = 0;           BG3_YDY = 1 << 8;
  BG3_CX = 0;            BG3_CY = 0;      

  BG2_XDX = 1 << 8;      BG2_XDY = 0;
  BG2_YDX = 0;           BG2_YDY = 1 << 8;
  BG2_CX = 0;            BG2_CY = 0;      

  SUB_BG3_XDX = 1 << 8;  SUB_BG3_XDY = 0;
  SUB_BG3_YDX = 0;       SUB_BG3_YDY = 1 << 8;
  SUB_BG3_CX = 0;        SUB_BG3_CY = 0;      

  consoleInitDefault((u16 *)BG_MAP_RAM(ANG_CONSOLE_MAP),
		     (u16 *)BG_TILE_RAM(ANG_CONSOLE_TILE),
		     16);

  printf("=== FAAngband console ===\n");

  irqInit();
  irqEnable(IRQ_VBLANK | IRQ_KEYS | IRQ_IPC_SYNC);

  irqSet(IRQ_KEYS, keysInterruptHandler);
  REG_KEYCNT |= 0x8000 | 0x4000 | KEY_SELECT | KEY_START;

  irqSet(IRQ_VBLANK, vsyncHandler);

  REG_IPC_FIFO_CR = IPC_FIFO_ENABLE | IPC_FIFO_SEND_CLEAR;
  REG_IPC_FIFO_TX = SET_LEDBLINK_OFF;

}

void init_font_etc() {
  int i;

  // nds_init_font_subpix();

  normal_font = read_bdf("nds/font.bdf");
  zoom_font = read_bdf("nds/font-zoom.bdf");

  if (normal_font == NULL) {
    iprintf("Error loading normal font!\n");
    return;
  }

  if (zoom_font == NULL) {
    iprintf("Error loading zoom font!\n");
    return;
  }

  { // report font in system log
    int minwd, maxwd, avgwd, ht;
    bdf_dims(normal_font, &minwd, &avgwd, &maxwd, &ht);
    printf("normal font dims:\n  width %d--%d (avg %d)\n  height %d\n", 
	   minwd,  maxwd, avgwd, ht);
    TILE_WIDTH  = avgwd;
    TILE_HEIGHT = ht;

    bdf_dims(zoom_font, &minwd, &avgwd, &maxwd, &ht);
    printf("zoom font dims:\n  width %d--%d (avg %d)\n  height %d\n", 
	   minwd,  maxwd, avgwd, ht);
  }

  keysSetRepeat(30, 2);

  /* Set up our palettes. */
  BG_PALETTE[255]     = RGB15(31,31,31);
  BG_PALETTE_SUB[255] = RGB15(31,31,31);

  for (i = 0; i < 16; i++)
    BG_PALETTE[TEXT_COLOUR_BASE+i]
      = BG_PALETTE_SUB[TEXT_COLOUR_BASE+i] 
      = color_data[i];
}

void draw_direction_pad () {
  int i, j, x0, y0;
  int dim = 32;
  u16b *fb = (u16b*) BG_BMP_RAM(ANG_BMP_BASE_MAIN_OVER);

  x0 = 256-4-dim;
  y0 = 192-4-dim;

  //for (i=0; i<256*192; i++)
  // fb[i] = 0;

  for (i=0; i<dim; i++)
    for (j=0; j<dim; j++)
      fb[(y0+j)*256+x0+i] = RGB15(8,8,15) | BIT(15);

  for (i=0; i<dim; i++) {
    fb[y0*256+x0+i]       = RGB15(15,15,31) | BIT(15);
    fb[(y0+dim)*256+x0+i] = RGB15(15,15,31) | BIT(15);
    fb[(y0+i)*256+x0]     = RGB15(15,15,31) | BIT(15);
    fb[(y0+i)*256+x0+dim] = RGB15(15,15,31) | BIT(15);
  }

}


/*
 * Main function
 *
 * This function must do a lot of stuff.
 */
int main(int argc, char *argv[]) {

  int i;
  bool game_start;

  /* Initialize the machine itself  */
  
  srand(IPC->time.rtc.hours * 60 * 60 + IPC->time.rtc.minutes * 60 
	+ IPC->time.rtc.seconds);

  init_screen();

  /* set up key repeat parameters */
  keysSetRepeat(20, 10);

  if (! fatInitDefault()) {
    iprintf("Unable to initialize FAT driver!\n");
    nds_show_console();
    return 1;
  }

  chdir("faangband");

  init_font_etc();

#define WAITS 5
  for (i=0; i<WAITS; i++)
    swiWaitForVBlank();

  nds_log("calling nds_load_kbd\n");
  if (!nds_load_kbd())
    nds_fatal_err("failed to load kbd data\n");

  for (i=0; i<WAITS; i++)
    swiWaitForVBlank();

  nds_log("calling kbd_init\n");
  kbd_init();

  /* Wait for response */

  for (i=0; i<WAITS; i++)
    swiWaitForVBlank();

  nds_log("Calling nds_init_buttons\n");
  nds_init_buttons();

  // draw_direction_pad();

  for (i=0; i<WAITS; i++)
    swiWaitForVBlank();

  IPC->mailData = 0x00424242;	// to arm7: everything has init'ed
  while ((IPC->mailData & 0xFFFFFF00) != 0x42424200); 
  
  /* Activate hooks */
  plog_aux = hook_plog;
  quit_aux = hook_quit;

  nds_log("Calling init_nds\n");

  /* Initialize the windows */
  if (init_nds()) quit("Oops!");
  
  for (i=0; i<WAITS; i++)
    swiWaitForVBlank();

  ANGBAND_SYS = "nds";

  nds_log("calling init_stuff\n");
  init_stuff();
  
  /* About to start */
  game_start = TRUE;
  while (game_start) {

    nds_log("calling init_angband\n");
    init_angband();

    for (i=0; i<WAITS; i++)
      swiWaitForVBlank();
      
    /* Wait for response */
    pause_line(data[0].rows-1);
      
    /* Play the game */
    play_game(false); // NB: plays "PLAYER"
      
    /* Free resources */
    cleanup_angband();
  }
  
  quit(NULL);
  
  return (0); // exit main
}


