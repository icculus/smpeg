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
    MPEGaudioaction *audioaction;
    MPEGvideoaction *videoaction;

    MPEGaudio *audio;
    MPEGvideo *video;

    bool audioaction_enabled;
    bool videoaction_enabled;

    bool sdlaudio;

    bool loop;
    bool pause;

    struct MPEGerror *MPEGerror;
};

typedef struct MPEG MPEG;

MPEG *MPEG_new();
MPEG *MPEG_new_file(const char *path, bool SDLaudio);
MPEG *MPEG_new_fd(int Mpeg_FD, bool SDLaudio);
MPEG *MPEG_new_mem(void *data, int size, bool SDLaudio);
MPEG *MPEG_new_rwops(SDL_RWops *mpeg_source, bool SDLaudio);
void MPEG_destroy(MPEG *self);

void MPEG_Init(MPEG *self, SDL_RWops *mpeg_source, bool SDLaudio);
void MPEG_InitErrorState(MPEG *self);

bool MPEG_AudioEnabled(MPEG *self);
void MPEG_EnableAudio(MPEG *self, bool enabled);
bool MPEG_VideoEnabled(MPEG *self);
void MPEG_EnableVideo(MPEG *self, bool enabled);

void MPEG_Loop(MPEG *self, bool toggle);
void MPEG_Play(MPEG *self);
void MPEG_Stop(MPEG *self);
void MPEG_Rewind(MPEG *self);
void MPEG_Pause(MPEG *self);

void MPEG_Skip(MPEG *self, float seconds);
void MPEG_Seek(MPEG *self, int bytes);
MPEGstatus MPEG_GetStatus(MPEG *self);
void MPEG_GetSystemInfo(MPEG *self, MPEG_SystemInfo *info);

bool MPEG_GetAudioInfo(MPEG *self, MPEG_AudioInfo *info);
void MPEG_Volume(MPEG *self, int vol);
bool MPEG_WantedSpec(MPEG *self, SDL_AudioSpec *wanted);
void MPEG_ActualSpec(MPEG *self, const SDL_AudioSpec *actual);
MPEGaudio *MPEG_GetAudio(MPEG *self);

bool MPEG_GetVideoInfo(MPEG *self, MPEG_VideoInfo *info);
bool MPEG_SetDisplay(MPEG *self, SDL_Surface *dst, SDL_mutex *lock,
		     MPEG_DisplayCallback callback);
void MPEG_MoveDisplay(MPEG *self, int x, int y);
void MPEG_ScaleDisplayXY(MPEG *self, int w, int h);
void MPEG_SetDisplayRegion(MPEG *self, int x, int y, int w, int h);
void MPEG_RenderFrame(MPEG *self, int frame);
void MPEG_RenderFinal(MPEG *self, SDL_Surface *dst, int x, int y);
SMPEG_Filter * MPEG_Filter(MPEG *self, SMPEG_Filter * filter);

#endif /* _MPEG_H_ */
