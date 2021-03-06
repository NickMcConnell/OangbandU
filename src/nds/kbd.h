#ifndef KBD_H
#define KBD_H

typedef u16 nds_palette[16];
int nds_load_palette(char *fname, nds_palette palette);

typedef struct {
	u16 width;
	u16 code;
} nds_kbd_key;

void redraw_kbd(int);
void kbd_init();
u16 kbd_xy2key(u8 x, u8 y);
void kbd_togglemod(int which, int how);
void kbd_set_color_from_pos(u16 r, u16 k,u8 color);
void kbd_set_color_from_code(u16 code,u8 color);
byte kbd_vblank();
bool nds_load_kbd();

#define K_MODIFIER	0x100	// if set: it's shift or ctrl or alt
#define K_CAPS		0x101
#define K_SHIFT		0x102
#define K_CTRL		0x103
#define K_ALT		0x104
#define K_F(n)		(0x200 + n)
#define K_SHIFTED_MOVE		0x0200

#endif



