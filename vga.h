#ifndef _VGA_H_
#define _VGA_H_

#include "types.h"

#define NUM_COLORS 256

/* VGA ports */
#define PALETTE_INDEX 0x3C8
#define PALETTE_DATA 0x3C9
#define INPUT_STATUS 0x03DA
#define MISC_OUTPUT 0x3C2
#define SC_INDEX 0x3C4
#define SC_DATA 0x3C5
#define GC_INDEX 0x03CE
#define GC_DATA 0x03CF
#define CRTC_INDEX 0x03D4
#define CRTC_DATA 0x03D5

/* VGA status bits */
#define DISPLAY_ENABLE 0x01
#define VRETRACE 0x08

/* Sequence controller registers */
#define MAP_MASK 0x02
#define ALL_PLANES 0xFF02
#define MEMORY_MODE 0x04

/* CRT controller registers */
#define HIGH_ADDRESS 0x0C
#define LOW_ADDRESS 0x0D
#define UNDERLINE_LOCATION 0x14
#define MODE_CONTROL 0x17

/* VGA memory pointer, dimensions of each page and offset */
extern byte far * const VGA;
extern const word vga_width;
extern const word vga_height;
extern word vga_page[4];

#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

/* macros for mode 13h*/
#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320
#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y) *(VGA+(x)+(y)*SCREEN_WIDTH)

void set_graphics_mode();
void set_mode_y();
void set_text_mode();
void set_mode( byte mode );
void set_palette(byte *palette);
void setpix(word page, int x, int y, byte c);

void wait_for_retrace();
void page_flip(word *page1, word *page2);

void copy2page( byte far *s, word page, int h );
void blit2mem( byte far *d, int x, int y, int w, int h );
void blit2vga( byte far *s, int x, int y, int w, int h );
void blit2page( byte far *s, word page, int x, int y, int w, int h );
void draw_rectangle( int x, int y, int w, int h, byte c );

#endif
