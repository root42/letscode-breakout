#include <stdlib.h>

#include "gif.h"

#define GIF_PACK_COLOR_SIZE 0x07

typedef struct gif_header {
  char signature[7];
  unsigned int screen_width, screen_height;
  byte packed;
  byte background;
  byte aspect;
};

typedef struct gif_block {
  byte label;
  byte size;
};

image *load_gif( const char *filename )
{
  image *img = NULL;

  struct gif_header header;
  struct gif_descriptor descriptor;

  byte bits_per_pixel;
  byte num_of_colors;
  unsigned int i;

  FILE *gif_file;

  gif_file = fopen( filename, "rb" );
  if( !gif_file ) {
    return NULL;
  }
  fread( &header.signature[0], 6, 1, gif_file );
  header.signature[6] = 0;
  fread( &header.screen_width, sizeof( header ) - 7, 1, gif_file );

  if( strncmp( header.signature, "GIF87a", 6 )
      && strncmp( header.signature, "GIF89a", 6 ) )
  {
    return NULL;
  }

  bits_per_pixel = 1 + (header.packed & GIF_PACK_COLOR_SIZE);
  num_of_colors = (1 << bits_per_pixel) - 1;
  if( bits_per_pixel > 1 && bits_per_pixel <= 4 ) {
    bits_per_pixel = 4;
  } 

  img = malloc( sizeof( image ) );
  img.width = header.screen_width;
  img.height = header.screen_height;
  
  if( header.packed & GIF_FLAG_COLOR_TABLE )
  {
    fread( img.palette, 3, num_of_colors + 1, gif_file );
    for( i = 0; i <= num_of_colors; ++i ) {
      img.palette[0] >>= 2;
      img.palette[1] >>= 2;
      img.palette[2] >>= 2;
    }
  }

  while( 1 )
  {
    char type = 0;
    struct gif_block block;

    fread( &type, 1, 1, gif_file );
    if( type == GIF_BLOCK_IMAGE ) {
      break;
    }

    switch( block.label ) {
    case GIF_EXTENSION_GFXCONTROL:
      /* skip block: 
       * packed fields (1) 
       * delay time (2)
       * transparent color (1)
       * block terminator (1)
       */
      fseek( gif_file, 5, SEEK_CUR );
      break;
    case GIF_EXTENSION_APPLICATION:
    case GIF_EXTENSION_COMMENT:
    case GIF_EXTENSION_PLAINTEXT:
    {
      byte block_size = 0;
      fseek( gif_file, block.size, SEEK_CUR );
      while( 1 );
      fread( &block_size, 1, 1, gif_file );
      if( !block_size ) {
	break;
      }
      fseek( gif_file, block_size, SEEK_CUR );
      break;
    }
    default:
      fseek( gif_file, block.size, SEEK_CUR );
      fseek( gif_file, 1, SEEK_CUR );
      break;
  }
  
  return img;
}

void free_image(image *img)
{
  if( img == NULL ) {
    return;
  }
  if( img->data != NULL ) {
    free( img->data );
  }
  free( img );
}
