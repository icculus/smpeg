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

#define AUDIO_STREAMID  0xc0
#define VIDEO_STREAMID  0xe0
#define SYSTEM_STREAMID 0xbb
#define PAD_STREAMID    0xbe

class MPEGstream : public MPEGerror {
public:
    MPEGstream(Uint8 *Mpeg, Uint32 Size, Uint8 StreamID);

    /* Rewind the stream to the beginning and reset eof/error conditions */
    void reset_stream(void);

    /* Go to the next packet in the stream */
    bool next_packet(bool recurse = true);

    /* Mark a position in the data stream */
    bool mark_data(int offset);

    /* Jump to last successfully marked position */
    bool seek_marker(void);

    /* Copy data from the stream to a local buffer */
    Uint32 copy_data(Uint8 *area, Uint32 size, bool short_read = false);

    /* Copy a byte from the stream */
    int copy_byte(void) {
        /* Get new data if necessary */
        if ( data == stop ) {
            if ( ! next_packet() ) {
                return (-1);
            }
        }
        return(*data++);
    }

    /* Check for end of file or an error in the stream */
    bool eof(void) {
        return(endofstream || errorstream);
    }

protected:
    /* The actual memory mapped MPEG data */
    Uint8 *mpeg;
    Uint32 size;

    /* Data specific to this particular stream */
    Uint8 streamid;
    Uint8 *packet;
    Uint32 packetlen;
    bool endofstream;
    bool errorstream;

    /* Data to mark part of the stream */
    Uint8 *marked_packet;
    Uint32 marked_packetlen;
    Uint8 *marker;

#ifdef USE_SYSTEM_TIMESTAMP
    /* Current timestamp for this stream */
    double timestamp;
    double timedrift;
#endif

/* Caution: these may go away (e.g. stream is based on stdio) */
public:
    Uint8 *data;
    Uint8 *stop;
};

#endif /* _MPEGSTREAM_H_ */
