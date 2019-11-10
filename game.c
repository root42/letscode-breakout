#include <stdio.h>

#include "gif.h"
#include "vga.h"

int main()
{
  struct image *img;
  img = load_gif("test.gif");
  if( img != NULL ) {
    set_graphics_mode();
    set_palette( img->palette );
    blit2vga( img->data, 0, 0, 320, 200 );
    while( !kbhit() )
      ;
    set_text_mode();
    return 0;
  } else {
    printf("Could not load test.gif\n");
    return 1;
  }
  
}
