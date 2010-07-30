#include <main-nds.h>
#include <nds/font-bdf.h>
#include <nds/ppm-lite.h>

void draw_text(struct font *fnt, 
	       char *str,
	       int x, int y,
	       u16 *dest)
{
  int w, h;
  struct ppm *img;
  text_dims(fnt, str, &w, &h);
  img = alloc_ppm(w, h);
  draw_string(fnt, str, img, 0, 0, -1, -1);
  draw_ppm(img, dest, x, y, 256);
  free_ppm(img);
}

void draw_colour_text(struct font *fnt, 
		      char *str,
		      int x, int y,
		      u16 *dest,
		      int fg, int bg)
{
  int w, h;
  struct ppm *img;
  text_dims(fnt, str, &w, &h);
  img = alloc_ppm(w, h);
  draw_string(fnt, str, img, 0, 0, fg, bg);
  draw_ppm(img, dest, x, y, 256);
  free_ppm(img);
}

void draw_char(int x, int y, char c, u16b *dest, struct font *font) {
  char str[2] = { 0, 0 };
  str[0] = c;
  draw_text(font, str, x, y, dest);
}

void draw_color_char(int x, int y, char c, int fg, int bg, u16b *dest,
		     struct font *font) {
  int fgc = -1, bgc = -1;
  if (fg >= 0) fgc = fg & 0xF;
  if (bg >= 0) bgc = bg & 0xF;
  char str[2] = { 0, 0 };
  str[0] = c;
  draw_colour_text(font, str, x, y, dest, fgc, bgc);
}

