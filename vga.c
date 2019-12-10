#include <dos.h>

#include "vga.h"

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

const byte far *VGA=(byte far *)0xA0000000L;

/* dimensions of each page and offset */
const int vga_width = 320;
const int vga_height = 200;
byte far *vga_page0 = 0;
byte far *vga_page1 = vga_width * vga_height;

void set_graphics_mode()
{
  set_mode(VGA_256_COLOR_MODE);
}

void set_mode_y()
{
  union REGS r;

  set_mode( VGA_256_COLOR_MODE );

  /* disable chain 4 */
  outportw( SC_INDEX, MEMORY_MODE + 0x0600 );
  /* disable word mode */
  outportw( CRTC_INDEX, MODE_CONTROL + 0xE300 );
  /* disable doubleword mode */
  outportw( CRTC_INDEX, UNDERLINE_LOCATION + 0x0000 );
  /* clear all vga memory: enable all planes */
  outportw( SC_INDEX, ALL_PLANES );
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
  outportw( CRTC_INDEX, high_address );
  outportw( CRTC_INDEX, low_address );
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

void blit2page( byte far *s, byte far *page, int x, int y, int w, int h )
{
  int i;
  byte p;
  int screen_offset;
  int bitmap_offset;
  const int page_size = (w * h) >> 2;

  for( p = 0; p < 4; p++ ) {
    outportw( SC_INDEX, MAP_MASK, ( 1 << ((p + x) & 3) ) << 8 );
    bitmap_offset = 0;
    screen_offset = (y * vga_width + x + p) >> 2;
    for(j=0; j<bmp->height; j++)
    {
      memcpy(
	&VGA[screen_offset],
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
      byte far *dst = VGA + i + j * SCREEN_WIDTH;
      *dst = c;
    }
  }
}

