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
#ifdef TIME_MPEG
int framerate = 0;
#else
int framerate = -1;
#endif

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
    MPEGstream_marker *marker;

    /* Set the MPEG data stream */
    mpeg = stream;
    time_source = NULL;

    /* Set default playback variables */
    _thread = NULL;
    _dst = NULL;
    _mutex = NULL;
    _stream = NULL;

    /* Mark the data to leave the stream unchanged */
    /* after parsing */
    marker = mpeg->new_marker(0);

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
        _w = (((buf[0]<<4)|(buf[1]>>4)) + 15) & ~15;    /* 12 bits of width */
        _h = ((((buf[1]&0xF)<<8)|buf[2]) + 15) & ~15;   /* 12 bits of height */
	switch(buf[3]&0xF)                /*  4 bits of fps */
	{
	  case 1: _fps = 23.97; break;
	  case 2: _fps = 24.00; break;
	  case 3: _fps = 25.00; break;
	  case 4: _fps = 29.97; break;
	  case 5: _fps = 30.00; break;
	  case 6: _fps = 50.00; break;
	  case 7: _fps = 59.94; break;
	  case 8: _fps = 60.00; break;
	  case 9: _fps = 15.00; break;
	  default: _fps = 30.00; break;
	}
    } else {
        _w = 0;
        _h = 0;
	_fps = 0.00;
        SetError("Not a valid MPEG video stream");
    }
    /* Rewind back to the old position */
    mpeg->seek_marker(marker);
    mpeg->delete_marker(marker);

    /* Set the default playback area */
    _rect.x = 0;
    _rect.y = 0;
    _rect.w = _w;
    _rect.h = _h;
}

MPEGvideo:: ~MPEGvideo()
{
    /* Stop it before we free everything */
    Stop();

    /* Free actual video stream */
    if( _stream )
        DestroyVidStream( _stream );
}

/* Simple thread play function */
int Play_MPEGvideo( void *udata )
{
    MPEGvideo *mpeg = (MPEGvideo *)udata;

    /* Get the time the playback started */
    mpeg->_stream->realTimeStart += ReadSysClock();

#ifdef TIME_MPEG
    int start_frames, stop_frames;
    int total_frames;
    Uint32 start_time, stop_time;
    float total_time;

    start_frames = mpeg->_stream->totNumFrames;
    start_time = SDL_GetTicks();
#endif
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
	  mpeg->playing = false;
        }
    }
    /* Get the time the playback stopped */
    mpeg->_stream->realTimeStart -= ReadSysClock();
#ifdef TIME_MPEG
    stop_time = SDL_GetTicks();
    stop_frames = mpeg->_stream->totNumFrames;
    total_frames = (stop_frames-start_frames);
    total_time = (float)(stop_time-start_time)/1000.0;
    if ( total_time > 0 ) {
        printf("%d frames in %2.2f seconds (%2.2f FPS)\n",
               total_frames, total_time, (float)total_frames/total_time);
    }
#endif
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
}

void
MPEGvideo:: Rewind(void)
{
    Stop();
    if ( _stream ) {
      /* Reinitialize vid_stream pointers */
      ResetVidStream( _stream );
#ifdef ANALYSIS 
      init_stats();
#endif
      /* Process start codes */
      /* Vivien: seems unnecessary */
#if 0
      if( mpegVidRsrc( 0, _stream, 1 ) == NULL )
      {
	SetError("Not an MPEG video stream");
	return;
      }
#endif
    }
}

void
MPEGvideo:: ResetSynchro(double time)
{
  _stream->_jumpFrame = -1;
  _stream->realTimeStart = -time;
  play_time = time;
  if (time > 0) {
	double oneframetime;
	if (_stream->_oneFrameTime == 0)
		oneframetime = 1.0 / _stream->_smpeg->_fps;	
	else
		oneframetime = _stream->_oneFrameTime;

	/* time -> frame */
	_stream->totNumFrames = (int)(time / oneframetime);

	/* Set Current Frame To 0 & Frame Adjust Frag Set */
	_stream->current_frame = 0;
	_stream->need_frameadjust=true;
  }
}


void
MPEGvideo::Skip(float seconds)
{
  int frame;

  /* Called only when there is no timestamp info in the MPEG */
  /* This is quite slow however */
  printf("Video: Skipping %f seconds...\n", seconds);  
  frame = (int) (_fps * seconds);

  if( _stream )
  {
    _stream->_jumpFrame = frame;
    while( (_stream->totNumFrames < frame) &&
	   ! _stream->film_has_ended )
    {
      mpegVidRsrc( 0, _stream, 0 );
    }
    ResetSynchro(0);
  }
}

MPEGstatus
MPEGvideo:: Status(void)
{
    if ( _stream ) {
        if( !_thread || (_stream->film_has_ended ) ) {
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
            info->current_frame = _stream->current_frame;
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
    _mutex = lock;
    _dst = dst;
    _callback = callback;

    if ( !_stream ) {
        decodeInitTables();

        InitCrop();
        InitIDCT();

        _stream = NewVidStream( (unsigned int) BUF_LENGTH );
        if( _stream ) {
            _stream->_smpeg        = this;
            _stream->ditherType    = FULL_COLOR_DITHER;
            _stream->matched_depth = dst->format->BitsPerPixel;

            if( mpegVidRsrc( 0, _stream, 1 ) == NULL ) {
                SetError("Not an MPEG video stream");
                return false;
            }
        }
    }
    if ( ! InitPictImages(_stream, _w, _h, _dst) )
        return false;
    return true;
}


/* If this is being called during play, the calling program is responsible
   for clearing the old area and coordinating with the update callback.
*/
void
MPEGvideo:: MoveDisplay( int x, int y )
{
    SDL_mutexP( _mutex );
    _rect.x = x;
    _rect.y = y;
    SDL_mutexV( _mutex );
}

void
MPEGvideo:: ScaleDisplayXY( int w, int h )
{
    SDL_mutexP( _mutex );
    _rect.w = w;
    _rect.h = h;
    SDL_mutexV( _mutex );
}


/* API CHANGE: This function no longer takes a destination surface and x/y
   You must use SetDisplay() and MoveDisplay() to set those attributes.
*/
void
MPEGvideo:: RenderFrame( int frame )
{

   if (_stream->need_frameadjust) {
      _stream->_jumpFrame=0;
      while(_stream->need_frameadjust) {
        mpegVidRsrc(0, _stream, 0);
#ifdef DEBUG
        printf("Adjusting Frame %d Timecode %f\n",_stream->totNumFrames,_stream->timestamp);
#endif
      }
      _stream->_jumpFrame=-1;
      return;
   }

    if( _stream->totNumFrames > frame ) {
        mpeg->rewind_stream();
	mpeg->next_packet();
        Rewind();
    }

    _stream->_jumpFrame = frame;

    while( (_stream->totNumFrames < frame) &&
           ! _stream->film_has_ended )
    {
        mpegVidRsrc( 0, _stream, 0 );
    }

    _stream->_jumpFrame = -1;
}

void
MPEGvideo:: RenderFinal(SDL_Surface *dst, int x, int y)
{
    SDL_Overlay *yuv;

    Stop();

    if( ! _stream->film_has_ended )
    {
        /* Search for the last "group of pictures" start code */
        Uint32 start_code;
	MPEGstream_marker * marker, * oldmarker;

	marker = 0;
        mpeg->rewind_stream();
	mpeg->next_packet();
        start_code = mpeg->copy_byte();
        start_code <<= 8;
        start_code |= mpeg->copy_byte();
        start_code <<= 8;
        start_code |= mpeg->copy_byte();
        while ( ! mpeg->eof() ) {
            start_code <<= 8;
            start_code |= mpeg->copy_byte();
            if ( start_code == GOP_START_CODE ) {
	          oldmarker = marker;
		  marker = mpeg->new_marker(-4);
		  if( oldmarker ) mpeg->delete_marker( oldmarker );
       		  mpeg->garbage_collect();
            }
        }
        /* Set the stream to the last spot marked */
        if ( ! mpeg->seek_marker( marker ) ) {
            mpeg->rewind_stream();
	    mpeg->next_packet();
	}

	mpeg->delete_marker( marker );
        _stream->buf_length = 0;
        _stream->bit_offset = 0;

        /* Process all frames without displaying any */
        _stream->_skipFrame = 1;
        RenderFrame( INT_MAX );
	mpeg->garbage_collect();
    }

    /* Create a temporary YUV surface and display the frame */
    yuv = SDL_CreateYUVOverlay(_w, _h, SDL_YV12_OVERLAY, dst);
    if ( yuv ) {
        if ( SDL_LockYUVOverlay(yuv) == 0 ) {
	  SDL_Rect dstrect;

            memcpy(yuv->pixels, _stream->current->image->pixels, (_w*_h)+2*(_w*_h)/4);
            SDL_UnlockYUVOverlay(yuv);
            dstrect.x = _rect.x;
            dstrect.y = _rect.y;
            dstrect.w = _rect.w;
            dstrect.h = _rect.h;
            SDL_DisplayYUVOverlay(yuv, &dstrect);
        }
        SDL_FreeYUVOverlay(yuv);
    }
}


/* EOF */