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

/* A virtual class to provide basic MPEG playback actions */

#ifndef _MPEGACTION_H_
#define _MPEGACTION_H_

#include "SDL.h"
#include "MPEGfilter.h"

typedef enum {
    MPEG_ERROR = -1,
    MPEG_STOPPED,
    MPEG_PLAYING
} MPEGstatus;

typedef struct {
    int playing;
    int paused;
    int looping;
    double play_time;
    void (*play)(void *);
    void (*stop)(void *);
    void *parent;
} MPEGaction;

MPEGaction *MPEGaction_create(void *parent, void (*play)(void *), void (*stop)(void *));

void MPEGaction_Play(MPEGaction *self);
void MPEGaction_Stop(MPEGaction *self);
void MPEGaction_Loop(MPEGaction *self, int toggle);
double MPEGaction_Time(MPEGaction *self, int toggle);
void MPEGaction_Pause(MPEGaction *self);

void MPEGaction_ResetPause(MPEGaction *self);

/* For getting info about the audio portion of the stream */
typedef struct MPEG_AudioInfo {
    int mpegversion;
    int mode;
    int frequency;
    int layer;
    int bitrate;
    int current_frame;
} MPEG_AudioInfo;

/* Audio action class */
typedef struct {
    MPEGaction *act;
} MPEGaudioaction;

MPEGaudioaction *MPEGaudioaction_create(void *parent, void (*play)(void *), void (*stop)(void *));

int MPEGaudioaction_GetAudioInfo(MPEGaudioaction *self, MPEG_AudioInfo *info);

/* Matches the declaration of SDL_UpdateRect() */
typedef void(*MPEG_DisplayCallback)(SDL_Surface* dst, int x, int y,
                                     unsigned int w, unsigned int h);

/* For getting info about the video portion of the stream */
typedef struct MPEG_VideoInfo {
    int width;
    int height;
    int current_frame;
    double current_fps;
} MPEG_VideoInfo;

/* Video action class */
typedef struct {
    MPEGaudioaction *time_source;
    MPEGaction *act;
} MPEGvideoaction;

MPEGvideoaction *MPEGvideoaction_create(void *parent, void (*play)(void *), void (*stop)(void *));

void MPEGvideoaction_SetTimeSource(MPEGvideoaction *self, MPEGaudioaction *source);
int MPEGvideoaction_GetVideoInfo(MPEGvideoaction *self, MPEG_VideoInfo *info);

/* For getting info about the system portion of the stream */
typedef struct MPEG_SystemInfo {
    int total_size;
    int current_offset;
    double total_time;
    double current_time;
} MPEG_SystemInfo;

#endif /* _MPEGACTION_H_ */
