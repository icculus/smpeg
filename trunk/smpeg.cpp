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
    MPEG *obj;
};

/* Create a new SMPEG object from an MPEG file.
   On return, if 'info' is not NULL, it will be filled with information 
   about the MPEG object.
   This function returns a new SMPEG object.  Use SMPEG_error() to find out
   whether or not there was a problem building the MPEG stream.
   The sdl_audio parameter indicates if SMPEG should initialize the SDL audio
   subsystem. If not, you will have to use the SMPEG_playaudio() function below
   to extract the decoded data.
 */
SMPEG* SMPEG_new(const char *file, SMPEG_Info* info, int sdl_audio)
{
    SMPEG *mpeg;

    /* Create a new SMPEG object! */
    mpeg = new SMPEG;
    mpeg->obj = new MPEG(file, sdl_audio ? true : false);

    /* Find out the details of the stream, if requested */
    SMPEG_getinfo(mpeg, info);

    /* We're done! */
    return(mpeg);
}

/* The same as above except for file descriptors */
SMPEG* SMPEG_new_descr(int file, SMPEG_Info* info, int sdl_audio)
{
    SMPEG *mpeg;

    /* Create a new SMPEG object! */
    mpeg = new SMPEG;
    mpeg->obj = new MPEG(file, sdl_audio ? true : false);

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
        if ( mpeg->obj ) {
            info->has_audio = (mpeg->obj->audiostream != NULL);
            if ( info->has_audio ) {
                mpeg->obj->GetAudioInfo(&ainfo);
		info->audio_current_frame = ainfo.current_frame;
		sprintf(info->audio_string,
		         "MPEG-%d Layer %d %dkbit/s %dHz %s",
			 ainfo.mpegversion+1,
			 ainfo.layer,
			 ainfo.bitrate,
			 ainfo.frequency,
			 (ainfo.mode == 3) ? "mono" : "stereo");
            }
            info->has_video = (mpeg->obj->videostream != NULL);
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
    mpeg->obj->EnableAudio(enable ? true : false);
}

/* Enable or disable video playback in MPEG stream */
void SMPEG_enablevideo( SMPEG* mpeg, int enable )
{
    mpeg->obj->EnableVideo(enable ? true : false);
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
        case MPEG_ERROR:
            status = SMPEG_ERROR;
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
    mpeg->obj->Loop(repeat ? true : false);
}

/* Scale pixel display on an SMPEG object */
void SMPEG_scale( SMPEG* mpeg, int scale )
{
    mpeg->obj->ScaleDisplay(scale);
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

/* Skip 'seconds' seconds of the MPEG */
void SMPEG_skip( SMPEG* mpeg, float seconds )
{
    mpeg->obj->Skip(seconds);
}

/* Render a particular frame in the MPEG video */
void SMPEG_renderFrame( SMPEG* mpeg, int framenum )
{
    mpeg->obj->RenderFrame(framenum);
}

/* Render the last frame of an MPEG video */
void SMPEG_renderFinal( SMPEG* mpeg, SDL_Surface* dst, int x, int y )
{
    mpeg->obj->RenderFinal(dst, x, y);
}

/* Exported function for SDL audio playback */
void SMPEG_playAudio(void *udata, Uint8 *stream, int len)
{
    MPEGaudio *audio = ((SMPEG *)udata)->obj->GetAudio();
    Play_MPEGaudio(audio, stream, len);
}

/* Get the best SDL audio spec for the audio stream */
int SMPEG_wantedSpec( SMPEG *mpeg, SDL_AudioSpec *wanted )
{
    return (int)mpeg->obj->WantedSpec(wanted);
}

/* Inform SMPEG of the actual SDL audio spec used for sound playback */
void SMPEG_actualSpec( SMPEG *mpeg, SDL_AudioSpec *spec )
{
    mpeg->obj->ActualSpec(spec);
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
