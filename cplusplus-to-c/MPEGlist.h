/* bufferlist.h */

/* A class for buffering the I/O and allow multiple streams to read the data
   asynchronously */

#ifndef _MPEGLIST_H_
#define _MPEGLIST_H_

#include "SDL.h"

typedef struct MPEGlist {
  struct MPEGlist *next;
  struct MPEGlist *prev;
  Uint32 lock;
  Uint8 *data;
  Uint32 size;
  double TimeStamp;
} MPEGlist;

MPEGlist* MPEGlist_new();
void MPEGlist_destroy(MPEGlist*);
MPEGlist* MPEGlist_Alloc(MPEGlist*, Uint32);
void MPEGlist_Lock(MPEGlist*);
void MPEGlist_Unlock(MPEGlist*);
void* MPEGlist_Buffer(MPEGlist*);
Uint32 MPEGlist_Size(MPEGlist*);
MPEGlist* MPEGlist_Next(MPEGlist*);
MPEGlist* MPEGlist_Prev(MPEGlist*);
Uint32 MPEGlist_IsLocked(MPEGlist*);

#endif
