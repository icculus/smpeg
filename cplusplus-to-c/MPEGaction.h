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

#define bool int
#define false 0
#define true (!false)



/* For getting info about the audio portion of the stream */
struct MPEG_AudioInfo {
    int mpegversion;
    int mode;
    int frequency;
    int layer;
    int bitrate;
    int current_frame;
};

typedef struct MPEG_AudioInfo MPEG_AudioInfo;



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



/* For getting info about the system portion of the stream */
typedef struct MPEG_SystemInfo {
    int total_size;
    int current_offset;
    double total_time;
    double current_time;
} MPEG_SystemInfo;



#undef _THIS
#define _THIS struct MPEGaction*
typedef struct MPEGaction {
  int playing;
  int paused;
  int looping;
  double play_time;
  void (*Loop)(_THIS, int);
  double (*Time)(_THIS);
  void (*Pause)(_THIS);
  void (*Play)(_THIS);
  void (*Stop)(_THIS);
  void (*Rewind)(_THIS);
  void (*ResetSynchro)(_THIS, double);
  void (*Skip)(_THIS, float);
  void (*ResetPause)(_THIS);
  MPEGstatus (*GetStatus)(_THIS);
/* MPEGaudioaction extends this with GetAudioInfo and Volume() */
  bool (*GetAudioInfo)(_THIS, MPEG_AudioInfo *);
  void (*Volume)(_THIS, int);
/* MPEGvideoaction extends this with SetTimeSource(), GetVideoInfo(),
 SetDisplay(), MoveDisplay(), ScaleDisplayXY(), SetDisplayRegion(),
 RenderFrame(), RenderFinal(), Final(), time_source */
//  void (*SetTimeSource)(_THIS, struct MPEGaudio *);
  bool (*GetVideoInfo)(_THIS, MPEG_VideoInfo *info);
  bool (*SetDisplay)(_THIS, SDL_Surface *, SDL_mutex *, MPEG_DisplayCallback);
  void (*MoveDIsplay)(_THIS, int, int);
  void (*ScaleDisplayXY)(_THIS, int, int);
  void (*SetDisplayRegion)(_THIS, int, int, int, int);
  void (*RenderFrame)(_THIS, int);
  void (*RenderFinal)(_THIS, SDL_Surface *, int, int);
  SMPEG_Filter * (*Final)(_THIS, SMPEG_Filter *filter);
//  struct MPEGaudio *time_source;
} MPEGaction;


MPEGaction *MPEGaction_init(_THIS);
MPEGaction *MPEGaction_new();

typedef struct MPEGaction MPEGaudioaction;
typedef struct MPEGaction MPEGvideoaction;


/*
Pure virtuals:
 Play
 Stop
 Rewind
 ResetSynchro
 Skip
 GetStatus
Virtuals:
 Loop
 Time
 Pause
*/

#undef _THIS








#endif /* _MPEGACTION_H_ */
