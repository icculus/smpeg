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

/* This is the C interface to the SMPEG library */

#include "MPEG.h"
#include "smpeg.h"

extern "C" {

/* This is the actual SMPEG object */
struct _SMPEG {
    MPEGfile *obj;
};

/* Create a new SMPEG object from an MPEG file.
   If 'dst' is NULL, then video will not be played.  'surfLock' is a mutex
   used to synchronize access to 'dst'.  'callback' is a function called 
   when an area of 'dst' needs to be updated.  If 'callback' is NULL, the
   default update function (SDL_UpdateRect) will be used.
   On return, if 'info' is not NULL, it will be filled with information 
   about the MPEG object.
   This function returns a new SMPEG object, or NULL if there was an error.
 */
SMPEG* SMPEG_new(const char *file, SMPEG_Info* info)
{
    SMPEG *mpeg;

    /* Create a new SMPEG object! */
    mpeg = new SMPEG;
    mpeg->obj = new MPEGfile(file);

    /* Find out the details of the stream, if requested */
    SMPEG_getinfo(mpeg, info);

    /* We're done! */
    return(mpeg);
}

/* Get current information about an SMPEG object */
void SMPEG_getinfo( SMPEG* mpeg, SMPEG_Info* info )
{
    if ( info ) {
        MPEG_AudioInfo ainfo;
        MPEG_VideoInfo vinfo;

        memset(info, 0, (sizeof *info));
        if ( mpeg->obj->mpeg ) {
            info->has_audio = (mpeg->obj->mpeg->audiostream != NULL);
            if ( info->has_audio ) {
                mpeg->obj->GetAudioInfo(&ainfo);
            }
            info->has_video = (mpeg->obj->mpeg->videostream != NULL);
            if ( info->has_video ) {
                mpeg->obj->GetVideoInfo(&vinfo);
                info->width = vinfo.width;
                info->height = vinfo.height;
                info->current_frame = vinfo.current_frame;
                info->current_fps = vinfo.current_fps;
            }
        }
    }
}

/* Enable or disable audio playback in MPEG stream */
void SMPEG_enableaudio( SMPEG* mpeg, int enable )
{
    mpeg->obj->EnableAudio(enable);
}

/* Enable or disable video playback in MPEG stream */
void SMPEG_enablevideo( SMPEG* mpeg, int enable )
{
    mpeg->obj->EnableVideo(enable);
}

/* Delete an SMPEG object */
void SMPEG_delete( SMPEG* mpeg )
{
    delete mpeg->obj;
    delete mpeg;
}

/* Get the current status of an SMPEG object */
SMPEGstatus SMPEG_status( SMPEG* mpeg )
{
    SMPEGstatus status;

    status = SMPEG_ERROR;
    switch (mpeg->obj->Status()) {
        case MPEG_STOPPED:
            if ( ! mpeg->obj->WasError() ) {
                status = SMPEG_STOPPED;
            }
            break;
        case MPEG_PLAYING:
            status = SMPEG_PLAYING;
            break;
    }
    return(status);
}

/* Set the audio volume of an MPEG stream */
void SMPEG_setvolume( SMPEG* mpeg, int volume )
{
    mpeg->obj->Volume(volume);
}

/* Set the destination surface for MPEG video playback */
void SMPEG_setdisplay( SMPEG* mpeg, SDL_Surface* dst, SDL_mutex* surfLock,
                                            SMPEG_DisplayCallback callback)
{
    mpeg->obj->SetDisplay(dst, surfLock, callback);
}

/* Set or clear looping play on an SMPEG object */
void SMPEG_loop( SMPEG* mpeg, int repeat )
{
    mpeg->obj->Loop(repeat);
}

/* Set or clear pixel-doubled display on an SMPEG object */
void SMPEG_double( SMPEG* mpeg, int big )
{
    mpeg->obj->DoubleDisplay(big);
}

/* Move the video display area within the destination surface */
void SMPEG_move( SMPEG* mpeg, int x, int y )
{
    mpeg->obj->MoveDisplay(x, y);
}

/* Play an SMPEG object */
void SMPEG_play( SMPEG* mpeg )
{
    mpeg->obj->Play();
}

/* Pause/Resume playback of an SMPEG object */
void SMPEG_pause( SMPEG* mpeg )
{
    mpeg->obj->Pause();
}

/* Stop playback of an SMPEG object */
void SMPEG_stop( SMPEG* mpeg )
{
    mpeg->obj->Stop();
}

/* Rewind the play position of an SMPEG object to the beginning of the MPEG */
void SMPEG_rewind( SMPEG* mpeg )
{
    mpeg->obj->Rewind();
}

/* Render a particular frame in the MPEG video */
void SMPEG_renderFrame( SMPEG* mpeg,
                        int framenum, SDL_Surface* dst, int x, int y )
{
    mpeg->obj->RenderFrame(framenum, dst, x, y);
}

/* Render the last frame of an MPEG video */
void SMPEG_renderFinal( SMPEG* mpeg, SDL_Surface* dst, int x, int y )
{
    mpeg->obj->RenderFinal(dst, x, y);
}

/* Return NULL if there is no error in the MPEG stream, or an error message
   if there was a fatal error in the MPEG stream for the SMPEG object.
*/
char *SMPEG_error( SMPEG* mpeg )
{
    char *error;

    error = NULL;
    if ( mpeg->obj->WasError() ) {
        error = mpeg->obj->TheError();
    }
    return(error);
}

/* Extern "C" */
};
