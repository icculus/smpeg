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
#include <string.h>

#include "SDL_timer.h"

#include "MPEGring.h"
#include "MPEGerror.h"


#undef _THIS
#define _THIS MPEG_ring *self
#undef METH
#define METH(m) MPEG_ring_##m

MPEG_ring*
METH(init) (_THIS, Uint32 size, Uint32 count)
{
    Uint32 tSize;

    MAKE_OBJECT(MPEG_ring);

    tSize = (size + sizeof(Uint32)) * count;
    if( tSize )
    {
        self->begin = (Uint8 *) malloc( tSize );
        self->timestamps = (double *) malloc( sizeof(double)*count );
    }
    else
        self->begin = 0;

    if( self->begin && count )
    {
        self->end   = self->begin + tSize;
        self->read  = self->begin;
        self->write = self->begin;
        self->timestamp_read  = self->timestamps;
        self->timestamp_write = self->timestamps;
        self->bufSize  = size;
        
        self->readwait = SDL_CreateSemaphore(0);
        self->writewait = SDL_CreateSemaphore(count);
    }
    else
    {
        self->end   = 0;
        self->read  = 0;
        self->write = 0;
        self->bufSize  = 0;

        self->readwait = 0;
    }

    if ( self->begin && self->readwait && self->writewait ) {
        self->active = 1;
    }

  return self;
}

/* Release any waiting threads on the ring so they can be cleaned up.
   The ring isn't valid after this call, so when threads are done you
   should call MPRing_sdelete() on the ring.
 */
void
METH(ReleaseThreads) (_THIS)
{
    /* Let the threads know that the ring is now inactive */
    self->active = 0;

    if ( self->readwait ) {
        while ( SDL_SemValue(self->readwait) == 0 ) {
            SDL_SemPost(self->readwait);
        }
    }
    if ( self->writewait ) {
        while ( SDL_SemValue(self->writewait) == 0 ) {
            SDL_SemPost(self->writewait);
        }
    }
}


void
METH(destroy) (_THIS)
{
    if( self )
    {
        /* Free up the semaphores */
        MPEG_ring_ReleaseThreads(self);

	/* Destroy the semaphores */
	if( self->readwait )
	{
	    SDL_DestroySemaphore( self->readwait );
	    self->readwait = 0;
	}
	if( self->writewait )
	{
	    SDL_DestroySemaphore( self->writewait );
	    self->writewait = 0;
	}

        /* Free data buffer */
        if ( self->begin ) {
            free( self->begin );
            free( self->timestamps );
            self->begin = 0;
            self->timestamps = 0;
        }
    }
}

/*
  Returns free buffer of self->bufSize to be filled with data.
  Zero is returned if there is no free buffer.
*/

Uint8 *
METH(NextWriteBuffer) (_THIS)
{
    Uint8 *buffer;

    buffer = 0;
    if ( self->active ) {
	//printf("Waiting for write buffer (%d available)\n", SDL_SemValue(self->writewait));
        SDL_SemWait(self->writewait);
	//printf("Finished waiting for write buffer\n");
	if ( self->active ) {
            buffer = self->write + sizeof(Uint32);
        }
    }
    return buffer;
}


/*
  Call this when the buffer returned from MPRing_nextWriteBuffer() has
  been filled.  The passed length must not be larger than self->bufSize.
*/

void
METH(WriteDone) (_THIS, Uint32 len, double timestamp)
{
    if ( self->active ) {
#ifdef NO_GRIFF_MODS
        assert(len <= self->bufSize);
#else
	if ( len > self->bufSize )
            len = self->bufSize;
#endif
        *((Uint32*) self->write) = len;

        self->write += self->bufSize + sizeof(Uint32);
        *(self->timestamp_write++) = timestamp;
        if( self->write >= self->end )
        {
            self->write = self->begin;
            self->timestamp_write = self->timestamps;
        }
//printf("Finished write buffer of %u bytes, making available for reads (%d+1 available for reads)\n", len, SDL_SemValue(self->readwait));
        SDL_SemPost(self->readwait);
    }
}


/*
  Returns the number of bytes in the next ring buffer and sets the buffer
  pointer to this buffer.  If there is no buffer ready then the buffer
  pointer is not changed and zero is returned.
*/

Uint32
METH(NextReadBuffer) (_THIS, Uint8** buffer)
{
    Uint32 size;

    size = 0;
    if ( self->active ) {
        /* Wait for a buffer to become available */
//printf("Waiting for read buffer (%d available)\n", SDL_SemValue(self->readwait));
        SDL_SemWait(self->readwait);
//printf("Finished waiting for read buffer\n");
	if ( self->active ) {
            size = *((Uint32*) self->read);
            *buffer = self->read + sizeof(Uint32);
        }
    }
    return size;
}

/*
  Call this when you have used some of the buffer previously returned by
  MPRing_nextReadBuffer(), and want to update it so the rest of the data
  is returned with the next call to MPRing_nextReadBuffer().
*/

double
METH(ReadTimeStamp) (_THIS)
{
  if(self->active)
    return *self->timestamp_read;
  return(0);
}

void
METH(ReadSome) (_THIS, Uint32 used)
{
    Uint8 *data;
    Uint32 oldlen;
    Uint32 newlen;

    if ( self->active ) {
        data = self->read + sizeof(Uint32);
        oldlen = *((Uint32*) self->read);
        newlen = oldlen - used;
        memmove(data, data+used, newlen);
        *((Uint32*) self->read) = newlen;
//printf("Reusing read buffer (%d+1 available)\n", SDL_SemValue(self->readwait));
        SDL_SemPost(self->readwait);
    }
}

/*
  Call this when the buffer returned from MPRing_nextReadBuffer() is no
  longer needed.  This assumes there is only one read thread and one write
  thread for a particular ring buffer.
*/

void
METH(ReadDone) (_THIS)
{
    if ( self->active ) {
        self->read += self->bufSize + sizeof(Uint32);
        self->timestamp_read++;
        if( self->read >= self->end )
        {
            self->read = self->begin;
            self->timestamp_read = self->timestamps;
        }
//printf("Finished read buffer, making available for writes (%d+1 available for writes)\n", SDL_SemValue(self->writewait));
        SDL_SemPost(self->writewait);
    }
}






#if 0
Uint32 MPEG_ring_BufferSize (MPEG_ring *ring)
{
  return (self->bufSize);
}

int MPEG_ring_BuffersWritten (MPEG_ring *ring)
{
    return (SDL_SemValue(self->readwait));
}
#endif /* 0 */

/* EOF */
