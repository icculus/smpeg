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

/*
 * Copyright (c) 1995 The Regents of the University of California.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF
 * CALIFORNIA HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY WARRANTIES,
 * INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS
 * ON AN "AS IS" BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
 * PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */

/*
 * Portions of this software Copyright (c) 1995 Brown University.
 * All rights reserved.
 * 
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement
 * is hereby granted, provided that the above copyright notice and the
 * following two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL BROWN UNIVERSITY BE LIABLE TO ANY PARTY FOR
 * DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
 * OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF BROWN
 * UNIVERSITY HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * BROWN UNIVERSITY SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
 * BASIS, AND BROWN UNIVERSITY HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
 */


/*
   Changes to make the code reentrant:
     Got rid of setjmp, longjmp
     deglobalized: EOF_flag, FilmState, curVidStream, bitOffset, bitLength,
     bitBuffer, sys_layer, input, seekValue, window, X Windows globals (to
     xinfo), curBits, ditherType, matched_depth, totNumFrames, realTimeStart

   Additional changes:
     Ability to play >1 movie (w/out CONTROLS)
     Make sure we do a full frame for each movie
     DISABLE_DITHER #ifdef to avoid compiling dithering code
     Changes to deal with non-MPEG streams
     Now deals with NO_DITHER, PPM_DITHER and noDisplayFlag==1
     CONTROLS version now can deal with >1 movie
   -lsh@cs.brown.edu (Loring Holden)
*/



#include <limits.h>
#include <string.h>

#include "video.h"
#include "proto.h"
#include "dither.h"
#include "util.h"

#include "MPEGvideo.h"


/*--------------------------------------------------------------*/


/* Define buffer length. */
#define BUF_LENGTH 80000


/* TODO: Eliminate these globals so multiple movies can be played. */

/* Quiet flag (verbose). */
int quietFlag = 1;

/* Framerate, -1: specified in stream (default)
               0: as fast as possible
               N (N>0): N frames/sec  
               */
int framerate = -1;

/* Flag for gamma correction */
int gammaCorrectFlag = 0;
double gammaCorrect = 1.0;

/* Flag for chroma correction */
int chromaCorrectFlag = 0;
double chromaCorrect = 1.0;

/* Flag for high quality at the expense of speed */
#ifdef QUALITY
int qualityFlag = 1;
#else
int qualityFlag = 0;
#endif

/*--------------------------------------------------------------*/

MPEGvideo::MPEGvideo(MPEGstream *stream)
{
    Uint32 start_code;

    /* Set the MPEG data stream */
    mpeg = stream;
    time_source = NULL;

    /* Set default playback variables */
    _thread = NULL;
    _x = 0;
    _y = 0;
    _scale = 1;
    _surf = NULL;
    _mutex = NULL;
    _stream = NULL;

    /* Get the width and height of the video */
    start_code = mpeg->copy_byte();
    start_code <<= 8;
    start_code |= mpeg->copy_byte();
    start_code <<= 8;
    start_code |= mpeg->copy_byte();
    while ( ! mpeg->eof() && (start_code != SEQ_START_CODE) ) {
        start_code <<= 8;
        start_code |= mpeg->copy_byte();
    }
    if ( start_code == SEQ_START_CODE ) {
        Uint8 buf[4];

        /* Get the width and height of the video */
        mpeg->copy_data(buf, 4);
        _w = (buf[0]<<4)|(buf[1]>>4);     /* 12 bits of width */
        _h = ((buf[1]&0xF)<<8)|buf[2];    /* 12 bits of height */
    } else {
        _w = 0;
        _h = 0;
        SetError("Not a valid MPEG video stream");
    }
    mpeg->reset_stream();
}

MPEGvideo:: ~MPEGvideo()
{
    /* Stop it before we free everything */
    Stop();

    /* Free actual video stream */
    if( _stream )
        DestroyVidStream( _stream );
}


void
MPEGvideo:: Loop(bool toggle)
{
    if ( _stream ) {
        _stream->loopFlag = toggle;
    }
}

/* Simple thread play function */
int Play_MPEGvideo( void *udata )
{
    MPEGvideo *mpeg = (MPEGvideo *)udata;

    /* Get the time the playback started */
    mpeg->_stream->realTimeStart += ReadSysClock();

    while( mpeg->playing )
    {
        int mark = mpeg->_stream->totNumFrames;

        /* make sure we do a whole frame */
        while( mark == mpeg->_stream->totNumFrames )
        {
            mpegVidRsrc( 0, mpeg->_stream, 0 );
        }

        if( mpeg->_stream->film_has_ended )
        {
            if( mpeg->_stream->loopFlag ) {
                /* Rewind and start playback over */
                mpeg->RewindStream();
            } else {
                mpeg->playing = false;
            }
        }
    }
    return(0);
}

void
MPEGvideo:: Play(void)
{
    ResetPause();
    if ( _stream ) {
		if ( playing ) {
			Stop();
		}
        playing = true;
#ifdef PROFILE_VIDEO	/* Profiling doesn't work well with threads */
		Play_MPEGvideo(this);
#else
        _thread = SDL_CreateThread( Play_MPEGvideo, this );
        if ( !_thread ) {
            playing = false;
        }
#endif
    }
}

void
MPEGvideo:: Stop(void)
{
    if ( _thread ) {
        playing = false;
        SDL_WaitThread(_thread, NULL);
        _thread = NULL;
    }
    ResetPause();
    if ( _stream ) {
        _stream->realTimeStart -= ReadSysClock();
    }
}

void
MPEGvideo:: RewindStream(void)
{
    /* Reinitialize vid_stream pointers */
    ResetVidStream( _stream );

    mpeg->reset_stream();

#ifdef ANALYSIS 
    init_stats();
#endif
    /* Process start codes */
    if( mpegVidRsrc( 0, _stream, 1 ) == NULL )
    {
        /* print something sensible here,
           but we only get here if the file is changed while we
           are decoding, right?
        */
    }
}

void
MPEGvideo:: Rewind(void)
{
    Stop();
    if ( _stream ) {
        RewindStream();
        _stream->realTimeStart = 0.0;
    }
    play_time = 0.0;
}

MPEGstatus
MPEGvideo:: Status(void)
{
    if ( _stream ) {
        if( !_thread || (_stream->film_has_ended && !_stream->loopFlag) ) {
            return MPEG_STOPPED;
        } else {
            return MPEG_PLAYING;
        }
    }
    return MPEG_ERROR;
}

bool
MPEGvideo:: GetVideoInfo(MPEG_VideoInfo *info)
{
    if ( info ) {
        info->width = _w;
        info->height = _h;
        if ( _stream ) {
            info->current_frame = _stream->totNumFrames;
#ifdef CALCULATE_FPS

            /* Get the appropriate indices for the timestamps */
            /* Calculate the frames-per-second from the timestamps */
            if ( _stream->frame_time[_stream->timestamp_index] ) {
                double *timestamps;
                double  time_diff;
                int this_index;
                int last_index;

                timestamps = _stream->frame_time;
                last_index = _stream->timestamp_index;
                this_index = last_index - 1;
                if ( this_index < 0 ) {
                    this_index = FPS_WINDOW-1;
                }
                time_diff = timestamps[this_index] - timestamps[last_index];
                info->current_fps = (double)FPS_WINDOW / time_diff;
            } else {
                info->current_fps = 0.0;
            }
#else
            info->current_fps = _stream->totNumFrames /
                                (ReadSysClock() - _stream->realTimeStart);
#endif
        } else {
            info->current_frame = 0;
            info->current_fps = 0.0;
        }
    }
    return(!WasError());
}

/*
   Returns zero if fails.

   surf - Surface to play movie on.
   lock - lock is held while MPEG stream is playing
   callback - called on every frame, for display update
*/
bool
MPEGvideo:: SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
                             MPEG_DisplayCallback callback)
{
    _surf = dst;
    _mutex = lock;
    if( callback ) {
        _callback = callback;
    } else {
        _callback = SDL_UpdateRect;
    }

    LUM_RANGE = 8;
    CR_RANGE = 4;
    CB_RANGE = 4;

    lum_values = _lum;
    cr_values  = _cr;
    cb_values  = _cb;

    decodeInitTables();

    InitColor();
    {
        /* Replace InitColorDisplay() */

        InitColorDither(dst->format->BitsPerPixel,
                        dst->format->Rmask,
                        dst->format->Gmask,
                        dst->format->Bmask);
    }

    InitCrop();

    _stream = NewVidStream( (unsigned int) BUF_LENGTH );
    //_stream = NewVidStream( /* 32K buffer size */ 32*1024 );
    //_stream = NewVidStream( /* 16K buffer size */ 16*1024 );
    if( _stream )
    {
        _stream->_smpeg        = this;
        _stream->ditherType    = FULL_COLOR_DITHER;
        _stream->matched_depth = dst->format->BitsPerPixel;

        if( mpegVidRsrc( 0, _stream, 1 ) == NULL )
        {
            SetError("Not an MPEG video stream");
            return false;
        }
    }
    return true;
}


/* If this is being called during play, the calling program is responsible
   for clearing the old area and coordinating with the update callback.
*/
void
MPEGvideo:: MoveDisplay( int x, int y )
{
    SDL_mutexP( _mutex );
    _x = x;
    _y = y;
    SDL_mutexV( _mutex );
}

void
MPEGvideo:: ScaleDisplay( int scale )
{
    _scale = scale;
}


void
MPEGvideo:: RenderFrame( int frame, SDL_Surface* dst, int x, int y )
{
    int saved_x, saved_y;
    SDL_Surface *saved_surf;

    /* Save original mpeg stream parameters */
    saved_x = _x;
    saved_y = _y;
    saved_surf = _surf;

    /* Set the parameters for this render */
    _x = x;
    _y = y;
    _surf = dst;

    if( _stream->totNumFrames > frame ) {
        Rewind();
    }

    _stream->_jumpFrame = frame;

    while( (_stream->totNumFrames < frame) &&
           ! _stream->film_has_ended )
    {
        mpegVidRsrc( 0, _stream, 0 );
    }

    _stream->_jumpFrame = -1;

    /* Now restore the parameters */
    _x = saved_x;
    _y = saved_y;
    _surf = saved_surf;
}

void
MPEGvideo:: RenderFinal(SDL_Surface *dst, int x, int y)
{
    int saved_x, saved_y;
    SDL_Surface *saved_surf;

    Stop();

    if( ! _stream->film_has_ended )
    {
        /* Search for the last "group of pictures" start code */
        Uint32 start_code;

        mpeg->reset_stream();
        start_code = mpeg->copy_byte();
        start_code <<= 8;
        start_code |= mpeg->copy_byte();
        start_code <<= 8;
        start_code |= mpeg->copy_byte();
        while ( ! mpeg->eof() ) {
            start_code <<= 8;
            start_code |= mpeg->copy_byte();
            if ( start_code == GOP_START_CODE ) {
                mpeg->mark_data(-4);
            }
        }

        /* Set the stream to the last spot marked */
        if ( ! mpeg->seek_marker() ) {
            mpeg->reset_stream();
        }
        _stream->buf_length = 0;
        _stream->bit_offset = 0;

        /* Process all frames without displaying any */
        _stream->_skipFrame = 1;
        RenderFrame( INT_MAX, dst, x, y );
    }

    /* Save original mpeg stream parameters */
    saved_x = _x;
    saved_y = _y;
    saved_surf = _surf;

    /* Set the parameters for this render */
    _x = x;
    _y = y;
    _surf = dst;

    /* Display the last frame processed */
    DisplayCurrentFrame( _stream );

    /* Now restore the parameters */
    _x = saved_x;
    _y = saved_y;
    _surf = saved_surf;
}


/* EOF */
