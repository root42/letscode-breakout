#include <dos.h>
#include <mem.h>

#include "vga.h"

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

byte far * const VGA=(byte far *)0xA0000000L;

/* dimensions of each page and offset */
word vga_width = 320;
word vga_height = 200;
word vga_page[4];
word vga_current_page = 0;
word vga_x_pan = 0;
byte vga_x_pel_pan = 0;
word vga_y_pan = 0;

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

void update_page_offsets()
{
  vga_page[0] = 0;
  vga_page[1] = ((dword)vga_width*vga_height) / 4;
  vga_page[2] = vga_page[1] * 2;
  vga_page[3] = vga_page[1] * 3;
}

void set_mode_y()
{
  set_mode( VGA_256_COLOR_MODE );
  update_page_offsets();
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

void set_virtual_640()
{
  vga_width = 640;
  update_page_offsets();
  outportb( CRTC_INDEX, LINE_OFFSET );
  outportb( CRTC_DATA, vga_width / 8 );
}

void set_pan_x( int x )
{
  vga_x_pan = x / 4;
  vga_x_pel_pan = x % 4;
}

void set_pan_y( int y )
{
  vga_y_pan = y;
}

void update_pan()
{
  byte ac;
  word o,high_address,low_address;

  o = vga_y_pan * (vga_width >> 2) + vga_x_pan;
  high_address = HIGH_ADDRESS | (o & 0xFF00);
  low_address = LOW_ADDRESS | (o << 8);
  outport( CRTC_INDEX, high_address );
  outport( CRTC_INDEX, low_address );
  disable();
  inp( INPUT_STATUS );
  ac = inp( AC_WRITE );
  outportb( AC_WRITE, PEL_PANNING );
  outportb( AC_WRITE, vga_x_pel_pan );
  outportb( AC_WRITE, ac );
  while( inp( INPUT_STATUS ) & VRETRACE );
  while( !(inp( INPUT_STATUS ) & VRETRACE ) );
  enable();
}

void setpix( word page, int x, int y, byte c )
{
  outportb( SC_INDEX, MAP_MASK );
  outportb( SC_DATA, 1 << (x & 3) );
  VGA[ page + ((dword)vga_width * y >> 2) + (x >> 2) ] = c;
  /* x/4 is equal to x>>2 */
}

void page_flip( word *page1, word *page2 )
{
  byte ac;
  word temp;
  word high_address, low_address;

  temp = *page1;
  *page1 = *page2;
  *page2 = temp;

  high_address = HIGH_ADDRESS | ((*page1 + vga_x_pan) & 0xFF00);
  low_address = LOW_ADDRESS | ((*page1 + vga_x_pan) << 8);
  /*
    instead of:
    outportb( CRTC_INDEX, HIGH_ADDRESS );
    outportb( CRTC_DATA, (*page1 & 0xFF00) >> 8 );

    do this:
    high_address = HIGH_ADDRESS | (*page1 & 0xFF00);
    outport( CRTC_INDEX, high_address );
   */
  outport( CRTC_INDEX, high_address );
  outport( CRTC_INDEX, low_address );
  disable();
#if 0
  ac = inp( AC_WRITE );
  outportb( AC_WRITE, PEL_PANNING );
  outportb( AC_WRITE, vga_x_pel_pan );
  outportb( AC_WRITE, ac );
#endif
  while( inp( INPUT_STATUS ) & VRETRACE );
  while( !(inp( INPUT_STATUS ) & VRETRACE ) );
  enable();
  vga_current_page = *page1;
}

void copy2page( byte far *s, word page, int x0, int y0, int w, int h )
{
  int x,y;
  byte c;
  for( y = 0; y < h; y++ ) {
    for( x = 0; x < w; ++x ) {
      c = *(s + (dword)y * w + x);
      setpix( page, x0 + x, y0 + y, c);
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
  byte far *src = (byte far *)VGA + x + y * vga_width;
  byte far *dst = d;
  for( i = y; i < y + h; ++i ) {
    movedata( FP_SEG( src ), FP_OFF( src ),
	      FP_SEG( dst ), FP_OFF( dst ), w );
    src += vga_width;
    dst += w;
  }
}

void blit2vga( byte far *s, int x, int y, int w, int h )
{
  int i;
  byte far *src = s;
  byte far *dst = (byte far *)VGA + x + y * vga_width;
  for( i = y; i < y + h; ++i ) {
    movedata( FP_SEG( src ), FP_OFF( src ),
	      FP_SEG( dst ), FP_OFF( dst ), w );
    src += w;
    dst += vga_width;
  }
}

void blit2page( byte far *s, word page, int x, int y, int w, int h )
{
  int j;
  byte p;
  dword screen_offset;
  dword bitmap_offset;

  for( p = 0; p < 4; p++ ) {
    outportb( SC_INDEX, MAP_MASK );
    outportb( SC_DATA, 1 << ((p + x) & 3) );
    bitmap_offset = p * w / 4 * h;
    screen_offset = ((dword)y * vga_width + x + p) >> 2;
    for(j=0; j<h; j++)
    {
      memcpy(
	VGA + page + screen_offset,
	s + bitmap_offset,
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
      byte far *dst = (byte far *)VGA + i + j * vga_width;
      *dst = c;
    }
  }
}

