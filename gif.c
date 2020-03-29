#include <alloc.h>
#include <stdio.h>
#include <stdlib.h>

#include "gif.h"

#define GIF_BLOCK_IMAGE 0x2C
#define GIF_EXTENSION_APPLICATION 0xFF
#define GIF_EXTENSION_COMMENT 0xFE
#define GIF_EXTENSION_GFXCONTROL 0xF9
#define GIF_EXTENSION_GFXCONTROL 0xF9
#define GIF_EXTENSION_PLAINTEXT 0x01
#define GIF_FLAG_COLOR_TABLE 0x80
#define GIF_FLAG_INTERLACED 0x40
#define GIF_PACK_COLOR_SIZE 0x07

typedef struct gif_header {
  char signature[7];
  unsigned int screen_width, screen_height;
  byte packed;
  byte background;
  byte aspect;
};

struct gif_descriptor {
  unsigned int  image_left, image_top, image_width, image_height;
  byte packed;
};

typedef struct gif_block {
  byte label;
  byte size;
};

typedef struct decoder_state {
  FILE *gif_file;
  struct image *img;

  unsigned int b_pointer;
  byte buffer[257];
  byte block_size;
  byte code_size;
  char bits_in;
  byte temp;

  unsigned int first_free;
  unsigned int free_code;
  byte init_code_size;
  unsigned int code, old_code, max_code;
  unsigned int clear_code, eoi_code;
  unsigned int prefix[ 4096 ];
  byte suffix[ 4096 ];

  unsigned int x, y;
  unsigned int tlx, tly, brx, bry, dy;

  int modex;
};

byte load_byte( struct decoder_state *decoder )
{
  if( decoder->b_pointer == decoder->block_size ) {
    fread( decoder->buffer, decoder->block_size + 1, 1, decoder->gif_file );
    decoder->b_pointer = 0;
  }
  return decoder->buffer[ decoder->b_pointer++ ];
}

unsigned int read_code( struct decoder_state *decoder )
{
  int counter;
  unsigned int code = 0;

  for( counter = 0; counter < decoder->code_size; ++counter )
  {
    if( ++(decoder->bits_in) == 9 ) {
      decoder->temp = load_byte( decoder );
      decoder->bits_in = 1;
    }
    if( decoder->temp & 1 ) {
      code += 1 << counter;
    }
    decoder->temp >>= 1;
  }
  return code;
}

void next_pixel( struct decoder_state* decoder, unsigned int c )
{
  word plane, x1, sx, sy, offset;
  sx = decoder->img->width;
  sy = decoder->img->height;
  if( decoder->modex ) {
    plane = decoder->x % 4;
    x1 = decoder->x / 4;
    offset = plane * (sx / 4) * sy;
    *(decoder->img->data + offset + decoder->y * sx / 4 + x1) = c & 255;
  } else {
    *(decoder->img->data + decoder->x + decoder->y * sx) = c & 255;
  }

  if( ++(decoder->x) == decoder->brx )
  {
    printf(".");
    decoder->x = decoder->tlx;
    switch( decoder->dy ) {
    case 0:
      decoder->y += 1;
      break;
    case 1:
      decoder->y += 2;
      break;
    case 2:
      decoder->y += 4;
      break;
    case 3:
    case 4:
      decoder->y += 8;
      break;
    }
    if( decoder->y >= decoder->bry ) {
      switch( --decoder->dy ) {
      case 3:
	decoder->y = 4;
	break;
      case 2:
	decoder->y = 2;
	break;
      case 1:
	decoder->y = 1;
	break;
      }
    }
  }
}

byte out_string( struct decoder_state *decoder, unsigned int cur_code )
{
  unsigned int out_count;
  byte out_code[1024];

  if( cur_code < decoder->clear_code ) {
    next_pixel( decoder, cur_code);
  } else {
    out_count = 0;
    do {
      out_code[out_count++] = decoder->suffix[cur_code];
      cur_code = decoder->prefix[cur_code];
    } while( cur_code >= decoder->clear_code );
    out_code[out_count++] = cur_code;
    do {
      next_pixel( decoder, out_code[--out_count] );
    } while( out_count );
  }
  return cur_code;
}

struct image *load_gif( const char *filename, int modex )
{
  struct image *img = NULL;
  struct gif_header header;
  struct gif_descriptor descriptor;
  struct decoder_state *decoder;

  byte bits_per_pixel;
  byte num_of_colors;
  unsigned int i;

  FILE *gif_file;

  gif_file = fopen( filename, "rb" );
  if( !gif_file ) {
    return NULL;
  }
  decoder = malloc( sizeof( struct decoder_state ) );
  fread( &header.signature[0], 6, 1, gif_file );
  header.signature[6] = 0;
  fread( &header.screen_width, sizeof( header ) - 7, 1, gif_file );

  if( strncmp( header.signature, "GIF87a", 6 )
      && strncmp( header.signature, "GIF89a", 6 ) )
  {
    return NULL;
  }

  decoder->modex = modex;
  bits_per_pixel = 1 + (header.packed & GIF_PACK_COLOR_SIZE);
  num_of_colors = (1 << bits_per_pixel) - 1;
  if( bits_per_pixel > 1 && bits_per_pixel <= 4 ) {
    bits_per_pixel = 4;
  }

  img = malloc( sizeof( struct image ) );
  img->width = header.screen_width;
  img->height = header.screen_height;
  printf( "far core left: %lu\n", farcoreleft() );
  img->data = (byte far *)farmalloc( img->width * img->height );
  if( img->data == NULL ) {
    printf( "Error: could not get memory for image\n" );
    free( img );
    return NULL;
  }
  memset( img->data, 0, sizeof( img->width * img->height ) );

  if( header.packed & GIF_FLAG_COLOR_TABLE )
  {
    fread( img->palette, 3, num_of_colors + 1, gif_file );
    for( i = 0; i <= num_of_colors; ++i ) {
      img->palette[i][0] >>= 2;
      img->palette[i][1] >>= 2;
      img->palette[i][2] >>= 2;
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

    fread( &block, sizeof(struct gif_block), 1, gif_file);

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
      fseek( gif_file, block.size, SEEK_CUR );
      while( 1 ) {
	fread( &(block.size), 1, 1, gif_file );
	if( !block.size ) {
	  break;
	}
	fseek( gif_file, block.size, SEEK_CUR );
      }
      break;
    }
    default:
      fseek( gif_file, block.size, SEEK_CUR );
      fseek( gif_file, 1, SEEK_CUR );
      break;
    }
  }

  fread( &descriptor, sizeof( descriptor ), 1, gif_file );

  if( descriptor.packed & GIF_FLAG_COLOR_TABLE )
  {
    fread( img->palette, 3, num_of_colors + 1, gif_file );
    for( i = 0; i <= num_of_colors; ++i ) {
      img->palette[i][0] >>= 2;
      img->palette[i][1] >>= 2;
      img->palette[i][2] >>= 2;
    }
  }

  decoder->tlx = descriptor.image_left;
  decoder->tly = descriptor.image_top;
  decoder->brx = decoder->tlx + descriptor.image_width;
  decoder->bry = decoder->tly + descriptor.image_height;
  decoder->dy = ( descriptor.packed & GIF_FLAG_INTERLACED ) ? 4 : 0;

  fread( &decoder->code_size, 1, 1, gif_file );
  fread( &decoder->block_size, 1, 1, gif_file );

  decoder->gif_file = gif_file;
  decoder->b_pointer = decoder->block_size;
  decoder->clear_code = 1 << decoder->code_size;
  decoder->eoi_code = decoder->clear_code + 1;
  decoder->first_free = decoder->clear_code + 2;
  decoder->free_code = decoder->first_free;
  decoder->init_code_size = ++decoder->code_size;
  decoder->max_code = 1 << decoder->code_size;
  decoder->bits_in = 8;

  decoder->x = descriptor.image_left;
  decoder->y = descriptor.image_top;
  decoder->img = img;

  do {
    decoder->code = read_code( decoder );
    if( decoder->code == decoder->eoi_code ) {
      break;
    } else if( decoder->code == decoder->clear_code ) {
      decoder->free_code = decoder->first_free;
      decoder->code_size = decoder->init_code_size;
      decoder->max_code = 1 << decoder->code_size;
      decoder->code = read_code( decoder );
      decoder->old_code = decoder->code;
      next_pixel( decoder, decoder->code );
    } else {
      if( decoder->code < decoder->free_code ) {
	decoder->suffix[decoder->free_code] = out_string( decoder, decoder->code );
      } else {
	decoder->suffix[decoder->free_code] = out_string( decoder, decoder->old_code );
	next_pixel( decoder, decoder->suffix[decoder->free_code] );
      }
      decoder->prefix[decoder->free_code++] = decoder->old_code;
      if( decoder->free_code >= decoder->max_code && decoder->code_size < 12 ) {
	decoder->code_size++;
	decoder->max_code <<= 1;
      }
      decoder->old_code = decoder->code;
    }
  } while( decoder->code != decoder->eoi_code );

  fclose( gif_file );
  free(decoder);
  printf("\n");

  return img;
}

void free_image(struct image *img)
{
  if( img == NULL ) {
    return;
  }
  if( img->data != NULL ) {
    farfree( img->data );
  }
  free( img );
}
