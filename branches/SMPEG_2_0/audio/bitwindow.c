/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Bitwindow.cc
// It's bit reservior for MPEG layer 3

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "MPEGaudio.h"

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int Mpegbitwindow_getbits (Mpegbitwindow *self, int bits)
{
  union
  {
    char store[4];
    int current;
  }u;
  int bi;

  if(!bits)return 0;

  u.current=0;
  bi=(self->bitindex&7);
  //  u.store[_KEY]=self->buffer[(self->bitindex>>3)&(WINDOWSIZE-1)]<<bi;
  u.store[_KEY]=self->buffer[self->bitindex>>3]<<bi;
  bi=8-bi;
  self->bitindex+=bi;

  while(bits)
  {
    if(!bi)
    {
      //      u.store[_KEY]=buffer[(bitindex>>3)&(WINDOWSIZE-1)];
      u.store[_KEY]=self->buffer[self->bitindex>>3];
      self->bitindex+=8;
      bi=8;
    }

    if(bits>=bi)
    {
      u.current<<=bi;
      bits-=bi;
      bi=0;
    }
    else
    {
      u.current<<=self->bits;
      bi-=bits;
      bits=0;
    }
  }
  self->bitindex-=bi;

  return (u.current>>8);
}
