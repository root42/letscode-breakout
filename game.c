#include <stdio.h>
#include <stdlib.h>

#include "gif.h"
#include "vga.h"

int main()
{
  struct image *img[4];
  word x,y;
  int i = 0;

  img[0] = load_gif("root42-0.gif");
  img[1] = load_gif("root42-1.gif");
  img[2] = load_gif("root42-2.gif");
  img[3] = load_gif("root42-3.gif");
  if( img[0] != NULL ) {
    set_mode_y();
    set_palette( &img[0]->palette[0][0] );
    /* convert_to_planes( img ); */
    /* blit2vga( img->data, 0, 0, 320, 200 ); */
    /* blit2page( img->data, vga_page0, 0, 0, 320, 200 ); */
    /* blit2page( img->data, vga_page1, 0, 0, 320, 200 ); */
    copy2page( img[0]->data, vga_page[0], 200 );
    copy2page( img[1]->data, vga_page[1], 200 );
    copy2page( img[2]->data, vga_page[2], 200 );
    copy2page( img[3]->data, vga_page[3], 200 );

    while( !kbhit() ) {
      delay(200);
      page_flip( &vga_page[i], &vga_page[(i + 1) % 4] );
      i = (i + 1) % 4;
    }
    set_text_mode();
    return 0;
  } else {
    printf("Could not load test.gif\n");
    return 1;
  }
}
