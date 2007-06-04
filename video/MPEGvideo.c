/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software
    
    - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn -

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
#include "MPEGfilter.h"

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

#undef _THIS
#define _THIS MPEGvideo *self

MPEGvideo *
MPEGvideo_init (_THIS, struct MPEGstream *stream)
{
    Uint32 start_code;
    MPEGstream_marker *marker;

    MAKE_OBJECT(MPEGvideo);
    self->error = MPEGerror_init(NULL);
//    self->action = MPEGaction_init(NULL);
    self->playing = false;
    self->paused = false;
    self->looping = false;
    self->play_time = 0.0;
    self->time_source = NULL;

    /* Set the MPEG data stream */
    self->mpeg = stream;
    self->time_source = NULL;

    /* Set default playback variables */
#ifdef THREADED_VIDEO
    self->_thread = NULL;
#endif /* THREADED_VIDEO */
    self->_dst = NULL;
    self->_mutex = NULL;
    self->_stream = NULL;

    /* Mark the data to leave the stream unchanged */
    /* after parsing */
    marker = MPEGstream_new_marker(self->mpeg, 0);

    /* Get the width and height of the video */
    start_code = MPEGstream_copy_byte(self->mpeg);
    start_code <<= 8;
    start_code |= MPEGstream_copy_byte(self->mpeg);
    start_code <<= 8;
    start_code |= MPEGstream_copy_byte(self->mpeg);
    while ( ! MPEGstream_eof(self->mpeg) && (start_code != SEQ_START_CODE) ) {
        start_code <<= 8;
        start_code |= MPEGstream_copy_byte(self->mpeg);
    }
    if ( start_code == SEQ_START_CODE ) {
        Uint8 buf[4];

        /* Get the width and height of the video */
        MPEGstream_copy_data(self->mpeg, buf, 4, false);
        self->_w = (buf[0]<<4)|(buf[1]>>4);    /* 12 bits of width */
        self->_h = ((buf[1]&0xF)<<8)|buf[2];   /* 12 bits of height */
	switch(buf[3]&0xF)                /*  4 bits of fps */
	{
	  case 1: self->_fps = 23.97f; break;
	  case 2: self->_fps = 24.00f; break;
	  case 3: self->_fps = 25.00f; break;
	  case 4: self->_fps = 29.97f; break;
	  case 5: self->_fps = 30.00f; break;
	  case 6: self->_fps = 50.00f; break;
	  case 7: self->_fps = 59.94f; break;
	  case 8: self->_fps = 60.00f; break;
	  case 9: self->_fps = 15.00f; break;
	  default: self->_fps = 30.00f; break;
	}
#ifndef THREADED_VIDEO
        self->frametime = (int)(1000.0 / self->_fps) - 10; /* uh... I dunno...  -PH */
#endif /* THREADED_VIDEO */
    } else {
        self->_w = 0;
        self->_h = 0;
	self->_fps = 0.00;
        MPEGerror_SetError(self->error, "Not a valid MPEG video stream");
#ifndef THREADED_VIDEO
        self->frametime = 0;
#endif /* THREADED_VIDEO */
    }
    /* Rewind back to the old position */
    MPEGstream_seek_marker(self->mpeg, marker);
    MPEGstream_delete_marker(self->mpeg, marker);

    /* Keep original width and height in _ow and _oh */
    self->_ow = self->_w;
    self->_oh = self->_h;

    /* Now round up width and height to a multiple   */
    /* of a macroblock size (16 pixels) to keep the  */
    /* video decoder happy */
    self->_w = (self->_w + 15) & ~15;
    self->_h = (self->_h + 15) & ~15;

    /* Set the default playback area */
    self->_dstrect.x = 0;
    self->_dstrect.y = 0;
    self->_dstrect.w = 0;
    self->_dstrect.h = 0;

    /* Set the source area */
    self->_srcrect.x = 0;
    self->_srcrect.y = 0;
    self->_srcrect.w = self->_ow;
    self->_srcrect.h = self->_oh;

    self->_image = 0;
    self->_filter = SMPEGfilter_null();
    self->_filter_mutex = SDL_CreateMutex();
//	printf("[MPEGvideo::MPEGvideo]_filter_mutex[%lx] = SDL_CreateMutex()\n",_filter_mutex);

    return self;
}

void
MPEGvideo_destroy (_THIS)
{
    /* Stop it before we free everything */
    MPEGvideo_Stop (self);

    /* Free actual video stream */
    if( self->_stream )
        DestroyVidStream ( self->_stream );

    /* Free overlay */
    if(self->_image) SDL_FreeYUVOverlay(self->_image);

    /* Release filter */
    SDL_DestroyMutex(self->_filter_mutex);
    self->_filter->destroy(self->_filter);

//  MPEGaction_destroy(self->action);
//  free(self->action);
//  self->action = NULL;
  self->playing = false;
  self->paused = false;
  self->looping = false;
  self->play_time = 0.0;
  self->time_source = NULL;

  MPEGerror_destroy(self->error);
  free(self->error);
  self->error = NULL;
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
        while( (mark == mpeg->_stream->totNumFrames) && mpeg->playing )
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


/* Dethreaded video.  Call regularly (such as every 10ms).  Automagic synchronise to audio. */
int
MPEGvideo_run (_THIS)
{
#ifndef THREADED_VIDEO
  int mark = self->_stream->totNumFrames;

  if (!self->playing)
      return 0;

  while (mark == self->_stream->totNumFrames) /* Get whole frame. */
    {
/* XXX: this function may hang if playing from stream and stream hangs. */
      mpegVidRsrc(0, self->_stream, 0); /* automagic sychronise w/ audio. */
    }

  if (self->_stream->film_has_ended)
    {
      self->playing = false;
    }

#endif /* THREADED_VIDEO */
  return(0);
}

void
MPEGvideo_Play (_THIS)
{
  self->_stream->realTimeStart += ReadSysClock(); /* dethreaded. */
  self->start_frames = self->_stream->totNumFrames;
  self->start_time = SDL_GetTicks();

    MPEGvideo_ResetPause (self);
    if ( self->_stream ) {
		if ( self->playing ) {
			MPEGvideo_Stop (self);
		}
        self->playing = true;
#ifdef PROFILE_VIDEO	/* Profiling doesn't work well with threads */
		Play_MPEGvideo(self);
#else
# ifndef THREADED_VIDEO
        self->playing = true;
# else /* THREADED_VIDEO */
        self->_thread = SDL_CreateThread( Play_MPEGvideo, self );
        if ( !self->_thread ) {
            self->playing = false;
        }
# endif /* THREADED_VIDEO */
#endif
    }
}

void
MPEGvideo_Stop (_THIS)
{
#ifndef THREADED_VIDEO
/* Dethreaded. */
# ifdef TIME_MPEG
  int total_frames;
  double total_time;

  self->stop_time = SDL_GetTicks();
  self->stop_frames = self->_stream->totNumFrames;
  total_frames = (self->stop_frames - self->start_frames);
  total_time = (float)(self->stop_time - self->start_time)/1000.0;
  if ( total_time > 0 ) {
      printf("%d frames in %2.2f seconds (%2.2f FPS)\n",
             total_frames, total_time, (float)total_frames/total_time);
  }
# endif /* TIME_MPEG */
  self->playing = false;
#else /* THREADED_VIDEO */
    if ( self->_thread ) {
        self->playing = false;
        SDL_WaitThread(self->_thread, NULL);
        self->_thread = NULL;
    }
#endif /* THREADED_VIDEO */
    MPEGvideo_ResetPause (self);
}

void
MPEGvideo_Rewind (_THIS)
{
    MPEGvideo_Stop (self);
    if ( self->_stream ) {
      /* Reinitialize vid_stream pointers */
      ResetVidStream( self->_stream );
#ifdef ANALYSIS 
      init_stats();
#endif
    }
}

void
MPEGvideo_ResetSynchro (_THIS, double time)
{
  if( self->_stream )
  {
    self->_stream->_jumpFrame = -1;
    self->_stream->realTimeStart = -time;
    self->play_time = time;
    if (time > 0) {
	double oneframetime;
	if (self->_stream->_oneFrameTime == 0)
		oneframetime = 1.0 / self->_stream->_smpeg->_fps;	
	else
		oneframetime = self->_stream->_oneFrameTime;

	/* time -> frame */
	self->_stream->totNumFrames = (int)(time / oneframetime);

	/* Set Current Frame To 0 & Frame Adjust Frag Set */
	self->_stream->current_frame = 0;
	self->_stream->need_frameadjust=true;
    }
  }
}


void
MPEGvideo_Skip (_THIS, float seconds)
{
  int frame;

  /* Called only when there is no timestamp info in the MPEG */
  /* This is quite slow however */
  printf("Video: Skipping %f seconds...\n", seconds);  
  frame = (int) (self->_fps * seconds);

  if( self->_stream )
  {
    self->_stream->_jumpFrame = frame;
    while( (self->_stream->totNumFrames < frame) &&
	   ! self->_stream->film_has_ended )
    {
      mpegVidRsrc( 0, self->_stream, 0 );
    }
    MPEGvideo_ResetSynchro (self, 0);
  }
}

MPEGstatus
MPEGvideo_GetStatus (_THIS)
{
    if ( self->_stream ) {
#ifndef THREADED_VIDEO
        return (self->playing ? MPEG_PLAYING : MPEG_STOPPED);
#else /* THREADED_VIDEO */
        if( !self->_thread || (self->_stream->film_has_ended ) ) {
            return MPEG_STOPPED;
        } else {
            return MPEG_PLAYING;
        }
#endif /* THREADED_VIDEO */
    }
    return MPEG_ERROR;
}

bool
MPEGvideo_GetVideoInfo (_THIS, MPEG_VideoInfo *info)
{
    if ( info ) {
        info->width = self->_ow;
        info->height = self->_oh;
        if ( self->_stream ) {
            info->current_frame = self->_stream->current_frame;
#ifdef CALCULATE_FPS

            /* Get the appropriate indices for the timestamps */
            /* Calculate the frames-per-second from the timestamps */
            if ( self->_stream->frame_time[self->_stream->timestamp_index] ) {
                double *timestamps;
                double  time_diff;
                int this_index;
                int last_index;

                timestamps = self->_stream->frame_time;
                last_index = self->_stream->timestamp_index;
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
            info->current_fps = self->_stream->totNumFrames /
                                (ReadSysClock() - self->_stream->realTimeStart);
#endif
        } else {
            info->current_frame = 0;
            info->current_fps = 0.0;
        }
    }
    return (!MPEGerror_WasError(self->error));
}

/*
   Returns zero if fails.

   surf - Surface to play movie on.
   lock - lock is held while MPEG stream is playing
   callback - called on every frame, for display update
*/
bool
MPEGvideo_SetDisplay (_THIS, SDL_Surface *dst, SDL_mutex *lock, MPEG_DisplayCallback callback)
{
    self->_mutex = lock;
    self->_dst = dst;
    self->_callback = callback;
    if ( self->_image ) {
      SDL_FreeYUVOverlay(self->_image);
    }
    self->_image = SDL_CreateYUVOverlay(self->_srcrect.w, self->_srcrect.h, SDL_YV12_OVERLAY, dst);
    if ( !self->_dstrect.w || !self->_dstrect.h ) {
        self->_dstrect.w = dst->w;
        self->_dstrect.h = dst->h;
    }

    if ( !self->_stream ) {
        decodeInitTables();

        InitCrop();
        InitIDCT();

        self->_stream = NewVidStream( (unsigned int) BUF_LENGTH );
        if( self->_stream ) {
            self->_stream->_smpeg        = self;
            self->_stream->ditherType    = FULL_COLOR_DITHER;
            self->_stream->matched_depth = dst->format->BitsPerPixel;

            if( mpegVidRsrc( 0, self->_stream, 1 ) == NULL ) {
                MPEGerror_SetError(self->error, "Not an MPEG video stream");
                return false;
            }
        }

        if ( ! InitPictImages(self->_stream, self->_w, self->_h, self->_dst) )
            return false;
    }
    return true;
}


/* If this is being called during play, the calling program is responsible
   for clearing the old area and coordinating with the update callback.
*/
void
MPEGvideo_MoveDisplay (_THIS, int x, int y)
{
    SDL_mutexP( self->_mutex );
    self->_dstrect.x = x;
    self->_dstrect.y = y;
    SDL_mutexV( self->_mutex );
}

void
MPEGvideo_ScaleDisplayXY (_THIS, int w, int h)
{
    SDL_mutexP( self->_mutex );
    self->_dstrect.w = w;
    self->_dstrect.h = h;
    SDL_mutexV( self->_mutex );
}

void
MPEGvideo_SetDisplayRegion (_THIS, int x, int y, int w, int h)
{
    SDL_mutexP( self->_mutex );
    self->_srcrect.x = x;
    self->_srcrect.y = y;
    self->_srcrect.w = w;
    self->_srcrect.h = h;

    if(self->_image)
    {
      SDL_FreeYUVOverlay(self->_image);
      self->_image = SDL_CreateYUVOverlay(self->_srcrect.w, self->_srcrect.h, SDL_YV12_OVERLAY, self->_dst);
    }

    SDL_mutexV( self->_mutex );
}

/* API CHANGE: This function no longer takes a destination surface and x/y
   You must use SetDisplay() and MoveDisplay() to set those attributes.
*/
void
MPEGvideo_RenderFrame (_THIS, int frame)
{
    self->_stream->need_frameadjust = true;

    if( self->_stream->current_frame > frame ) {
        MPEGstream_rewind_stream(self->mpeg);
        MPEGstream_next_packet(self->mpeg, true, true);
        MPEGvideo_Rewind (self);
    }

    self->_stream->_jumpFrame = frame;

    while( (self->_stream->current_frame < frame) &&
           ! self->_stream->film_has_ended )
    {
        mpegVidRsrc( 0, self->_stream, 0 );
    }

    self->_stream->_jumpFrame = -1;
}

void
MPEGvideo_RenderFinal (_THIS, SDL_Surface *dst, int x, int y)
{
    SDL_Surface *saved_dst;
    int saved_x, saved_y;

    /* This operation can only be performed when stopped */
    MPEGvideo_Stop (self);

    /* Set (and save) the destination and location */
    saved_dst = self->_dst;
    saved_x = self->_dstrect.x;
    saved_y = self->_dstrect.y;
    MPEGvideo_SetDisplay (self, dst, self->_mutex, self->_callback);
    MPEGvideo_MoveDisplay (self, x, y);

    if ( ! self->_stream->film_has_ended ) {
        /* Search for the last "group of pictures" start code */
        Uint32 start_code;
        MPEGstream_marker * marker, * oldmarker;

        marker = 0;
        start_code = MPEGstream_copy_byte(self->mpeg);
        start_code <<= 8;
        start_code |= MPEGstream_copy_byte(self->mpeg);
        start_code <<= 8;
        start_code |= MPEGstream_copy_byte(self->mpeg);

        while ( ! MPEGstream_eof(self->mpeg) ) {
            start_code <<= 8;
            start_code |= MPEGstream_copy_byte(self->mpeg);
            if ( start_code == GOP_START_CODE ) {
	          oldmarker = marker;
        	  marker = MPEGstream_new_marker(self->mpeg, -4);
        	  if( oldmarker ) MPEGstream_delete_marker(self->mpeg, oldmarker);
                  MPEGstream_garbage_collect(self->mpeg);
            }
        }

        /* Set the stream to the last spot marked */
        if ( ! MPEGstream_seek_marker(self->mpeg, marker) ) {
            MPEGstream_rewind_stream(self->mpeg);
            MPEGstream_next_packet(self->mpeg, true, true);
        }

        MPEGstream_delete_marker(self->mpeg, marker);
        self->_stream->buf_length = 0;
        self->_stream->bit_offset = 0;

        /* Process all frames without displaying any */
        self->_stream->_skipFrame = 1;

        MPEGvideo_RenderFrame (self, INT_MAX );

        MPEGstream_garbage_collect(self->mpeg);
    }

    /* Display the frame */
    MPEGvideo_DisplayFrame (self, self->_stream);

    /* Restore the destination and location */
    MPEGvideo_SetDisplay (self, saved_dst, self->_mutex, self->_callback);
    MPEGvideo_MoveDisplay (self, saved_x, saved_y);
}



/* virtual methods of MPEGaction. */

void MPEGvideo_SetTimeSource (_THIS, struct MPEGaudio *source)
{
  self->time_source = source;
}

void MPEGvideo_Loop (_THIS, bool toggle)
{
  self->looping = toggle;
}

double MPEGvideo_Time (_THIS)
{
  return self->play_time;
}

void MPEGvideo_ResetPause (_THIS)
{
  self->paused = false;
}

/*virtual*/ void MPEGvideo_Pause (_THIS) /* A toggle action */
{
    if ( self->paused ) {
        self->paused = false;
        MPEGvideo_Play (self);
    } else {
        MPEGvideo_Stop (self);
        self->paused = true;
    }
}



/* other odds and ends. */

struct MPEGaudio *
MPEGvideo_TimeSource (_THIS)
{
  return self->time_source;
}



/* EOF */
