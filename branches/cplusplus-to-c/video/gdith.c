/* 
 * gdith.c --
 *
 *      Procedures dealing with grey-scale and mono dithering, 
 *      as well as X Windows set up procedures.
 *
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

#include <math.h>
#include "video.h"
#include "proto.h"
#include "dither.h"
#include "SDL_timer.h"

#ifdef USE_ATI
#include "vhar128.h"
#endif

#ifdef __STDC__
#include <stdlib.h>
#include <string.h>
#endif


/* 
   Changes to make the code reentrant:
      X variables now passed in xinfo: display, ximage,cmap,window, gc, etc
      De-globalized: ditherType, matched_depth, totNumFrames
      vid_stream->film_has_ended instead of FilmState

   Additional changes:
      Now can name and position each movie window individually
      DISABLE_DITHER cpp define - do not include dither code if defined
      NOFRAMECOUNT cpp define - do not count frames when running without
         controls
      Short circuit InitColorDisplay if not displaying anything
      ExistingWindow default now 0
   -lsh@cs.brown.edu (Loring Holden)
*/

/* Frame Rate Info */
extern int framerate;

/* Video rates table */
/* Cheat on Vid rates, round to 30, and use 30 if illegal value 
   Except for 9, where Xing means 15, and given their popularity, we'll
   be nice and do it */
static double VidRateNum[16]={ 30, 23.97, 24, 25, 29.97, 30, 50, 59.94, 
                            60, 15, 30, 30, 30, 15, 30, 30 };

#ifdef CALCULATE_FPS
static inline void TimestampFPS( VidStream* vid_stream )
{
    MPEGvideo* mpeg = (MPEGvideo*) vid_stream->_smpeg;

    vid_stream->frame_time[vid_stream->timestamp_index] = MPEGvideo_Time(mpeg);
    ++vid_stream->timestamp_index;
    if ( vid_stream->timestamp_index == FPS_WINDOW ) {
        vid_stream->timestamp_index = 0;
    }
}
#endif

/*
   Do frame rate control.  Returns _skipFrame
*/
#define LOOSE_MPEG_SCHEDULING
#ifdef LOOSE_MPEG_SCHEDULING
#define MAX_FRAME_SKIP  4
#define MAX_FUDGE_TIME  (MAX_FRAME_SKIP*vid_stream->_oneFrameTime)
#else
#ifdef TIGHT_MPEG_SCHEDULING
#define MAX_FRAME_SKIP  1
#define MAX_FUDGE_TIME  (MAX_FRAME_SKIP*vid_stream->_oneFrameTime)
#else
#define MAX_FRAME_SKIP  3
#define MAX_FUDGE_TIME  (MAX_FRAME_SKIP*vid_stream->_oneFrameTime)
#endif /* TIGHT_MPEG_SCHEDULING */
#endif /* LOOSE_MPEG_SCHEDULING */
#define FUDGE_TIME	(((MAX_FRAME_SKIP+1)/2)*vid_stream->_oneFrameTime)

/* This results in smoother framerate, but more dropped frames on
   systems that can play most of the video fine, but have problems
   with jerkiness in a few spots.
*/
//#define SLOW_START_SCHEDULING
#define SLOW_START_INCREMENT    0.3

/* Define this to debug the frame scheduler */
//#define DEBUG_MPEG_SCHEDULING

inline double CurrentTime( VidStream* vid_stream )
{
    MPEGvideo* mpeg = (MPEGvideo*) vid_stream->_smpeg;
    MPEGaudio* audio;
    double now;

//    if ( mpeg->TimeSource() ) {
    if ((audio = MPEGvideo_TimeSource(mpeg))) {
//        now = mpeg->TimeSource()->Time();
      now = MPEGaudio_Time(audio);
    } else {
        now = ReadSysClock() - vid_stream->realTimeStart;
    }
    return now;
}


#undef _THIS
#define _THIS MPEGvideo *self

int
MPEGvideo_timeSync ( _THIS, VidStream* vid_stream )
{
    static double correction = -1;

    /* Update the number of frames displayed */
    vid_stream->totNumFrames++;
    vid_stream->current_frame++;

    /* Do we need to initialize framerate? */
    if ( vid_stream->rate_deal < 0 ) {
        switch( framerate ) {
          case -1: /* Go with stream Value */
            vid_stream->rate_deal = VidRateNum[ vid_stream->picture_rate ];
            break;

          case 0: /* as fast as possible */
            vid_stream->rate_deal = 0;
            break;

          default:
            vid_stream->rate_deal = framerate;
            break;
        }
        if ( vid_stream->rate_deal ) {
            vid_stream->_oneFrameTime = 1.0 / vid_stream->rate_deal;
        }
    }

    /* Update the current play time */
    self->play_time += vid_stream->_oneFrameTime;

    /* Synchronize using system timestamps */
    if(vid_stream->current && vid_stream->current->show_time > 0){
#ifdef DEBUG_TIMESTAMP_SYNC
      fprintf(stderr, "video: time:%.3f  shift:%.3f\r",
	      play_time,
	      play_time - vid_stream->current->show_time);
#endif
      if(correction == -1)
#ifdef STRANGE_SYNC_TEST
       /* this forces us to maintain the offset we have at the begining
          all the time, and is only usefull for testing */
        correction = play_time - vid_stream->current->show_time;
#else
       correction = 0;
#endif
#ifdef USE_TIMESTAMP_SYNC
      play_time = vid_stream->current->show_time + correction ;
#endif
      vid_stream->current->show_time = -1;
    }

    /* If we are looking for a particular frame... */
    if( vid_stream->_jumpFrame > -1 )
    {
        if ( vid_stream->totNumFrames != vid_stream->_jumpFrame ) {
            vid_stream->_skipFrame = 1;
        } else {
            vid_stream->_skipFrame = 0;
        }
        return vid_stream->_skipFrame;
    }

    /* If we're already behind, don't check timing */
    if ( vid_stream->_skipFrame > 0 )
    {
        return --vid_stream->_skipFrame;
    }

    /* See if we need to skip frames, based on timing */
    if ( vid_stream->rate_deal ) {
        static const double TIMESLICE = 0.01;   // Seconds per OS timeslice
        double time_behind;

        /* Calculate the frame time relative to real time */
        time_behind = CurrentTime(vid_stream) - MPEGvideo_Time(self);

#ifdef DEBUG_MPEG_SCHEDULING
printf("Frame %d: frame time: %f, real time: %f, time behind: %f\n", vid_stream->totNumFrames, Time(), CurrentTime(vid_stream), time_behind);
#endif

        /* Allow up to MAX_FUDGE_TIME of delay in output */
        if ( time_behind < -TIMESLICE ) {
            time_behind = -time_behind;
            vid_stream->_skipCount = 0;
#ifdef DEBUG_MPEG_SCHEDULING
printf("Ahead!  Sleeping %f\n", time_behind-TIMESLICE);
#endif
            SDL_Delay((Uint32)((time_behind-TIMESLICE)*1000));
        } else
        if ( time_behind < FUDGE_TIME ) {
            if ( vid_stream->_skipCount > 0 ) {
                vid_stream->_skipCount /= 2;
            }
#ifdef DEBUG_MPEG_SCHEDULING
printf("Just right.\n");
#endif
        } else
        if ( time_behind < MAX_FUDGE_TIME ) {
            if ( vid_stream->_skipCount > 0 ) {
                vid_stream->_skipCount--;
            }
            vid_stream->_skipFrame = 1+(int)(vid_stream->_skipCount/2);
#ifdef DEBUG_MPEG_SCHEDULING
printf("A little behind, skipping %d frames\n", vid_stream->_skipFrame);
#endif
        } else {
            /* time_behind >= MAX_FUDGE_TIME */
            if ( (time_behind > (MAX_FUDGE_TIME*2)) &&
                 (vid_stream->_skipCount == MAX_FRAME_SKIP) ) {
#ifdef DEBUG_MPEG_SCHEDULING
printf("Way too far behind, losing time sync...\n");
#endif
#if 0 // This results in smoother video, but sync's terribly on slow machines
                play_time = CurrentTime(vid_stream) - (MAX_FUDGE_TIME*2);
#endif
            }
#ifdef SLOW_START_SCHEDULING
            vid_stream->_skipCount += SLOW_START_INCREMENT;
#else
            vid_stream->_skipCount += 1.0;
#endif
            if( vid_stream->_skipCount > MAX_FRAME_SKIP ) {
                vid_stream->_skipCount = MAX_FRAME_SKIP;
            }
            vid_stream->_skipFrame = (int)(vid_stream->_skipCount+0.9);
#ifdef DEBUG_MPEG_SCHEDULING
printf("A lot behind, skipping %d frames\n", vid_stream->_skipFrame);
#endif
        }
    }
    return(vid_stream->_skipFrame);
}

/* Do the hard work of copying from the video stream working buffer to the
   screen display and then calling the update callback.
*/
void
MPEGvideo_DisplayFrame ( _THIS, VidStream * vid_stream )
{
  SMPEG_FilterInfo info;

  if ( self->_filter_mutex )
    SDL_mutexP( self->_filter_mutex );

  /* Get a pointer to _image pixels */
  if ( SDL_LockYUVOverlay( self->_image ) ) {
    return;
  }

  /* Compute additionnal info for the filter */
  if((self->_filter->flags & SMPEG_FILTER_INFO_PIXEL_ERROR) && vid_stream->current->mb_qscale)
  {
    register int x, y;
    register Uint16 * ptr;

    /* Compute quantization error for each pixel */
    info.yuv_pixel_square_error = (Uint16 *) malloc(self->_w*self->_h*12/8*sizeof(Uint16));

    ptr =  info.yuv_pixel_square_error;
    for(y = 0; y < self->_h; y++)
      for(x = 0; x < self->_w; x++)
	*ptr++ = (Uint16) (((Uint32) vid_stream->noise_base_matrix[x & 7][y & 7] * 
			    vid_stream->current->mb_qscale[((y>>4) * (self->_w>>4)) + (x >> 4)]) >> 8);
  }
  
  if((self->_filter->flags & SMPEG_FILTER_INFO_MB_ERROR) && vid_stream->current->mb_qscale)
  {
    /* Retreive macroblock quantization error info */
    info.yuv_mb_square_error = vid_stream->current->mb_qscale;
  }
    
  if( self->_filter )
  {
    SDL_Overlay src;
    Uint16 pitches[3];
    Uint8 *pixels[3];

    /* Fill in an SDL YV12 overlay structure for the source */
#ifdef USE_ATI
    vhar128_lockimage(vid_stream->ati_handle, vid_stream->current->image, &src);
#else
    src.format = SDL_YV12_OVERLAY;
    src.w = self->_w;
    src.h = self->_h;
    src.planes = 3;
    pitches[0] = self->_w;
    pitches[1] = self->_w / 2;
    pitches[2] = self->_w / 2;
    src.pitches = pitches;
    pixels[0] = vid_stream->current->image;
    pixels[1] = vid_stream->current->image + pitches[0] * self->_h;
    pixels[2] = vid_stream->current->image + pitches[0] * self->_h +
                                             pitches[1] * self->_h / 2;
    src.pixels = pixels;
#endif

    self->_filter->callback(self->_image, &src, &(self->_srcrect), &info, self->_filter->data );

#ifdef USE_ATI
    vhar128_unlockimage(vid_stream->ati_handle, vid_stream->current->image, &src);
#endif
  }

  /* Now display the image */
  if ( self->_mutex )
    SDL_mutexP( self->_mutex );

  SDL_DisplayYUVOverlay(self->_image, &self->_dstrect);

  if ( self->_callback )
    self->_callback(self->_dst, self->_dstrect.x, self->_dstrect.y, self->_dstrect.w, self->_dstrect.h);

  SDL_UnlockYUVOverlay( self->_image );

  if( self->_filter )
  {
    if( self->_filter->flags & SMPEG_FILTER_INFO_PIXEL_ERROR )
      free(info.yuv_pixel_square_error);
  }

  if ( self->_filter_mutex )
    SDL_mutexV( self->_filter_mutex );
  
  if ( self->_mutex )
    SDL_mutexV( self->_mutex );
}

/*
 *--------------------------------------------------------------
 *
 * ExecuteDisplay --
 *
 *	Actually displays display plane in previously created window.
 *
 * Results:
 *	None.
 *
 * Side effects:
 *	Updates video frame timing control
 *
 *--------------------------------------------------------------
 */


void
MPEGvideo_ExecuteDisplay (_THIS, VidStream* vid_stream )
{
    if( ! vid_stream->_skipFrame )
    {
      MPEGvideo_DisplayFrame (self, vid_stream);

#ifdef CALCULATE_FPS
      TimestampFPS(vid_stream);
#endif
    }
    MPEGvideo_timeSync ( self, vid_stream );
}


SMPEG_Filter *
MPEGvideo_Filter (_THIS, SMPEG_Filter *filter)
{
  SMPEG_Filter * old_filter;

  old_filter = self->_filter;
  if ( self->_filter_mutex )
    SDL_mutexP( self->_filter_mutex );
  self->_filter = filter;
  if ( self->_filter_mutex )
    SDL_mutexV( self->_filter_mutex );

  return(old_filter);
}

/* EOF */
