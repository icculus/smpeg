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

/* A class used to parse and play MPEG streams */

#ifndef _MPEG_H_
#define _MPEG_H_

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "SDL.h"

#include "MPEGerror.h"
#include "MPEGstream.h"
#include "MPEGaction.h"
#include "MPEGaudio.h"
#include "MPEGvideo.h"
#include "MPEGsystem.h"
#include "MPEGfilter.h"

#define LENGTH_TO_CHECK_FOR_SYSTEM 0x50000	// Added by HanishKVC

/* The main MPEG class - parses system streams and creates other streams
 A few design notes:
   Making this derived from MPEGstream allows us to do system stream
   parsing.  We create an additional MPEG object for each type of 
   stream in the MPEG file because each needs a separate pointer to
   the MPEG data.  The MPEG stream then creates an accessor object to
   do all the data parsing for that stream type.  It's a little odd,
   but seemed like the best way to implement stream parsing.
 */
struct MPEG
{
    /* We need to have separate audio and video streams */
    MPEGstream * audiostream;
    MPEGstream * videostream;

    MPEGsystem * system;

    char *mpeg_mem;       // Used to copy MPEG passed in as memory
    SDL_RWops *source;
//    MPEGaudioaction *audioaction;
//    MPEGvideoaction *videoaction;
//    MPEGaction *audioaction;
//    MPEGaction *videoaction;

    MPEGaudio *audio;
    MPEGvideo *video;

    bool audio_enabled;
    bool video_enabled;

    bool sdlaudio;

    bool loop;
    bool pause;

    struct MPEGerror *error;
};

typedef struct MPEG MPEG;


#undef _THIS
#define _THIS MPEG *self

void MPEG_parse_stream_list (_THIS);
bool MPEG_seekIntoStream (_THIS, int position);

MPEG * MPEG_init (_THIS);
MPEG * MPEG_init_name (_THIS, const char *path, bool SDLaudio);
MPEG * MPEG_init_descr (_THIS, int Mpeg_FD, bool SDLaudio);
MPEG * MPEG_init_data (_THIS, void *data, int size, bool SDLaudio);
MPEG * MPEG_init_rwops (_THIS, SDL_RWops *mpeg_source, bool SDLaudio);
void MPEG_destroy (_THIS);

void MPEG_Init (_THIS, SDL_RWops *mpeg_source, bool SDLaudio);
void MPEG_InitErrorState (_THIS);

bool MPEG_AudioEnabled (_THIS);
void MPEG_EnableAudio (_THIS, bool enabled);
bool MPEG_VideoEnabled (_THIS);
void MPEG_EnableVideo (_THIS, bool enabled);

/* Dethreaded video. */
void MPEG_run (_THIS);
int MPEG_frametime (_THIS);

void MPEG_Loop (_THIS, bool toggle);
void MPEG_Play (_THIS);
void MPEG_Stop (_THIS);
void MPEG_Rewind (_THIS);
void MPEG_Pause (_THIS);

void MPEG_Skip (_THIS, float seconds);
void MPEG_Seek (_THIS, int bytes);
MPEGstatus MPEG_GetStatus (_THIS);
void MPEG_GetSystemInfo (_THIS, MPEG_SystemInfo *info);

bool MPEG_GetAudioInfo (_THIS, MPEG_AudioInfo *info);
void MPEG_Volume (_THIS, int vol);
bool MPEG_WantedSpec (_THIS, SDL_AudioSpec *wanted);
void MPEG_ActualSpec (_THIS, const SDL_AudioSpec *actual);
MPEGaudio * MPEG_GetAudio (_THIS);

bool MPEG_GetVideoInfo (_THIS, MPEG_VideoInfo *info);
bool MPEG_SetDisplay (_THIS, SDL_Surface *dst, SDL_mutex *lock, MPEG_DisplayCallback callback);
void MPEG_MoveDisplay (_THIS, int x, int y);
void MPEG_ScaleDisplayXY (_THIS, int w, int h);
void MPEG_SetDisplayRegion (_THIS, int x, int y, int w, int h);
void MPEG_RenderFrame (_THIS, int frame);
void MPEG_RenderFinal (_THIS, SDL_Surface *dst, int x, int y);
SMPEG_Filter * MPEG_Filter (_THIS, SMPEG_Filter * filter);

#endif /* _MPEG_H_ */
