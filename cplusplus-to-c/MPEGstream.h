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

#ifndef _MPEGSTREAM_H_
#define _MPEGSTREAM_H_

#include "SDL_types.h"
#include "MPEGerror.h"
#include "MPEGvideo.h"
#include "MPEGaudio.h"
#include "MPEGlist.h"

#define AUDIO_STREAMID  0xc0
#define VIDEO_STREAMID  0xe0
#define SYSTEM_STREAMID 0xbb

struct MPEGstream_marker
{
    /* Data to mark part of the stream */
    MPEGlist *marked_buffer;
    Uint8 *marked_data;
    Uint8 *marked_stop;
};

typedef struct {
    Uint32 pos;
    Uint8 streamid;
    Uint8 *data;
    Uint8 *stop;
    Uint32 preread_size;
    MPEGsystem *system;
    MPEGlist *br;
    bool cleareof;
    bool enabled;
    SDL_mutex *mutex;
    /* "pos" where "timestamp" belongs */
    Uint32 timestamp_pos;
    double timestamp;
} MPEGstream;

MPEGstream *MPEGstream_create(MPEGsystem * System, Uint8 Streamid);
void MPEGstream_destroy(MPEGstream *self);

/* Cleanup the buffers and reset the stream */
void MPEGstream_reset_stream(MPEGstream *self);

/* Rewind the stream */
void MPEGstream_rewind_stream(MPEGstream *self);

/* Go to the next packet in the stream */
bool MPEGstream_next_packet(MPEGstream *self, bool recurse, bool update_timestamp);

/* Mark a position in the data stream */
MPEGstream_marker *MPEGstream_new_marker(MPEGstream *self, int offset);

/* Jump to the marked position */
bool MPEGstream_seek_marker(MPEGstream *self, MPEGstream_marker const *marker);

/* Jump to last successfully marked position */
void MPEGstream_delete_marker(MPEGstream_marker *marker);

/* Copy data from the stream to a local buffer */
Uint32 MPEGstream_copy_data(MPEGstream *self, Uint8 *area, Sint32 size, bool short_read);

/* Copy a byte from the stream */
int MPEGstream_copy_byte(MPEGstream *self);

/* Check for end of file or an error in the stream */
bool MPEGstream_eof(MPEGstream *self) const;

/* Insert a new packet at the end of the stream */
void MPEGstream_insert_packet(MPEGstream *self, Uint8 *data, Uint32 size, double timestamp=-1);

/* Check for unused buffers and free them */
void MPEGstream_garbage_collect(MPEGstream *self);

/* Enable or disable the stream */
void MPEGstream_enable(MPEGstream *self, bool toggle);

/* Get stream time */
double MPEGstream_time(MPEGstream *self);

/* Get a buffer from the stream */
bool MPEGstream_next_system_buffer(MPEGstream *self);

#endif /* _MPEGSTREAM_H_ */
