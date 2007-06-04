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

/* audioaction and videoaction are derived from this. */

#ifndef _MPEGACTION_H_
#define _MPEGACTION_H_

#include "SDL.h"
#include "MPEGerror.h"
#include "MPEGfilter.h"

typedef enum {
    MPEG_ERROR = -1,
    MPEG_STOPPED,
    MPEG_PLAYING
} MPEGstatus;


#if 0
/* Base class. */
class MPEGaction {
public:
    MPEGaction() {
        playing = false;
        paused = false;
        looping = false;
	play_time = 0.0;
    }
    virtual void Loop(bool toggle) {
        looping = toggle;
    }
    virtual double Time(void) {  /* Returns the time in seconds since start */
        return play_time;
    }
    virtual void Play(void) = 0;
    virtual void Stop(void) = 0;
    virtual void Rewind(void) = 0;
    virtual void ResetSynchro(double) = 0;
    virtual void Skip(float seconds) = 0;
    virtual void Pause(void) {  /* A toggle action */
        if ( paused ) {
            paused = false;
            Play();
        } else {
            Stop();
            paused = true;
        }
    }
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  conflict name in popcorn */
    virtual MPEGstatus GetStatus(void) = 0;

protected:
    bool playing;
    bool paused;
    bool looping;
    double play_time;

    void ResetPause(void) {
        paused = false;
    }
};




/* audioaction, derived from action. */


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
class MPEGaudioaction : public MPEGaction {
public:
    virtual bool GetAudioInfo(MPEG_AudioInfo *info) {
        return(true);
    }
    virtual void Volume(int vol) = 0;
};



/* videoaction, derived from action */

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
class MPEGvideoaction : public MPEGaction {
public:
    virtual void SetTimeSource(MPEGaudioaction *source) {
        time_source = source;
    }
    virtual bool GetVideoInfo(MPEG_VideoInfo *info) {
        return(false);
    }
    virtual bool SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
                                MPEG_DisplayCallback callback) = 0;
    virtual void MoveDisplay(int x, int y) = 0;
    virtual void ScaleDisplayXY(int w, int h) = 0;
    virtual void SetDisplayRegion(int x, int y, int w, int h) = 0;
    virtual void RenderFrame(int frame) = 0;
    virtual void RenderFinal(SDL_Surface *dst, int x, int y) = 0;
    virtual SMPEG_Filter * Filter(SMPEG_Filter * filter) = 0;
protected:
    MPEGaudioaction *time_source;
};


/* For getting info about the system portion of the stream */
typedef struct MPEG_SystemInfo {
    int total_size;
    int current_offset;
    double total_time;
    double current_time;
} MPEG_SystemInfo;
#endif /* 0 */


/**********
* Start C *
***********/


struct MPEG_SystemInfo {
    int total_size;
    int current_offset;
    double total_time;
    double current_time;
};

typedef struct MPEG_SystemInfo MPEG_SystemInfo;



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
typedef void(*MPEG_DisplayCallback)(SDL_Surface* dst, int x, int y, unsigned int w, unsigned int h);

/* For getting info about the video portion of the stream */
struct MPEG_VideoInfo {
    int width;
    int height;
    int current_frame;
    double current_fps;
};

typedef struct MPEG_VideoInfo MPEG_VideoInfo;




#if 0
struct MPEGaction {
    /* base */
    bool playing;
    bool paused;
    bool looping;
    double play_time;

    /* audio */

    /* video */
//    MPEGaudioaction *time_source;
    struct MPEGaudio *time_source;
};

typedef struct MPEGaction MPEGaction;



#undef _THIS
#define _THIS MPEGaction *self


/* base methods. */
MPEGaction * MPEGaction_init (_THIS);
void MPEGaction_destroy (_THIS);
void MPEGaction_ResetPause (_THIS);
/*virtual*/ void MPEGaction_Loop (_THIS, bool toggle);
/*virtual*/ double MPEGaction_Time (_THIS); /* Returns the time in seconds since start */
/*virtual*/ double MPEGaction_Play (_THIS);
/*virtual*/ double MPEGaction_Stop (_THIS);
/*virtual*/ double MPEGaction_Rewind (_THIS);
/*virtual*/ double MPEGaction_ResetSynchro (_THIS, double);
/*virtual*/ double MPEGaction_Skip (_THIS, float);
/*virtual*/ void MPEGaction_Pause (_THIS); /* A toggle action */
/*virtual*/ MPEGstatus MPEGaction_GetStatus (_THIS);

#endif /* 0 */



#endif /* _MPEGACTION_H_ */
