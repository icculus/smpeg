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


#include "MPEGstream.h"


MPEGstream:: MPEGstream(Uint8 *Mpeg, Uint32 Size, Uint8 StreamID) :
                                                        MPEGerror()
{
    mpeg = Mpeg;
    size = Size;
    if ( StreamID == 0 ) { /* Autodetect StreamID */
        if ( mpeg[3] == 0xba ) {
            StreamID = SYSTEM_STREAMID;
        } else
        if ( mpeg[3] == 0xb3 ) {
            StreamID = VIDEO_STREAMID;
        } else
        if ( mpeg[0] == 0xff ) {
            StreamID = AUDIO_STREAMID;
        }
    }
    streamid = StreamID;
    reset_stream();
}

void
MPEGstream:: reset_stream(void)
{
    if ( mpeg[3] == 0xba ) { /* System stream, set up for next packet */
        packet = mpeg;
        packetlen = 0;
        data = stop = packet;
    } else {
        packet = mpeg;
        packetlen = size;
        data = packet;
        stop = data + packetlen;
    }
    marked_packet = NULL;
    marked_packetlen = 0;
#ifdef USE_SYSTEM_TIMESTAMP
    timestamp = 0.0;
    timedrift = 0.0;
#endif
    endofstream = false;
    errorstream = false;
}

bool
MPEGstream:: next_packet(bool recurse)
{
    const Uint8 PACKET_START_CODE[] = { 0x00, 0x00, 0x01, 0xba };
    Uint8 stream_id;

    /* Get to the next packet */
    packet += packetlen;
    if ( packet >= (mpeg+size) ) {
        endofstream = true;
        return(false);
    }

    /* Check to see what kind of packet this is */
    while ( packet < (mpeg+size) ) {
        if ( memcmp(packet, PACKET_START_CODE, 3) == 0 )
            break;
        ++packet;
    }
    if ( packet >= (mpeg+size) ) {
        errorstream = true;
        return(false);
    }

    if ( memcmp(packet, PACKET_START_CODE, 4) == 0 ) {
        packet += 4;

        /* The system stream timestamp is not very useful to us since we
           don't coordinate the audio and video threads very tightly, and
           we read in large amounts of data at once in the video stream.
           BUT... if you need it, here it is.
         */
#ifdef USE_SYSTEM_TIMESTAMP
#define FLOAT_0x10000 (double)((Uint32)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ 90000L
        { Uint8 hibit; Uint32 lowbytes;
          hibit = (packet[0]>>3)&0x01;
          lowbytes = (((Uint32)packet[0] >> 1) & 0x03) << 30;
          lowbytes |= (Uint32)packet[1] << 22;
          lowbytes |= ((Uint32)packet[2] >> 1) << 15;
          lowbytes |= (Uint32)packet[3] << 7;
          lowbytes |= ((Uint32)packet[4]) >> 1;
          timestamp = (double)hibit*FLOAT_0x10000*FLOAT_0x10000+(double)lowbytes;
          timestamp /= STD_SYSTEM_CLOCK_FREQ;
        }
#endif
        packet += 8;
    }

    /* Get the stream id, and packet length */
    stream_id = packet[3];
    packet += 4;
    packetlen = (((unsigned short)packet[0]<<8)|packet[1]);
    packet += 2;

    /* If this is a system packet, skip it */
    if ( stream_id != streamid ) {
        if ( stream_id == SYSTEM_STREAMID ) {
            packet += packetlen;
            stream_id = packet[3];
            packet += 4;
            packetlen = (((unsigned short)packet[0]<<8)|packet[1]);
            packet += 2;
        }
    }

    /* We found a packet for us? */
    if ( stream_id == streamid ) {
        if ( stream_id != SYSTEM_STREAMID ) {
            /* Skip stuffing bytes */
            while ( packet[0] == 0xff ) {
                ++packet;
                --packetlen;
            }
            if ( (packet[0] & 0x40) == 0x40 ) {
                packet += 2;
                packetlen -= 2;
            }
            if ( (packet[0] & 0x30) == 0x30 ) {
                packet += 9;
                packetlen -= 9;
            } else if ( (packet[0] & 0x20) == 0x20 ) {
                packet += 4;
                packetlen -= 4;
            } else if ( packet[0] != 0x0f ) {
                errorstream = true;
                return(false);
            }
            ++packet;
            --packetlen;
        }

        /* We have the packet data! */
        if ( packetlen > 0 ) {
            data = packet;
            stop = data + packetlen;
            return(true);
        }
    }

    /* Look for another packet of ours? */
    if ( recurse || !packetlen ) {
        return(next_packet());
    }
    return(false);
}

bool
MPEGstream:: mark_data(int offset)
{
    Uint8 *last_packet;
    Uint32 last_packetlen;

    /* We can't mark past the end of the stream */
    if ( eof() ) {
        return(false);
    }

    /* It may be possible to seek in the data stream, but punt for now */
    if ( ((data+offset) < packet) || ((data+offset) > stop) ) {
        return(false);
    }

    /* Set up the mark */
    marked_packet = packet;
    marked_packetlen = packetlen;
    marker = data+offset;
    return(true);
}

bool
MPEGstream:: seek_marker(void)
{
    if ( marked_packet ) {
        /* Reset the data positions */
        packet = marked_packet;
        packetlen = marked_packetlen;
        data = marker;
        stop = packet+packetlen;

        /* Reset error conditions for the stream */
        endofstream = false;
        errorstream = false;
    }
    return(marked_packet != NULL);
}

Uint32
MPEGstream:: copy_data(Uint8 *area, Uint32 size, bool short_read)
{
    Uint32 copied = 0;
    while ( (size > 0) && !endofstream ) {
        Uint32 len;

        /* Get new data if necessary */
        if ( data == stop ) {
            if ( ! next_packet() ) {
                break;
            }
        }

        /* Copy as much as we need */
        if ( size <= (Uint32)(stop-data) ) {
            len = size;
        } else {
            len = (stop-data);
        }
        memcpy(area, data, len);
        area += len;
        data += len;
        size -= len;
        copied += len;

        /* Allow 32-bit aligned short reads? */
        if ( ((copied%4) == 0) && short_read ) {
            break;
        }
    }
    return(copied);
}

