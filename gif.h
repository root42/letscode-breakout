#ifndef _GIF_H_
#define _GIF_H_

#include "types.h"

typedef struct image {
  word width;
  word height;
  byte palette[256][3];
  byte far *data;
};

struct image *load_gif( const char *filename );
void free_image(struct image *img);
void convert_to_planes(struct image *img);

#endif
