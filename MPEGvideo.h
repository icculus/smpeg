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

/* A class based on the MPEG stream class, used to parse and play video */

#ifndef _MPEGVIDEO_H_
#define _MPEGVIDEO_H_

#include "SDL.h"
#include "SDL_thread.h"
#include "MPEGstream.h"
#include "MPEGaction.h"

/* This is the MPEG video stream structure in the mpeg_play code */
struct vid_stream;
typedef struct vid_stream VidStream;

class MPEGvideo : public MPEGerror, public MPEGvideoaction {

    /* Thread to play the video asynchronously */
    friend int Play_MPEGvideo(void *udata);

    /* Various mpeg_play functions that need our data */
    friend void DoDitherImage( VidStream* vid_stream );
    friend void DisplayCurrentFrame( VidStream* vid_stream );
    friend int get_more_data( VidStream* vid_stream );

public:
    MPEGvideo(MPEGstream *stream);
    virtual ~MPEGvideo();

    /* MPEG actions */
    void Loop(bool toggle);
    void Play(void);
    void Stop(void);
    void Rewind(void);
    MPEGstatus Status(void);

    /* MPEG video actions */
    bool GetVideoInfo(MPEG_VideoInfo *info);
    bool SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
                                            MPEG_DisplayCallback callback);
    void MoveDisplay(int x, int y);
    void DoubleDisplay(bool toggle);
    void RenderFrame(int frame, SDL_Surface *dst, int x, int y);
    void RenderFinal(SDL_Surface *dst, int x, int y);

protected:
    MPEGstream *mpeg;

    VidStream* _stream;
    SDL_Surface* _surf;
    SDL_mutex* _mutex;
    SDL_Thread* _thread;

    MPEG_DisplayCallback _callback;

    int _double;        // play back at double size
    int _w;             // width of movie
    int _h;             // height of movie
    int _x;             // pixel x offset
    int _y;             // pixel y offset
    int _uw;            // update width
    int _uh;            // update height

    int _lum[ 8 ];  	// hardcoded LUM_RANGE = 8
    int _cr[ 4 ];   	// hardcoded CR_RANGE = 4
    int _cb[ 4 ];   	// hardcoded CB_RANGE = 4

    void RewindStream(void);
};

#endif /* _MPEGVIDEO_H_ */
