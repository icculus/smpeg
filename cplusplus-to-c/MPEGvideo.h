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

/* A class based on the MPEG stream class, used to parse and play video */

#ifndef _MPEGVIDEO_H_
#define _MPEGVIDEO_H_

#include "pthread.h"
#include "SDL.h"
#include "SDL_thread.h"
#include "MPEGerror.h"
#include "MPEGaction.h"

/* This is the MPEG video stream structure in the mpeg_play code */
struct vid_stream;
typedef struct vid_stream VidStream;

/* Temporary definition of time stamp structure. */

typedef double TimeStamp;

typedef struct {
    MPEGstream *mpeg;

    VidStream *_stream;
    SDL_Surface *_dst;
    SDL_mutex *_mutex;
    SDL_Thread *_thread;

    MPEG_DisplayCallback _callback;

    int _ow;            // original width of the movie
    int _oh;            // original height of the movie
    int _w;             // mb aligned width of the movie
    int _h;             // mb aligned height of the movie
    SDL_Rect _srcrect;	// source area
    SDL_Rect _dstrect;	// display area
    SDL_Overlay *_image;// source image
    float _fps;         // frames per second
    SMPEG_Filter *_filter; // pointer to the current filter used
    SDL_mutex *_filter_mutex; // make sure the filter is not changed while being used

    MPEGerror *err;
    MPEGvideoaction *act;
} MPEGvideo;

/* Thread to play the video asynchronously */
int Play_MPEGvideo(void *udata);

/* Various mpeg_play functions that need our data */
VidStream *mpegVidRsrc( TimeStamp time_stamp, VidStream* vid_stream, int first );
int get_more_data( VidStream* vid_stream );

MPEGvideo *MPEGvideo_create(MPEGstream *stream);
MPEGvideo *MPEGvideo_destroy(MPEGvideo *self);

/* MPEG actions */
void MPEGvideo_Play(MPEGvideo *self);
void MPEGvideo_Stop(MPEGvideo *self);
void MPEGvideo_Rewind(MPEGvideo *self);
void MPEGvideo_ResetSynchro(MPEGvideo *self, double time);
void MPEGvideo_Skip(MPEGvideo *self, float seconds);

/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name in popcorn */
MPEGstatus MPEGvideo_GetStatus(MPEGvideo *self);

/* MPEG video actions */
bool MPEGvideo_GetVideoInfo(MPEGvideo *self, MPEG_VideoInfo *info);
bool MPEGvideo_SetDisplay(MPEGvideo *self, SDL_Surface *dst, pthread_mutex_t *lock, MPEG_DisplayCallback callback);
void MPEGvideo_MoveDisplay(MPEGvideo *self, int x, int y);
void MPEGvideo_ScaleDisplayXY(MPEGvideo *self, int w, int h);
void MPEGvideo_SetDisplayRegion(MPEGvideo *self, int x, int y, int w, int h);
void MPEGvideo_RenderFrame(MPEGvideo *self, int frame);
void MPEGvideo_RenderFinal(MPEGvideo *self, SDL_Surface *dst, int x, int y);

SMPEG_Filter *Filter(SMPEG_Filter * filter);

/* Display and sync functions */
void MPEGvideo_DisplayFrame( MPEGvideo *self, VidStream* vid_stream );
void MPEGvideo_ExecuteDisplay( VidStream* vid_stream );
int MPEGvideo_timeSync( VidStream* vid_stream );

/* Yes, it's a hack.. */
MPEGaudioaction *MPEGvideo_TimeSource(MPEGvideo *self);
void RewindStream(void);

#endif /* _MPEGVIDEO_H_ */
