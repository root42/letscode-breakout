#ifndef _GIF_H_
#define _GIF_H_

#include "types.h"

typedef struct image {
  unsigned int width;
  unsigned int height;
  byte palette[256][3];
  byte *data;
};

struct image *load_gif( const char *filename );
void free_image(struct image *img);
void convert_to_planes(struct image *img);

#endif
