#include <stdio.h>
#include <math.h>

#include "gif.h"
#include "vga.h"

#ifndef M_PI
#define M_PI 3.14159
#endif

unsigned int SINEX[256];
unsigned int SINEY[256];

void init_sin()
{
  int i;
  for( i = 0; i < 256; ++i ) {
    SINEX[ i ] = 320 * ( (sin( 2.0 * M_PI * i / 256.0 ) + 1.0 ) / 2.0 );
    SINEY[ i ] = 200 * ( (sin( 2.0 * M_PI * i / 128.0 ) + 1.0 ) / 2.0 );
  }
}

int main()
{
  struct image *img[2];
  int frame = 0;
  int x = 0;
  int y = 0;

  init_sin();
  img[0] = load_gif("root42-a.gif");
  img[1] = load_gif("root42-b.gif");
  if( img[0] != NULL ) {
    set_mode_y();
    set_virtual_640();
    set_palette( &img[0]->palette[0][0] );

    copy2page( img[0]->data, vga_page[0],   0, 0, img[0]->width, img[0]->height );
    copy2page( img[1]->data, vga_page[0], 320, 0, img[1]->width, img[1]->height );
    copy2page( img[0]->data, vga_page[1],   0, 0, img[0]->width, img[0]->height );
    copy2page( img[1]->data, vga_page[1], 320, 0, img[1]->width, img[1]->height );

    while( !kbhit() ) {
      x = SINEX[frame % 256];
      y = SINEY[frame % 256];
      set_x_pan( x );
      set_y_pan( y );
      update_pan();
/*      page_flip( &vga_page[0], &vga_page[1] );*/
      frame++;
    }
    set_text_mode();
    return 0;
  } else {
    printf("Could not load test.gif\n");
    return 1;
  }

}
