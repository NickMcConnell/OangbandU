#ifndef MAIN_NDS_H
#define MAIN_NDS_H

#include <fat.h>
#include <nds.h>
#include <angband.h>
#include <nds/font-bdf.h>

// MJS - mem layout
#define ANG_BMP_BASE_MAIN  4
#define ANG_BMP_BASE_SUB   4

#define ANG_BMP_BASE_MAIN_OVER 8

#define ANG_SUBPIX_BASE    18

#define ANG_KBD_TILE_BASE  2
#define ANG_KBD_MAP_BASE   4

#define ANG_CONSOLE_MAP    0
#define ANG_CONSOLE_TILE   1

// from NH:
#define POWER_STATE_ON            0
#define POWER_STATE_TRANSITIONING 1
#define POWER_STATE_ASLEEP        2

#define SET_LEDBLINK_OFF  (0<<4)
#define SET_LEDBLINK_SLOW (1<<4)
#define SET_LEDBLINK_ON   (3<<4)

void put_key_event(byte c);
bool nds_load_file(const char* name, u16b* dest, u32b len);
void nds_fatal_err(const char* msg);
void nds_raw_print(const char* str);
void nds_log(const char *fmt, ...);

// from NH:
int nds_power_state();
void nds_show_console();
void nds_hide_console();
void nds_error();

extern struct font *system_font;

#define TEXT_ATTR_BOLD    1
#define TEXT_ATTR_ULINE   2
#define TEXT_ATTR_INVERSE 4

/*
 * The color scheme used is tailored for an IBM PC.  It consists of the
 * standard 8 colors, folowed by their bright counterparts.  There are
 * exceptions, these are listed below.	Bright black doesn't mean very
 * much, so it is used as the "default" foreground color of the screen.
 */
#define CLR_BLACK		0
#define CLR_RED			1
#define CLR_GREEN		2
#define CLR_BROWN		3 /* on IBM, low-intensity yellow is brown */
#define CLR_BLUE		4
#define CLR_MAGENTA		5
#define CLR_CYAN		6
#define CLR_GRAY		7 /* low-intensity white */
#define NO_COLOR		8
#define CLR_ORANGE		9
#define CLR_BRIGHT_GREEN	10
#define CLR_YELLOW		11
#define CLR_BRIGHT_BLUE		12
#define CLR_BRIGHT_MAGENTA	13
#define CLR_BRIGHT_CYAN		14
#define CLR_WHITE		15
#define CLR_MAX			16

/* The "half-way" point for tty based color systems.  This is used in */
/* the tty color setup code.  (IMHO, it should be removed - dean).    */
#define BRIGHT		8

#endif
