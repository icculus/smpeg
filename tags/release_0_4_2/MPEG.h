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

/* A class used to parse and play MPEG streams */

#ifndef _MPEG_H_
#define _MPEG_H_

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
   but seemed like the best way do implement stream parsing.
 */
class MPEG : public MPEGerror
{
public:
    MPEG(const char * name, bool sdlaudio = true);
    MPEG(int Mpeg_FD, bool sdlaudio = true);
    MPEG(void *data, int size, bool sdlaudio = true);
    virtual ~MPEG();

    /* Initialize the MPEG */
    void Init(int Mpeg_FD, bool Sdlaudio);
    void Init(void *data, int size, bool Sdlaudio);

    /* Enable/Disable audio and video */
    bool AudioEnabled(void);
    void EnableAudio(bool enabled);
    bool VideoEnabled(void);
    void EnableVideo(bool enabled);

    /* MPEG actions */
    void Loop(bool toggle);
    void Play(void);
    void Stop(void);
    void Rewind(void);
    void Pause(void);
    void Seek(int bytes);
    void Skip(float seconds);
    MPEGstatus Status(void);
    void GetSystemInfo(MPEG_SystemInfo *info);

    /* MPEG audio actions */
    bool GetAudioInfo(MPEG_AudioInfo *info);
    void Volume(int vol);
    bool WantedSpec(SDL_AudioSpec *wanted);
    void ActualSpec(const SDL_AudioSpec *actual);
    MPEGaudio *GetAudio(void);

    /* MPEG video actions */
    bool GetVideoInfo(MPEG_VideoInfo *info);
    bool SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
		                 MPEG_DisplayCallback callback);
    void MoveDisplay(int x, int y);
    void ScaleDisplayXY(int w, int h);
    void SetDisplayRegion(int x, int y, int w, int h);
    void RenderFrame(int frame);
    void RenderFinal(SDL_Surface *dst, int x, int y);
    SMPEG_Filter * Filter(SMPEG_Filter * filter);

public:
    /* We need to have separate audio and video streams */
    MPEGstream * audiostream;
    MPEGstream * videostream;

    MPEGsystem * system;

protected:
    int mpeg_fd;
    bool close_fd;

    MPEGaudioaction *audioaction;
    MPEGvideoaction *videoaction;

    MPEGaudio *audio;
    MPEGvideo *video;

    bool audioaction_enabled;
    bool videoaction_enabled;

    bool sdlaudio;

    bool loop;
    bool pause;

    void parse_stream_list();
    bool seekIntoStream(int position);
};

#endif /* _MPEG_H_ */
