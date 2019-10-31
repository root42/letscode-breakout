#include <dos.h>

#include "mouse.h"

#define MOUSE_INT 0x33

#define INIT_MOUSE 0x00
#define SHOW_MOUSE 0x01
#define HIDE_MOUSE 0x02
#define GET_MOUSE_STATUS 0x03

int init_mouse()
{
  union REGS regs;
  regs.x.ax = INIT_MOUSE;
  int86( MOUSE_INT, &regs, &regs );
  return 0xFFFF == regs.x.ax;
}

void show_mouse()
{
  union REGS regs;
  regs.x.ax = SHOW_MOUSE;
  int86( MOUSE_INT, &regs, &regs );
}

void hide_mouse()
{
  union REGS regs;
  regs.x.ax = HIDE_MOUSE;
  int86( MOUSE_INT, &regs, &regs );
}

void get_mouse( int *x, int *y, int *left, int *right )
{
  union REGS regs;
  regs.x.ax = GET_MOUSE_STATUS;
  int86( MOUSE_INT, &regs, &regs );
  *x = regs.x.cx / 2;
  *y = regs.x.dx;
  *left = regs.x.bx & 0x1;
  *right = regs.x.bx & 0x2;
}


