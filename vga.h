#ifndef _VGA_H_
#define _VGA_H_

#define NUM_COLORS 256

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320

#define PALETTE_INDEX 0x3C8
#define PALETTE_DATA 0x3C9
#define INPUT_STATUS 0x03DA
#define VRETRACE 0x08

typedef unsigned char byte;

extern byte far *VGA;

#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y) *(VGA+(x)+(y)*SCREEN_WIDTH)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))

void set_mode(byte mode);
void wait_for_retrace();
void blit2mem( byte far *d, int x, int y, int w, int h );
void blit2vga( byte far *s, int x, int y, int w, int h );
void draw_rectangle( int x, int y, int w, int h, byte c );

#endif
