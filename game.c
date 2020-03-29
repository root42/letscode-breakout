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
  for( i = 0;i < 256; ++i ) {
    SINEX[i] = 320 * (( sin(2.0*M_PI*i/256.0) + 1.0) / 2.0);
    SINEY[i] = 200 * (( sin(2.0*M_PI*i/128.0) + 1.0) / 2.0);
  }
}

int main()
{
  struct image *img[4];
  word x, y, frame = 0;
  int modex = 1, i = 0;

  init_sin();
  img[0] = load_gif("run-1.gif", modex);
  img[1] = load_gif("run-2.gif", modex);
  img[2] = load_gif("run-3.gif", modex);
  img[3] = load_gif("run-4.gif", modex);
  for( i = 0; i < 4; ++i ) {
    if( img[i] == NULL ) {
      printf( "Couldn't load image %u\n", i );
      return 1;
    }
  }
  set_mode_y();
  set_palette( &img[0]->palette[0][0] );

  while( !kbhit() ) {
    i = frame % 4;
    if( modex ) {
      blit2page( img[i]->data, vga_page[1], 0, 0, img[i]->width, img[i]->height );
    } else {
      copy2page( img[i]->data, vga_page[1], 0, 0, img[i]->width, img[i]->height );
    }
    page_flip( &vga_page[0], &vga_page[1] );
    frame++;
    delay(250);
  }
  getch();
  set_text_mode();
  return 0;
}
