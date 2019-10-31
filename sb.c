#include <dos.h>
#include <stdio.h>
#include <stdlib.h>
#include <mem.h>

#include "sb.h"

void single_cycle_playback();
void read_buffer( short buffer );

short sb_base; /* default 220h */
char sb_irq; /* default 7 */
char sb_dma; /* default 1 */
void interrupt ( *old_irq )();

short r_buffer;
FILE *raw_file;
long file_size;

volatile int playing;
volatile long to_be_played;
volatile long to_be_read;
unsigned char *dma_buffer;
short page;
short offset;

#define DMA_BLOCK_LENGTH 0x7FFF
#define SB_BLOCK_LENGTH 0x3FFF

#define SB_RESET 0x6
#define SB_READ_DATA 0xA
#define SB_READ_DATA_STATUS 0xE
#define SB_WRITE_DATA 0xC

#define SB_PAUSE_PLAYBACK 0xD0
#define SB_ENABLE_SPEAKER 0xD1
#define SB_DISABLE_SPEAKER 0xD3
#define SB_SET_PLAYBACK_FREQUENCY 0x40
#define SB_SINGLE_CYCLE_PLAYBACK 0x14
#define SB_START_AUTOINIT_PLAYBACK 0x1C
#define SB_SET_BLOCK_SIZE 0x48
#define SB_STOP_AUTOINIT_PLAYBACK 0xDA

#define MASK_REGISTER 0x0A
#define MODE_REGISTER 0x0B
#define MSB_LSB_FLIP_FLOP 0x0C
#define DMA_CHANNEL_0 0x87
#define DMA_CHANNEL_1 0x83
#define DMA_CHANNEL_3 0x82

int reset_dsp(short port)
{
  outportb( port + SB_RESET, 1 );
  delay(10);
  outportb( port + SB_RESET, 0 );
  delay(10);
  if( ((inportb(port + SB_READ_DATA_STATUS) & 0x80) == 0x80)
    && (inportb(port + SB_READ_DATA) == 0x0AA )
   ) {
    sb_base = port;
    return 1;
  }
  return 0;
}

void write_dsp( unsigned char command )
{
  while( (inportb( sb_base + SB_WRITE_DATA ) & 0x80) == 0x80 );
  outportb( sb_base + SB_WRITE_DATA, command );
}

int sb_detect()
{
  char temp;
  char *BLASTER;
  sb_base = 0;

  /* possible values: 210, 220, 230, 240, 250, 260, 280 */
  for( temp = 1; temp < 9; temp++ ) {
    if( temp != 7 ) {
      if( reset_dsp( 0x200 + (temp << 4) ) ) {
	break;
      }
    }
  }
  if( temp == 9 ) {
    return 0;
  }

  BLASTER = getenv("BLASTER");
  sb_dma = 0;
  for( temp = 0; temp < strlen( BLASTER ); ++temp ) {
    if((BLASTER[temp] | 32) == 'd') {
      sb_dma = BLASTER[temp + 1] - '0';
    }
  }

  for( temp = 0; temp < strlen( BLASTER ); ++temp ) {
    if((BLASTER[temp] | 32) == 'i') {
      sb_irq = BLASTER[temp + 1] - '0';
      if( BLASTER[temp + 2] != ' ' ) {
	sb_irq = sb_irq * 10 + BLASTER[ temp + 2 ] - '0';
      }
    }
  }

  return sb_base != 0;
}

void interrupt sb_irq_handler()
{
  inportb(sb_base + SB_READ_DATA_STATUS);
  outportb( 0x20, 0x20 );
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    outportb( 0xA0, 0x20 );
  }
  if( playing ) {
    to_be_played -= 16384;
    if( to_be_played > 0 ) {
      read_buffer( r_buffer );
      if( to_be_played <= 16384 ) {
	r_buffer ^= 1;
	single_cycle_playback();
      } else if( to_be_played <= 32768 ) {
	write_dsp( SB_STOP_AUTOINIT_PLAYBACK );
      } else {
	/* All is good! continue playing! */
      }
    } else {
      playing = 0;
    }
  }
  r_buffer ^= 1;
}

void init_irq()
{
  /* save the old irq vector */
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    if( sb_irq == 2) old_irq = getvect( 0x71 );
    if( sb_irq == 10) old_irq = getvect( 0x72 );
    if( sb_irq == 11) old_irq = getvect( 0x73 );
  } else {
    old_irq = getvect( sb_irq + 8 );
  }
  /* set our own irq vector */
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    if( sb_irq == 2) setvect( 0x71, sb_irq_handler );
    if( sb_irq == 10) setvect( 0x72, sb_irq_handler );
    if( sb_irq == 11) setvect( 0x73, sb_irq_handler );
  } else {
    setvect( sb_irq + 8, sb_irq_handler );
  }
  /* enable irq with the mainboard's PIC */
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    if( sb_irq == 2) outportb( 0xA1, inportb( 0xA1 ) & 253 );
    if( sb_irq == 10) outportb( 0xA1, inportb( 0xA1 ) & 251 );
    if( sb_irq == 11) outportb( 0xA1, inportb( 0xA1 ) & 247 );
    outportb( 0x21, inportb( 0x21 ) & 251 );
  } else {
    outportb( 0x21, inportb( 0x21 ) & !(1 << sb_irq) );
  }
}

void deinit_irq()
{
  /* restore the old irq vector */
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    if( sb_irq == 2) setvect( 0x71, old_irq );
    if( sb_irq == 10) setvect( 0x72, old_irq );
    if( sb_irq == 11) setvect( 0x73, old_irq );
  } else {
    setvect( sb_irq + 8, old_irq );
  }
  /* enable irq with the mainboard's PIC */
  if( sb_irq == 2 || sb_irq == 10 || sb_irq == 11 ) {
    if( sb_irq == 2) outportb( 0xA1, inportb( 0xA1 ) | 2 );
    if( sb_irq == 10) outportb( 0xA1, inportb( 0xA1 ) | 4 );
    if( sb_irq == 11) outportb( 0xA1, inportb( 0xA1 ) | 8 );
    outportb( 0x21, inportb( 0x21 ) | 4 );
  } else {
    outportb( 0x21, inportb( 0x21 ) & (1 << sb_irq) );
  }
}

void assign_dma_buffer()
{
  unsigned char* temp_buf;
  long linear_address;
  short page1, page2;

  temp_buf = (char *) malloc(32768);
  linear_address = FP_SEG(temp_buf);
  linear_address = (linear_address << 4)+FP_OFF(temp_buf);
  page1 = linear_address >> 16;
  page2 = (linear_address + 32767) >> 16;
  if( page1 != page2 ) {
    dma_buffer = (char *)malloc(32768);
    free( temp_buf );
  } else {
    dma_buffer = temp_buf;
  }
  linear_address = FP_SEG(dma_buffer);
  linear_address = (linear_address << 4)+FP_OFF(dma_buffer);
  page = linear_address >> 16;
  offset = linear_address & 0xFFFF;
}

void sb_init()
{
  init_irq();
  assign_dma_buffer();
  write_dsp( SB_ENABLE_SPEAKER );
}

void sb_deinit()
{
  write_dsp( SB_DISABLE_SPEAKER );
  free( dma_buffer );
  deinit_irq();
}

void single_cycle_playback()
{
  playing = 1;
  /* program the DMA controller */
  outportb( MASK_REGISTER, 4 | sb_dma );
  outportb( MSB_LSB_FLIP_FLOP, 0 );
  outportb( MODE_REGISTER, 0x48 | sb_dma );
  outportb( sb_dma << 1, offset & 0xFF );
  outportb( sb_dma << 1, offset >> 8 );
  switch( sb_dma ) {
  case 0:
    outportb( DMA_CHANNEL_0, page );
    break;
  case 1:
    outportb( DMA_CHANNEL_1, page );
    break;
  case 3:
    outportb( DMA_CHANNEL_3, page );
    break;
  }
  outportb((sb_dma << 1) + 1, to_be_played & 0xFF );
  outportb((sb_dma << 1) + 1, to_be_played >> 8 );
  outportb(MASK_REGISTER, sb_dma);
  write_dsp(SB_SINGLE_CYCLE_PLAYBACK);
  write_dsp(to_be_played & 0xFF);
  write_dsp(to_be_played >> 8);
  to_be_played = 0;
}

void sb_single_play( const char *file_name )
{
  memset(dma_buffer, 0, 32768);
  raw_file = fopen(file_name, "rb");
  fseek(raw_file, 0L, SEEK_END);
  file_size = ftell( raw_file );
  fseek(raw_file, 0L, SEEK_SET);
  fread(dma_buffer, 1, file_size, raw_file);
  write_dsp(SB_SET_PLAYBACK_FREQUENCY);
  write_dsp(256-1000000/11000);
  to_be_played = file_size;
  single_cycle_playback();
}

void sb_stop()
{
  write_dsp( SB_PAUSE_PLAYBACK );
  playing = 0;
  fclose( raw_file );
}

void read_buffer( short buffer )
{
  const int buf_off = buffer << 14;
  if( to_be_read <= 0 ) {
    return;
  }
  if( to_be_read < 16384 ) {
    memset( dma_buffer + buf_off, 128, 16384 );
    fread( dma_buffer + buf_off, 1, to_be_read, raw_file );
    to_be_read = 0;
  } else {
    fread( dma_buffer + buf_off, 1, 16384, raw_file );
    to_be_read -= 16384;
  }
}

void auto_init_playback()
{
  /* program the DMA controller */
  outportb( MASK_REGISTER, 4 | sb_dma );
  outportb( MSB_LSB_FLIP_FLOP, 0 );
  /*
   * 0x58 + DMA = 010110xx
   *              ^^ block mode
   *                ^^ auto init
   *                  ^^ read operation (SB reads from memory)
   */
  outportb( MODE_REGISTER, 0x58 | sb_dma );
  outportb( sb_dma << 1, offset & 0xFF );
  outportb( sb_dma << 1, offset >> 8 );
  switch( sb_dma ) {
  case 0:
    outportb( DMA_CHANNEL_0, page );
    break;
  case 1:
    outportb( DMA_CHANNEL_1, page );
    break;
  case 3:
    outportb( DMA_CHANNEL_3, page );
    break;
  }
  outportb((sb_dma << 1) + 1, DMA_BLOCK_LENGTH & 0xFF );
  outportb((sb_dma << 1) + 1, DMA_BLOCK_LENGTH >> 8 );
  outportb(MASK_REGISTER, sb_dma);
  write_dsp(SB_SET_BLOCK_SIZE);
  write_dsp(SB_BLOCK_LENGTH & 0xFF);
  write_dsp(SB_BLOCK_LENGTH >> 8);
  write_dsp(SB_START_AUTOINIT_PLAYBACK);
}

void sb_auto_play( const char *file_name )
{
  if( !dma_buffer ) {
    return;
  }
  r_buffer = 0;
  memset( dma_buffer, 0, 32768 );
  raw_file = fopen( file_name, "rb" );
  if( !raw_file ) {
    printf("error: file not found %s\n", file_name );
    return;
  }
  fseek( raw_file, 0L, SEEK_END );
  file_size = ftell( raw_file );
  fseek( raw_file, 0L, SEEK_SET );
  to_be_read = to_be_played = file_size;
  printf("to be read: %lu\n", to_be_read );
  write_dsp( SB_SET_PLAYBACK_FREQUENCY );
  write_dsp( 256-1000000/11000 );
  write_dsp( SB_ENABLE_SPEAKER );
  read_buffer( 0 );
  read_buffer( 1 );
  if( to_be_read > 0 ) {
    printf( "auto init playback...\n" );
    auto_init_playback();
  } else {
    printf( "single cycle playback...\n" );
    single_cycle_playback();
  }
  playing = 1;
}

#if 0

int main()
{
  int sb_detected = 0;

  sb_detected = sb_detect();
  if( !sb_detected ) {
    printf("SoundBlaster not found!\n");
    return 1;
  } else {
    printf("SoundBlaster found at A%x I%u D%u.\n", sb_base, sb_irq, sb_dma );
  }

  sb_init();
  sb_auto_play("lwhome.raw");
  while( playing && !kbhit() )
    ;
  sb_stop();
  sb_deinit();
  return 0;
}

#endif

