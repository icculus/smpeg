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

/* A ring-buffer class for multi-threaded applications */

#ifndef _MPEGRING_H
#define _MPEGRING_H

#include "SDL_types.h"
#include "SDL_thread.h"
#include "SDL_mutex.h"

class MPEG_ring {
public:
    /* Create a ring with 'count' buffers, each 'size' bytes long */
    MPEG_ring(Uint32 size, int count = 32);

    /* Release any waiting threads on the ring so they can be cleaned up.
       The ring isn't valid after this call, so when threads are done you
       should call MPRing_sdelete() on the ring.
     */
    void ReleaseThreads(void);

    /* Destroy a ring after all threads are no longer using it */
    virtual ~MPEG_ring();

    /* Returns the maximum size of each buffer */
    Uint32 BufferSize( void ) {
        return(bufSize);
    }
    /* Returns how many buffers have available data */
    int BuffersWritten(void) {
        return(usedCount);
    }

    /* Reserve a buffer for writing in the ring */
    Uint8 *NextWriteBuffer( void );

    /* Release a buffer, written to in the ring */
    void WriteDone( Uint32 len );

    /* Reserve a buffer for reading in the ring */
    Uint32 NextReadBuffer( Uint8** buffer );

    /* Release a buffer having read some of it */
    void ReadSome( Uint32 used );

    /* Release a buffer having read all of it */
    void ReadDone( void );

protected:
    MPEG_ring *ring;    /* Converted from C code, an alias for 'this' */

    /* read only */
    Uint32 bufSize;
    
    /* private */
    int bufCount;
    int usedCount;

    Uint8 *begin;
    Uint8 *end;

    Uint8 *read;
    Uint8 *write;

    /* For read/write synchronization */
    int active;
    int reader_active;
    SDL_mutex* readwait;
    int read_waiting;
    SDL_mutex* writewait;
    int write_waiting;
    int writer_active;
};

#endif /* _MPEGRING_H */
