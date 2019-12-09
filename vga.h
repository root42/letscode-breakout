#ifndef _VGA_H_
#define _VGA_H_

#include "types.h"

#define NUM_COLORS 256

#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320

extern byte far *VGA;

#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y) *(VGA+(x)+(y)*SCREEN_WIDTH)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

void set_graphics_mode();
void set_mode_y()
void set_text_mode();
void set_mode( byte mode );
void set_palette(byte *palette)

void wait_for_retrace();
void page_flip(byte **page1, byte **page2)

void blit2mem( byte far *d, int x, int y, int w, int h );
void blit2vga( byte far *s, int x, int y, int w, int h );
void draw_rectangle( int x, int y, int w, int h, byte c );

#endif
