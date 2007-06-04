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

/* The generic MPEG stream class */

#include <string.h>

#include "MPEG.h"
#include "MPEGstream.h"
#include "video/video.h"

/* This is the limit of the quantity of pre-read data */
#define MAX_QUEUE (256 * 1024)

#undef _THIS
#define _THIS MPEGstream *self

MPEGstream *
MPEGstream_init (_THIS, MPEGsystem *System, Uint8 Streamid)
{
    MAKE_OBJECT(MPEGstream);
    
    self->system = System;
    self->streamid = Streamid;

    self->br = MPEGlist_init(NULL);
    self->cleareof = true;
        
    self->data = 0;
    self->stop = 0;
    self->pos = 0;
        
    self->preread_size = 0;
    self->enabled = true;
    self->mutex = SDL_CreateMutex();

    return self;
}

void
MPEGstream_destroy (_THIS)
{
    MPEGlist *newbr;

    SDL_DestroyMutex(self->mutex);
    
    /* Free the list */
    for(newbr = self->br; MPEGlist_Prev(newbr); newbr = MPEGlist_Prev(newbr));

    while(MPEGlist_Next(newbr))
    {
        newbr = MPEGlist_Next(newbr);
        MPEGlist_delete(MPEGlist_Prev(newbr));
    }
    MPEGlist_delete(newbr);
    self->br = NULL;
}

void
MPEGstream_reset_stream (_THIS)
{
    MPEGlist *newbr;

    SDL_mutexP(self->mutex);
    /* Seek the first buffer */
    for(newbr = self->br; MPEGlist_Prev(newbr); newbr = MPEGlist_Prev(newbr));
    
    /* Free buffers  */
    while(MPEGlist_Next(newbr))
    {
        newbr = MPEGlist_Next(newbr);
        MPEGlist_delete(MPEGlist_Prev(newbr));
    }
    MPEGlist_delete(newbr);
    
    self->br = MPEGlist_init(NULL);
    self->cleareof = true;
    self->data = 0;
    self->stop = 0;
    self->pos = 0;
    self->preread_size = 0;
    SDL_mutexV(self->mutex);
}

void
MPEGstream_rewind_stream (_THIS)
{
    /* Note that this will rewind all streams, and other streams than this one */
    /* will finish reading their prebuffured data (they are not reseted) */
    /* This should works because there are always sequence start codes or */
    /* audio start codes at the beginning of the streams */
    /* Of course, this won't work on network streams */
    
    /* Restart the system */
    MPEGsystem_Rewind(self->system);
}

bool
MPEGstream_next_system_buffer (_THIS)
{
    bool has_data = true;
    
    /* No more buffer ? */
    while(has_data && !MPEGlist_Next(self->br))
    {
        SDL_mutexV(self->mutex);
        MPEGsystem_RequestBuffer(self->system);
        has_data = MPEGsystem_Wait(self->system);
        SDL_mutexP(self->mutex);
    }
    
    if (has_data && (MPEGlist_Size(self->br) || self->cleareof)) {
        self->cleareof = false;
        self->br = MPEGlist_Next(self->br);
        self->preread_size -= MPEGlist_Size(self->br);
    }
    return(has_data);
}

bool
MPEGstream_next_packet (_THIS, bool recurse, bool update_timestamp)
{
    SDL_mutexP(self->mutex);
    
    /* Unlock current buffer */
    MPEGlist_Unlock(self->br);
    
    /* Check for the end of stream mark */
    MPEGstream_next_system_buffer(self);
    if (MPEGstream_eof(self)) {
        /* Report eof */
        SDL_mutexV(self->mutex);
        return(false);
    }
    
    /* Lock the buffer */
    MPEGlist_Lock(self->br);
    
    /* Make sure that we have read buffers in advance if possible */
    if (self->preread_size < MAX_QUEUE)
        MPEGsystem_RequestBuffer(self->system);
    
    /* Update stream datas */
    self->data = (Uint8 *)MPEGlist_Buffer(self->br);
    self->stop = self->data + MPEGlist_Size(self->br);
    if (update_timestamp) {
        self->timestamp = self->br->TimeStamp;
        self->timestamp_pos = self->pos;
    }
    SDL_mutexV(self->mutex);
    
    return(true);
}

MPEGstream_marker *
MPEGstream_new_marker (_THIS, int offset)
{
    MPEGstream_marker *marker;

    SDL_mutexP(self->mutex);
    /* We can't mark past the end of the stream */
    if (MPEGstream_eof(self)) {
        SDL_mutexV(self->mutex);
        return(0);
    }

    /* It may be possible to seek in the data stream, but punt for now */
//    if ( ((data+offset) < br->Buffer()) || ((data+offset) > stop) ) {
    if ( ((void*)(self->data+offset) < MPEGlist_Buffer(self->br)) || ((self->data+offset) > self->stop) ) {
        SDL_mutexV(self->mutex);
        return(0);
    }

    /* Set up the mark */
    marker = (MPEGstream_marker *)malloc(sizeof(MPEGstream_marker));
    marker->marked_buffer = self->br;
    marker->marked_data = self->data + offset;
    marker->marked_stop = self->stop;

    /* Lock the new buffer */
    MPEGlist_Lock(marker->marked_buffer);

    SDL_mutexV(self->mutex);

    return(marker);
}

bool
MPEGstream_seek_marker (_THIS, MPEGstream_marker const *marker)
{
    SDL_mutexP(self->mutex);

    if (marker) {
        /* Release current buffer */
        if(MPEGlist_IsLocked(self->br))
          {
            MPEGlist_Unlock(self->br);
            MPEGlist_Lock(marker->marked_buffer);
          }
        
        /* Reset the data positions */
        self->br = marker->marked_buffer;
        self->data = marker->marked_data;
        self->stop = marker->marked_stop;
    }

    SDL_mutexV(self->mutex);
    
    return(marker != 0);
}

void
MPEGstream_delete_marker (_THIS, MPEGstream_marker *marker)
{
    if (self && marker->marked_buffer) {
        MPEGlist_Unlock(marker->marked_buffer);
        free(marker);
    }
}

Uint32
MPEGstream_copy_data (_THIS, Uint8 *area, Sint32 size, bool short_read)
{
    Uint32 copied = 0;
    bool timestamped = false;

    while ((size > 0) && !MPEGstream_eof(self)) {
        Uint32 len;

        /* Get new data if necessary */
        if (self->data == self->stop) {
            /* try to use the timestamp of the first packet */
            if (!MPEGstream_next_packet(self, true, (self->timestamp == -1) || !timestamped) ) {
                break;
            }
            timestamped = true;
        }

        SDL_mutexP(self->mutex);

        /* Copy as much as we need */
        if (size <= (Sint32)(self->stop - self->data) ) {
            len = size;
        } else {
            len = (self->stop - self->data);
        }

        memcpy(area, self->data, len);

        area += len;
        self->data += len;
        size -= len;
        copied += len;
        self->pos += len;

        /* Allow 32-bit aligned short reads? */
        if (((copied % 4) == 0) && short_read ) {
            break;
        }

        SDL_mutexV(self->mutex);
    }

    return(copied);
}

int
MPEGstream_copy_byte (_THIS)
{
    /* Get new data if necessary */
    if (self->data == self->stop) {
        if (!MPEGstream_next_packet(self, true, true)) {
            return (-1);
        }
    }
    self->pos++;
    return(*self->data++);
}

bool
MPEGstream_eof (_THIS)
{
    return(!MPEGlist_Size(self->br));
}

void
MPEGstream_insert_packet (_THIS, Uint8 *Data, Uint32 Size, double timestamp)
{
    MPEGlist *newbr;

    /* Discard all packets if not enabled */
    if (!self->enabled) return;

    SDL_mutexP(self->mutex);

    self->preread_size += Size;

    /* Seek the last buffer */
    for (newbr = self->br; MPEGlist_Next(newbr); newbr = MPEGlist_Next(newbr));

    /* Position ourselves at the end of the stream */
    newbr = MPEGlist_Alloc(newbr, Size);
    if (Size) {
        memcpy(MPEGlist_Buffer(newbr), Data, Size);
    }
    newbr->TimeStamp = timestamp;

    SDL_mutexV(self->mutex);
    MPEGstream_garbage_collect(self);
}

/* - Check for unused buffers and free them - */
void
MPEGstream_garbage_collect (_THIS)
{
    MPEGlist *newbr;
    
    SDL_mutexP(self->mutex);  
    
    MPEGlist_Lock(self->br);
    
    /* First of all seek the first buffer */
    for(newbr = self->br; MPEGlist_Prev(newbr); newbr = MPEGlist_Prev(newbr));
    
    /* Now free buffers until we find a locked buffer */
    while(MPEGlist_Next(newbr) && !MPEGlist_IsLocked(newbr))
    {
        newbr = MPEGlist_Next(newbr);
        MPEGlist_delete(MPEGlist_Prev(newbr));
    }
    
    MPEGlist_Unlock(self->br);
    
    SDL_mutexV(self->mutex);
}

void
MPEGstream_enable (_THIS, bool toggle)
{
    self->enabled = toggle;
}

double
MPEGstream_time (_THIS)
{
    return (self->br->TimeStamp);
}
