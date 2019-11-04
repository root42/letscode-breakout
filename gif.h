#ifdef _GIF_H_
#define _GIF_H_

#include "types.h"

typedef struct image {
  unsigned int width;
  unsigned int height;
  byte palette[256][3];
  byte *data;
};

image *load_gif( const char *filename );
void free_image(image *img);

#endif
