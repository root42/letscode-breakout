#include <dos.h>

#include "vga.h"

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

#define PALETTE_INDEX 0x3C8
#define PALETTE_DATA 0x3C9
#define INPUT_STATUS 0x03DA
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

byte far *VGA=(byte far *)0xA0000000L;

void set_graphics_mode()
{
  set_mode(VGA_256_COLOR_MODE);
}

void set_mode_y()
{
  union REGS r;

  set_mode( VGA_256_COLOR_MODE );

  /* disable chain 4 */
  outpw( SC_INDEX, MEMORY_MODE + 0x0600 );
  /* disable word mode */
  outpw( CRTC_INDEX, MODE_CONTROL + 0xE300 );
  /* disable doubleword mode */
  outpw( CRTC_INDEX, UNDERLINE_LOCATION + 0x0000 );
  /* clear all vga memory: enable all planes */
  outpw( SC_INDEX, ALL_PLANES );
  /* size_t is 16 bit, so do it 2x */
  memset( VGA + 0x0000, 0, 0x8000 );
  memset( VGA + 0x8000, 0, 0x8000 );
}

void page_flip(byte **page1, byte **page2)
{
  word high_address, low_address;
  
  *page1 = *page1 ^ *page2;
  *page2 = *page1 ^ *page2;
  *page1 = *page1 ^ *page2;

  high_address = HIGH_ADDRESS | (*page1 & 0xFF00);
  low_address = LOW_ADDRESS | (*page1 << 8);

  while( inp( INPUT_STATUS ) & DISPLAY_ENABLE );
  outpw( CRTC_INDEX, high_address );
  outpw( CRTC_INDEX, low_address );
  while( !(inp( INPUT_STATUS ) & VRETRACE) );
}

void set_text_mode()
{
  set_mode(TEXT_MODE);
}

void set_mode(byte mode)
{
  union REGS regs;
  regs.h.ah = SET_MODE;
  regs.h.al = mode;
  int86( VIDEO_INT, &regs, &regs );
}

void wait_for_retrace()
{
  while( inp( INPUT_STATUS ) & VRETRACE );
  while( ! (inp( INPUT_STATUS ) & VRETRACE) );
}

void set_palette(byte *palette)
{
  int i;

  outp( PALETTE_INDEX, 0 );
  for( i = 0; i < NUM_COLORS * 3; ++i ) {
    outp( PALETTE_DATA, palette[ i ] );
  }
}

void blit2mem( byte far *d, int x, int y, int w, int h )
{
  int i;
  byte far *src = VGA + x + y * SCREEN_WIDTH;
  byte far *dst = d;
  for( i = y; i < y + h; ++i ) {
    movedata( FP_SEG( src ), FP_OFF( src ),
	      FP_SEG( dst ), FP_OFF( dst ), w );
    src += SCREEN_WIDTH;
    dst += w;
  }
}

void blit2vga( byte far *s, int x, int y, int w, int h )
{
  int i;
  byte far *src = s;
  byte far *dst = VGA + x + y * SCREEN_WIDTH;
  for( i = y; i < y + h; ++i ) {
    movedata( FP_SEG( src ), FP_OFF( src ),
	      FP_SEG( dst ), FP_OFF( dst ), w );
    src += w;
    dst += SCREEN_WIDTH;
  }
}

/* Warning: no clipping! */
void draw_rectangle( int x, int y, int w, int h, byte c )
{
  int i, j;
  for( j = y; j < y + h; ++j ) {
    for( i = x; i < x + w; ++i ) {
      byte far *dst = VGA + i + j * SCREEN_WIDTH;
      *dst = c;
    }
  }
}

