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

void set_mode_y()
{
  union REGS r;

  set_mode( VGA_256_COLOR_MODE );

  vga_page[0] = 0;
  vga_page[1] = (vga_width * vga_height) / 4 * 1;
  vga_page[2] = (vga_width * vga_height) / 4 * 2;
  vga_page[3] = (vga_width * vga_height) / 4 * 3;

  /* disable chain 4 */
  outportb( SC_INDEX, MEMORY_MODE );
  outportb( SC_DATA, 0x06 );
  /* disable doubleword mode */
  outportb( CRTC_INDEX, UNDERLINE_LOCATION );
  outportb( CRTC_DATA, 0x00 );
  /* disable word mode */
  outportb( CRTC_INDEX, MODE_CONTROL );
  outportb( CRTC_DATA, 0xE3 );
  /* clear all vga memory: enable all planes */
  outportb( SC_INDEX, MAP_MASK );
  outportb( SC_DATA, 0xFF );
  /* size_t is 16 bit, so do it 2x */
  memset( VGA + 0x0000, 0, 0x8000 );
  memset( VGA + 0x8000, 0, 0x8000 );
}

void page_flip(word *page1, word *page2)
{
  word high_address, low_address;
  word temp;

  temp = *page1;
  *page1 = *page2;
  *page2 = temp;

  high_address = HIGH_ADDRESS | (*page1 & 0xFF00);
  low_address = LOW_ADDRESS | (*page1 << 8);

  while( inp( INPUT_STATUS ) & DISPLAY_ENABLE );
  outport( CRTC_INDEX, high_address );
  outport( CRTC_INDEX, low_address );
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

void setpix( word page, int x, int y, byte c) {
  outportb(SC_INDEX, MAP_MASK);
  outportb(SC_DATA, 0x01 << (x & 3));
  VGA[page + ((vga_width * y) >> 2) + (x >> 2)] = c;
}

void copy2page( byte far *s, word page, int h )
{
  int i,j;
  byte c;
  for( j = 0; j < h; j++ ) {
    for( i = 0; i < SCREEN_WIDTH; i++ ) {
      c = s[ j * SCREEN_WIDTH + i ];
      setpix( page, i, j, c );
    }
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