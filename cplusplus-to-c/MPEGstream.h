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

/* MPEG stream can be:
 - a video stream (2D over time)
 - an audio stream (1D over time)
 - a system stream (combination video and audio)
*/
/* The idea of smpeg is to split the system stream (if any) into separate audio
and video stream, then feed the separated stream into their corresponding
decoders. */

#define AUDIO_STREAMID  0xc0
#define VIDEO_STREAMID  0xe0
#define SYSTEM_STREAMID 0xbb

struct MPEGstream_marker
{
    /* Data to mark part of the stream */
    MPEGlist * marked_buffer;
    Uint8 *marked_data;
    Uint8 *marked_stop;  
};

typedef struct MPEGstream_marker MPEGstream_marker;


struct MPEGstream
{
    Uint8 *data;
    Uint8 *stop;

    Uint32 preread_size;

    struct MPEGsystem * system; /* points to the system stream (for audio/video streams). */
    MPEGlist * br;
    bool cleareof;
    bool enabled;

    SDL_mutex * mutex;

//    /* Get a buffer from the stream */
//    bool next_system_buffer(void);

    /* "pos" where "timestamp" belongs */
    Uint32 timestamp_pos;
    double timestamp;

    Uint32 pos;

    Uint8 streamid;

};

typedef struct MPEGstream MPEGstream;


#undef _THIS
#define _THIS MPEGstream *self
#undef METH
#define METH(m) MPEGstream_##m

MPEGstream * METH(init) (_THIS, struct MPEGsystem *System, Uint8 Streamid);
void METH(destroy) (_THIS);

bool METH(next_system_buffer) (_THIS);

/* Cleanup the buffers and reset the stream */
void METH(reset_stream) (_THIS);

/* Rewind the stream */
void METH(rewind_stream) (_THIS);

/* Go to the next packet in the stream */
//bool METH(next_packet) (_THIS, bool recurse = true, bool update_timestamp = true);
bool METH(next_packet) (_THIS, bool recurse, bool update_timestamp);

/* Mark a position in the data stream */
MPEGstream_marker *METH(new_marker) (_THIS, int offset);

/* Jump to the marked position */
bool METH(seek_marker) (_THIS, MPEGstream_marker const * marker);

/* Jump to last successfully marked position */
void METH(delete_marker) (_THIS, MPEGstream_marker * marker);

/* Copy data from the stream to a local buffer */
//Uint32 METH(copy_data) (_THIS, Uint8 *area, Sint32 size, bool short_read = false);
Uint32 METH(copy_data) (_THIS, Uint8 *area, Sint32 size, bool short_read);

/* Copy a byte from the stream */
int METH(copy_byte) (_THIS);

/* Check for end of file or an error in the stream */
bool METH(eof) (_THIS); // const;

/* Insert a new packet at the end of the stream */
//void METH(insert_packet) (_THIS, Uint8 * data, Uint32 size, double timestamp=-1);
void METH(insert_packet) (_THIS, Uint8 * data, Uint32 size, double timestamp);

/* Check for unused buffers and free them */
void METH(garbage_collect) (_THIS);

/* Enable or disable the stream */
void METH(enable) (_THIS, bool toggle);

/* Get stream time */
double METH(time) (_THIS);

#endif /* _MPEGSTREAM_H_ */


