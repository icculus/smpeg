/* bufferlist.h */

/* A class for buffering the I/O and allow multiple streams to read the data
   asynchronously */

#ifndef _MPEGLIST_H_
#define _MPEGLIST_H_

#include "SDL.h"

struct MPEGlist {
  struct MPEGlist *next;
  struct MPEGlist *prev;
  Uint32 lock;
  Uint8 *data;
  Uint32 size;
  double TimeStamp;
};

typedef struct MPEGlist MPEGlist;

#undef _THIS
#define _THIS MPEGlist *self
#undef METH
#define METH(m) MPEGlist_##m

MPEGlist* METH(init) (_THIS);
void METH(destroy) (_THIS);
MPEGlist* METH(Alloc) (_THIS, Uint32);
void METH(Lock) (_THIS);
void METH(Unlock) (_THIS);
void* METH(Buffer) (_THIS);
Uint32 METH(Size) (_THIS);
MPEGlist* METH(Next) (_THIS);
MPEGlist* METH(Prev) (_THIS);
Uint32 METH(IsLocked) (_THIS);

#endif
