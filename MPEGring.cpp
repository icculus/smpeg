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


#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include <SDL/SDL_timer.h>

#include "MPEGring.h"


MPEG_ring:: MPEG_ring( Uint32 size, int count )
{
    Uint32 tSize;

    /* Set up the 'ring' pointer for all the old C code */
    ring = this;

    /* Our thread waiting code assumes that the writer will not fill all
       buffers within its timeslice, nor will the reader drain all buffers
       within its timeslice.  If this happens, potential deadlock could
       occur.
       Set this value to the number of buffers that could be processed.
     */
    if ( count <= 1 ) {
        fprintf(stderr, "MPRing: Potential deadlock - use more buffers!\n");
    }
    tSize = (size + sizeof(Uint32)) * count;
    if( tSize )
        ring->begin = (Uint8 *) malloc( tSize );
    else
        ring->begin = 0;

    if( ring->begin )
    {
        ring->end   = ring->begin + tSize;
        ring->read  = ring->begin;
        ring->write = ring->begin;
        ring->bufSize  = size;
        ring->bufCount = count;
        
        ring->readwait = SDL_CreateMutex();
        SDL_mutexP(ring->readwait);
        ring->read_waiting = 0;
        ring->writewait = SDL_CreateMutex();
        SDL_mutexP(ring->writewait);
        ring->write_waiting = 0;
    }
    else
    {
        ring->end   = 0;
        ring->read  = 0;
        ring->write = 0;
        ring->bufSize  = 0;
        ring->bufCount = 0;

        ring->readwait = 0;
        ring->read_waiting = 0;
        ring->writewait = 0;
        ring->write_waiting = 0;
    }

    ring->usedCount = 0;
    ring->active = 1;
    ring->reader_active = 0;
    ring->writer_active = 0;
}

/* Release any waiting threads on the ring so they can be cleaned up.
   The ring isn't valid after this call, so when threads are done you
   should call MPRing_sdelete() on the ring.
 */
void
MPEG_ring:: ReleaseThreads( void )
{
    /* Let the threads know that the ring is now inactive */
    ring->active = 0;

    /* Wait for the threads to get off of the ring */
    while ( ring->reader_active || ring->writer_active ) {
        if ( ring->reader_active ) {
            SDL_mutexV(ring->readwait);
        }
        if ( ring->writer_active ) {
            SDL_mutexV(ring->writewait);
        }
        SDL_Delay(10);
    }
}


MPEG_ring:: ~MPEG_ring( void )
{
    if( ring )
    {
        /* Free up the mutexes */
        if( ring->readwait )
        {
            SDL_DestroyMutex( ring->readwait );
            ring->readwait = 0;
        }
        if( ring->writewait )
        {
            SDL_DestroyMutex( ring->writewait );
            ring->writewait = 0;
        }

        /* Free data buffer */
        if ( ring->begin ) {
            free( ring->begin );
            ring->begin = 0;
        }
    }
}

/*
  Returns free buffer of ring->bufSize to be filled with data.
  Zero is returned if there is no free buffer.
*/

Uint8 *
MPEG_ring:: NextWriteBuffer( void )
{
    if ( ring->active ) {
        if ( ring->usedCount >= ring->bufCount ) {
            ring->write_waiting = 1;
//fprintf(stderr, "Waiting for write buffers on ring %p\n", ring);
            ring->writer_active = 1;
            SDL_mutexP(ring->writewait);
            ring->writer_active = 0;
            if ( ! ring->active ) {
                goto inactive;
            }
        }
        return( ring->write + sizeof(Uint32) );
    }
inactive:
    return(NULL);
}


/*
  Call this when the buffer returned from MPRing_nextWriteBuffer() has
  been filled.  The passed length must not be larger than ring->bufSize.
*/

void
MPEG_ring:: WriteDone( Uint32 len )
{
    if ( ring->active ) {
        assert(len <= ring->bufSize);
        *((Uint32*) ring->write) = len;

        ring->write += ring->bufSize + sizeof(Uint32);
        if( ring->write >= ring->end )
        {
            ring->write = ring->begin;
        }
        ring->usedCount++;

        if ( ring->read_waiting ) {
//fprintf(stderr, "Allowing read thread to continue on ring %p\n", ring);
            ring->read_waiting = 0;
            SDL_mutexV(ring->readwait);
        }
    }
}


/*
  Returns the number of bytes in the next ring buffer and sets the buffer
  pointer to this buffer.  If there is no buffer ready then the buffer
  pointer is not changed and zero is returned.
*/

Uint32
MPEG_ring:: NextReadBuffer( Uint8** buffer )
{
    if ( ring->active ) {
        if ( ! ring->usedCount ) {
            ring->read_waiting = 1;
//fprintf(stderr, "Waiting for read buffers on ring %p\n", ring);
            ring->reader_active = 1;
            SDL_mutexP(ring->readwait);
            ring->reader_active = 0;
            if ( ! ring->active ) {
                goto inactive;
            }
        }

        *buffer = ring->read + sizeof(Uint32);
        return( *((Uint32*) ring->read) );
    }
inactive:
    return(0);
}

/*
  Call this when you have used some of the buffer previously returned by
  MPRing_nextReadBuffer(), and want to update it so the rest of the data
  is returned with the next call to MPRing_nextReadBuffer().
*/

void
MPEG_ring:: ReadSome( Uint32 used )
{
    Uint8 *data;
    Uint32 oldlen;
    Uint32 newlen;

    if ( ring->active ) {
        data = ring->read + sizeof(Uint32);
        oldlen = *((Uint32*) ring->read);
        newlen = oldlen - used;
        memcpy(data, data+used, newlen);
        *((Uint32*) ring->read) = newlen;
    }
}

/*
  Call this when the buffer returned from MPRing_nextReadBuffer() is no
  longer needed.  This assumes there is only one read thread and one write
  thread for a particular ring buffer.
*/

void
MPEG_ring:: ReadDone( void )
{
    if ( ring->active ) {
        ring->read += ring->bufSize + sizeof(Uint32);
        if( ring->read >= ring->end )
        {
            ring->read = ring->begin;
        }
        ring->usedCount--;

        if ( ring->write_waiting ) {
//fprintf(stderr, "Allowing write thread to continue on ring %p\n", ring);
            ring->write_waiting = 0;
            SDL_mutexV(ring->writewait);
        }
    }
}


/* EOF */
