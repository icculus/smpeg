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

MPEGlist* MPEGlist_init (_THIS);
void MPEGlist_destroy (_THIS);
void MPEGlist_delete (_THIS);
MPEGlist* MPEGlist_Alloc (_THIS, Uint32);
void MPEGlist_Lock (_THIS);
void MPEGlist_Unlock (_THIS);
void* MPEGlist_Buffer (_THIS);
Uint32 MPEGlist_Size (_THIS);
MPEGlist* MPEGlist_Next (_THIS);
MPEGlist* MPEGlist_Prev (_THIS);
Uint32 MPEGlist_IsLocked (_THIS);

#endif
