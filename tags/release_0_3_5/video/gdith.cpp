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
#include <sys/time.h>
#include "SDL_timer.h"

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

/* Range values for lum, cr, cb. */
int LUM_RANGE;
int CR_RANGE;
int CB_RANGE;

/* Arrays holding quantized value ranged for lum, cr, and cb. */
int *lum_values;
int *cr_values;
int *cb_values;

/* Frame Rate Info */
extern int framerate;

/* Video rates table */
/* Cheat on Vid rates, round to 30, and use 30 if illegal value 
   Except for 9, where Xing means 15, and given their popularity, we'll
   be nice and do it */
static int VidRateNum[16]={ 30, 24, 24, 25, 30, 30, 50, 60, 
                            60, 15, 30, 30, 30, 30, 30, 30 };

/* Luminance and chrominance lookup tables */
static double *L_tab, *Cr_r_tab, *Cr_g_tab, *Cb_g_tab, *Cb_b_tab;


/*
 *--------------------------------------------------------------
 *
 * InitColor --
 *
 *	Initialize lum, cr, and cb quantized range value arrays.
 *      Also initializes the lookup tables for the possible
 *      values of lum, cr, and cb.
 *    Color values from ITU-R BT.470-2 System B, G and SMPTE 170M
 *    see InitColorDither in 16bits.c for more
 *
 * Results: 
 *      None.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

void InitColor()
{
  int i, CR, CB;

  if ( L_tab ) free(L_tab);
  L_tab    = (double *)malloc(LUM_RANGE*sizeof(double)); 
  if ( Cr_r_tab ) free(Cr_r_tab);
  Cr_r_tab = (double *)malloc(CR_RANGE*sizeof(double));
  if ( Cr_g_tab ) free(Cr_g_tab);
  Cr_g_tab = (double *)malloc(CR_RANGE*sizeof(double));
  if ( Cb_g_tab ) free(Cb_g_tab);
  Cb_g_tab = (double *)malloc(CB_RANGE*sizeof(double));
  if ( Cb_b_tab ) free(Cb_b_tab);
  Cb_b_tab = (double *)malloc(CB_RANGE*sizeof(double));

  if (L_tab == NULL    || Cr_r_tab == NULL ||
      Cr_g_tab == NULL || Cb_g_tab == NULL ||
      Cb_b_tab == NULL) {
    fprintf(stderr, "Could not alloc memory in InitColor\n");
    exit(1);
  }

  for (i=0; i<LUM_RANGE; i++) {
    lum_values[i]  = ((i * 256) / (LUM_RANGE)) + (256/(LUM_RANGE*2));
    L_tab[i] = lum_values[i];
    if (gammaCorrectFlag) {
      L_tab[i] = GAMMA_CORRECTION(L_tab[i]);
    }
  }
  
  for (i=0; i<CR_RANGE; i++) {
    register double tmp;
    if (chromaCorrectFlag) {
      tmp = ((i * 256) / (CR_RANGE)) + (256/(CR_RANGE*2));
      Cr_r_tab[i] = (int) (0.419/0.299) * CHROMA_CORRECTION128D(tmp - 128.0);
      Cr_g_tab[i] = (int) -(0.299/0.419) * CHROMA_CORRECTION128D(tmp - 128.0);
      cr_values[i] = CHROMA_CORRECTION256(tmp);
    } else {
      tmp = ((i * 256) / (CR_RANGE)) + (256/(CR_RANGE*2));
      Cr_r_tab[i] = (int)  (0.419/0.299) * (tmp - 128.0);
      Cr_g_tab[i] = (int) -(0.299/0.419) * (tmp - 128.0);
      cr_values[i] = (int) tmp;
    }
  }

  for (i=0; i<CB_RANGE; i++) {
    register double tmp;
    if (chromaCorrectFlag) {
      tmp = ((i * 256) / (CB_RANGE)) + (256/(CB_RANGE*2));
      Cb_g_tab[i] = (int) -(0.114/0.331) * CHROMA_CORRECTION128D(tmp - 128.0);
      Cb_b_tab[i] = (int)  (0.587/0.331) * CHROMA_CORRECTION128D(tmp - 128.0);
      cb_values[i] = CHROMA_CORRECTION256(tmp);
    } else {
      tmp = ((i * 256) / (CB_RANGE)) + (256/(CB_RANGE*2));
      Cb_g_tab[i] = (int) -(0.114/0.331) * (tmp - 128.0);
      Cb_b_tab[i] = (int)  (0.587/0.331) * (tmp - 128.0);
      cb_values[i] = (int) tmp;
    }
  }

}


/*
 *--------------------------------------------------------------
 *
 * ConvertColor --
 *
 *	Given a l, cr, cb tuple, converts it to r,g,b.
 *
 * Results:
 *	r,g,b values returned in pointers passed as parameters.
 *
 * Side effects:
 *      None.
 *
 *--------------------------------------------------------------
 */

void ConvertColor( unsigned int l, unsigned int cr, unsigned int cb,
                   unsigned char* r, unsigned char* g, unsigned char* b )
{
    double fl, fcr, fcb, fr, fg, fb;

    /*
     * Old method w/o lookup table
     *
     * fl = 1.164*(((double) l)-16.0);
     * fcr =  ((double) cr) - 128.0;
     * fcb =  ((double) cb) - 128.0;
     *
     * fr = fl + (1.366 * fcr);
     * fg = fl - (0.700 * fcr) - (0.334 * fcb);
     * fb = fl + (1.732 * fcb);
     */

    fl = L_tab[l];

    fr = fl + Cr_r_tab[cr];
    fg = fl + Cr_g_tab[cr] + Cb_g_tab[cb];
    fb = fl + Cb_b_tab[cb];

    if (fr < 0.0) fr = 0.0;
    else if (fr > 255.0) fr = 255.0;

    if (fg < 0.0) fg = 0.0;
    else if (fg > 255.0) fg = 255.0;

    if (fb < 0.0) fb = 0.0;
    else if (fb > 255.0) fb = 255.0;

    *r = (unsigned char) fr;
    *g = (unsigned char) fg;
    *b = (unsigned char) fb;
}


#ifdef CALCULATE_FPS
static inline void TimestampFPS( VidStream* vid_stream )
{
    MPEGvideo* mpeg = (MPEGvideo*) vid_stream->_smpeg;

    vid_stream->frame_time[vid_stream->timestamp_index] = mpeg->Time();
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
    double now;

    if ( mpeg->TimeSource() ) {
        now = mpeg->TimeSource()->Time();
    } else {
        now = ReadSysClock() - vid_stream->realTimeStart;
    }
    return now;
}

int timeSync( VidStream* vid_stream )
{
    MPEGvideo* mpeg = (MPEGvideo*) vid_stream->_smpeg;

    /* Update the number of frames displayed */
    vid_stream->totNumFrames++;

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
    mpeg->play_time += vid_stream->_oneFrameTime;

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
        time_behind = CurrentTime(vid_stream) - mpeg->Time();

#ifdef DEBUG_MPEG_SCHEDULING
//printf("Frame %d: frame time: %f, real time: %f, time behind: %f\n", vid_stream->totNumFrames, mpeg->Time(), CurrentTime(vid_stream), time_behind);
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
                mpeg->play_time = CurrentTime(vid_stream) - (MAX_FUDGE_TIME*2);
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
   This has been optimized for writing to 16-bit SDL surfaces.
*/
void DisplayCurrentFrame( VidStream* vid_stream )
{
#ifdef USE_MMX
    extern int mmx_available;
#endif
    unsigned char* l = vid_stream->current->luminance;
    unsigned char* Cr = vid_stream->current->Cr;
    unsigned char* Cb = vid_stream->current->Cb;
    unsigned char* disp;
    MPEGvideo* mpeg = (MPEGvideo*) vid_stream->_smpeg;

    if ( SDL_MUSTLOCK(mpeg->_surf) ) {
        if ( SDL_LockSurface(mpeg->_surf) < 0 ) {
            return;
        }
    }
    disp = (unsigned char*) mpeg->_surf->pixels;
    disp += mpeg->_x * 2;
    disp += mpeg->_surf->pitch * mpeg->_y;

    if ( mpeg->_mutex )
        SDL_mutexP( mpeg->_mutex );

    if( mpeg->_scale != 1 )
    {
#ifdef USE_INTERLACED_VIDEO
        static int start = 1;
        if ( mpeg->_surf->format->BytesPerPixel == 2 ) {
	  ScaleColor16DitherImageModInterlace( l, Cr, Cb, disp,
					       vid_stream->v_size, vid_stream->h_size,
					       (mpeg->_surf->pitch / 2) - (vid_stream->h_size * mpeg->_scale), start, mpeg->_scale);
	}
        start = !start;
#else
        if ( mpeg->_surf->format->BytesPerPixel == 2 ) {
            ScaleColor16DitherImageMod( l, Cr, Cb, disp,
					vid_stream->v_size, vid_stream->h_size,
					(mpeg->_surf->pitch / 2) - (vid_stream->h_size * mpeg->_scale), mpeg->_scale);
        } else
        if ( mpeg->_surf->format->BytesPerPixel == 4 ) {
            ScaleColor32DitherImageMod( l, Cr, Cb, disp,
					vid_stream->v_size, vid_stream->h_size,
					(mpeg->_surf->pitch / 4) - (vid_stream->h_size * mpeg->_scale), mpeg->_scale);
        }
#endif
        if ( SDL_MUSTLOCK(mpeg->_surf) ) {
            SDL_UnlockSurface(mpeg->_surf);
        }
        mpeg->_callback( mpeg->_surf, mpeg->_x, mpeg->_y,
                     vid_stream->h_size * mpeg->_scale, vid_stream->v_size * mpeg->_scale );
    }
    else
    {
#ifdef USE_INTERLACED_VIDEO
        static int start = 1;
        if ( mpeg->_surf->format->BytesPerPixel == 2 ) {
        	Color16DitherImageModInterlace( l, Cr, Cb, disp,
                         vid_stream->v_size, vid_stream->h_size,
                         (mpeg->_surf->pitch / 2) - vid_stream->h_size, start );
	}
        start = !start;
#else
        if ( mpeg->_surf->format->BytesPerPixel == 2 ) {
#ifdef USE_MMX
            if ( mmx_available ) {
                Color16DitherImageMMX( l, Cr, Cb, disp,
                         vid_stream->v_size, vid_stream->h_size,
                         (mpeg->_surf->pitch / 2) - vid_stream->h_size );
            } else
#endif
            Color16DitherImageMod( l, Cr, Cb, disp,
                         vid_stream->v_size, vid_stream->h_size,
                         (mpeg->_surf->pitch / 2) - vid_stream->h_size );
        } else
        if ( mpeg->_surf->format->BytesPerPixel == 4 ) {
#ifdef USE_MMX
            if ( mmx_available ) {
                Color32DitherImageMMX( l, Cr, Cb, disp,
                         vid_stream->v_size, vid_stream->h_size,
                         (mpeg->_surf->pitch / 4) - vid_stream->h_size );
            } else
#endif
            Color32DitherImageMod( l, Cr, Cb, disp,
                         vid_stream->v_size, vid_stream->h_size,
                         (mpeg->_surf->pitch / 4) - vid_stream->h_size );
        }
#endif
        if ( SDL_MUSTLOCK(mpeg->_surf) ) {
            SDL_UnlockSurface(mpeg->_surf);
        }
        mpeg->_callback( mpeg->_surf, mpeg->_x, mpeg->_y,
                         vid_stream->h_size, vid_stream->v_size );
    }

    if ( mpeg->_mutex )
        SDL_mutexV( mpeg->_mutex );
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


void ExecuteDisplay( VidStream* vid_stream )
{
    if( ! vid_stream->_skipFrame )
    {
        DisplayCurrentFrame(vid_stream);
#ifdef CALCULATE_FPS
        TimestampFPS(vid_stream);
#endif
    }
    timeSync( vid_stream );
}


/* EOF */
