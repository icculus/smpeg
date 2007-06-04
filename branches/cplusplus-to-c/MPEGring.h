/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* A ring-buffer class for multi-threaded applications.
   This assumes a single reader and a single writer, with blocking reads.
 */

#ifndef _MPEGRING_H
#define _MPEGRING_H

#include "SDL_types.h"
#include "SDL_thread.h"

struct MPEG_ring {
    /* read only */
    Uint32 bufSize;
    
    /* private */
    Uint8 *begin;
    Uint8 *end;

    double *timestamps;
    double *timestamp_read;
    double *timestamp_write;
 
    Uint8 *read;
    Uint8 *write;

    /* For read/write synchronization */
    int active;
    struct SDL_semaphore *readwait;
    struct SDL_semaphore *writewait;
};

typedef struct MPEG_ring MPEG_ring;


#undef _THIS
#define _THIS MPEG_ring* self

/* Create a ring with 'count' buffers, each 'size' bytes long */
MPEG_ring * MPEG_ring_init (_THIS, Uint32, Uint32);

/* Release any waiting threads on the ring so they can be cleaned up.
   The ring isn't valid after this call, so when threads are done you
   should call MPRing_sdelete() on the ring.
 */
void MPEG_ring_ReleaseThreads (MPEG_ring*);

/* Destroy a ring after all threads are no longer using it */
void MPEG_ring_destroy (MPEG_ring*);

#if 0
/* Returns the maximum size of each buffer */
Uint32 MPEG_ring_BufferSize (MPEG_ring*);

/* Returns how many buffers have available data */
int MPEG_ring_BuffersWritten (MPEG_ring*);


//Uint32 MPEG_ring_BufferSize (MPEG_ring *ring)
//{
//  return (ring->bufSize);
//}
//
//int MPEG_ring_BuffersWritten (MPEG_ring *ring)
//{
//    return (SDL_SemValue(ring->readwait));
//}


#else /* 0 */
/* Returns the maximum size of each buffer */
#define MPEG_ring_BufferSize(ring) (ring->bufSize)

/* Returns how many buffers have available data */
#define MPEG_ring_BuffersWritten(ring) (ring->readwait)

#endif /* 0 */

/* Reserve a buffer for writing in the ring */
Uint8 *MPEG_ring_NextWriteBuffer (MPEG_ring*);

/* Release a buffer, written to in the ring */
void MPEG_ring_WriteDone (MPEG_ring*, Uint32, double);

/* Reserve a buffer for reading in the ring */
Uint32 MPEG_ring_NextReadBuffer (MPEG_ring*, Uint8**);

/* Read the timestamp of the current buffer */
double MPEG_ring_ReadTimeStamp (MPEG_ring*);

/* Release a buffer having read some of it */
void MPEG_ring_ReadSome (MPEG_ring*, Uint32);

/* Release a buffer having read all of it */
void MPEG_ring_ReadDone (MPEG_ring*);


#endif /* _MPEGRING_H */
