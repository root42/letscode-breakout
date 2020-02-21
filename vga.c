#include <dos.h>
#include <mem.h>

#include "vga.h"

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

byte far * const VGA=(byte far *)0xA0000000L;

/* dimensions of each page and offset */
const word vga_width = 320;
const word vga_height = 200;
word vga_page[4];

void set_graphics_mode()
{
  set_mode(VGA_256_COLOR_MODE);
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

void set_mode_y()
{
  set_mode( VGA_256_COLOR_MODE );
  vga_page[0] = 0;
  vga_page[1] = (vga_width*vga_height) / 4 * 1;
  vga_page[2] = (vga_width*vga_height) / 4 * 2;
  vga_page[3] = (vga_width*vga_height) / 4 * 3;
  /* disable chain 4 */
  outportb( SC_INDEX, MEMORY_MODE );
  outportb( SC_DATA, 0x06 );
  /* disable doubleword mode */
  outportb( CRTC_INDEX, UNDERLINE_LOCATION );
  outportb( CRTC_DATA, 0x00 );
  /* disable word mode */
  outportb( CRTC_INDEX, MODE_CONTROL );
  outportb( CRTC_DATA, 0xE3 );
  /* clear all VGA mem */
  outportb( SC_INDEX, MAP_MASK );
  outportb( SC_DATA, 0xFF );
  /* write 2^16 nulls */
  memset( VGA + 0x0000, 0, 0x8000 ); /* 0x10000 / 2 = 0x8000 */
  memset( VGA + 0x8000, 0, 0x8000 ); /* 0x10000 / 2 = 0x8000 */
}

void setpix( word page, int x, int y, byte c )
{
  outportb( SC_INDEX, MAP_MASK );
  outportb( SC_DATA, 1 << (x & 3) );
  VGA[ page + ((vga_width*y) >> 2) + (x >> 2) ] = c; /* x/4 is equal to x>>2 */
}

void page_flip( word *page1, word *page2 )
{
  word temp;
  word high_address, low_address;

  temp = *page1;
  *page1 = *page2;
  *page2 = temp;

  high_address = HIGH_ADDRESS | (*page1 & 0xFF00);
  low_address = LOW_ADDRESS | (*page1 << 8);
  /*
    instead of:
    outportb( CRTC_INDEX, HIGH_ADDRESS );
    outportb( CRTC_DATA, (*page1 & 0xFF00) >> 8 );

    do this:
    high_address = HIGH_ADDRESS | (*page1 & 0xFF00);
    outport( CRTC_INDEX, high_address );
   */
  while( inp( INPUT_STATUS ) & DISPLAY_ENABLE );
  outport( CRTC_INDEX, high_address );
  outport( CRTC_INDEX, low_address );
  while( !(inp( INPUT_STATUS ) & VRETRACE ) );
}

void copy2page( byte far *s, word page, int h )
{
  int x,y;
  byte c;
  for( y = 0; y < h; y++ ) {
    for( x = 0; x < vga_width; ++x ) {
      c = s[ y * vga_width + x ];
      setpix( page, x, y, c);
    }
  }
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
  byte far *src = (byte far *)VGA + x + y * SCREEN_WIDTH;
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
  byte far *dst = (byte far *)VGA + x + y * SCREEN_WIDTH;
  for( i = y; i < y + h; ++i ) {
    movedata( FP_SEG( src ), FP_OFF( src ),
	      FP_SEG( dst ), FP_OFF( dst ), w );
    src += w;
    dst += SCREEN_WIDTH;
  }
}

void blit2page( byte far *s, word page, int x, int y, int w, int h )
{
  int j;
  byte p;
  int screen_offset;
  int bitmap_offset;

  for( p = 0; p < 4; p++ ) {
    outportb( SC_INDEX, MAP_MASK );
    outportb( SC_DATA, 1 << ((p + x) & 3) );
    bitmap_offset = 0;
    screen_offset = ((dword)y * vga_width + x + p) >> 2;
    for(j=0; j<h; j++)
    {
      memcpy(
	&VGA[page + screen_offset],
	&s[bitmap_offset],
	w >> 2
	);
      bitmap_offset += w >> 2;
      screen_offset += vga_width >> 2;
    }
  }
}

/* Warning: no clipping! */
void draw_rectangle( int x, int y, int w, int h, byte c )
{
  int i, j;
  for( j = y; j < y + h; ++j ) {
    for( i = x; i < x + w; ++i ) {
      byte far *dst = (byte far *)VGA + i + j * SCREEN_WIDTH;
      *dst = c;
    }
  }
}

