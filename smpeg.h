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

#ifndef _SMPEG_H_
#define _SMPEG_H_

#include "SDL.h"
#include "SDL_mutex.h"
#include "SDL_audio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* This is the actual SMPEG object */
typedef struct _SMPEG SMPEG;

/* Used to get information about the SMPEG object */
typedef struct _SMPEG_Info {
    int has_audio;
    int has_video;
    int width;
    int height;
    int current_frame;
    double current_fps;
} SMPEG_Info;

/* Possible MPEG status codes */
typedef enum {
    SMPEG_ERROR = -1,
    SMPEG_STOPPED,
    SMPEG_PLAYING
} SMPEGstatus;

/* Matches the declaration of SDL_UpdateRect() */
typedef void(*SMPEG_DisplayCallback)(SDL_Surface* dst, int x, int y,
                                     unsigned int w, unsigned int h);


/* Create a new SMPEG object from an MPEG file.
   On return, if 'info' is not NULL, it will be filled with information 
   about the MPEG object.
   This function returns a new SMPEG object.  Use SMPEG_error() to find out
   whether or not there was a problem building the MPEG stream.
   The sdl_audio parameter indicates if SMPEG should initialize the SDL audio
   subsystem. If not, you will have to use the SMPEG_playaudio() function below
   to extract the decoded data.
 */
extern SMPEG* SMPEG_new(const char *file, SMPEG_Info* info, int sdl_audio);

/* Get current information about an SMPEG object */
extern void SMPEG_getinfo( SMPEG* mpeg, SMPEG_Info* info );

/* Enable or disable audio playback in MPEG stream */
extern void SMPEG_enableaudio( SMPEG* mpeg, int enable );

/* Enable or disable video playback in MPEG stream */
extern void SMPEG_enablevideo( SMPEG* mpeg, int enable );

/* Delete an SMPEG object */
extern void SMPEG_delete( SMPEG* mpeg );

/* Get the current status of an SMPEG object */
extern SMPEGstatus SMPEG_status( SMPEG* mpeg );

/* Set the audio volume of an MPEG stream, in the range 0-100 */
extern void SMPEG_setvolume( SMPEG* mpeg, int volume );

/* Set the destination surface for MPEG video playback
   'surfLock' is a mutex used to synchronize access to 'dst', and can be NULL.
   'callback' is a function called when an area of 'dst' needs to be updated.
   If 'callback' is NULL, the default function (SDL_UpdateRect) will be used.
*/
extern void SMPEG_setdisplay(SMPEG* mpeg, SDL_Surface* dst, SDL_mutex* surfLock,
                                            SMPEG_DisplayCallback callback);

/* Set or clear looping play on an SMPEG object */
extern void SMPEG_loop( SMPEG* mpeg, int repeat );

/* Set or clear pixel-doubled display on an SMPEG object */
extern void SMPEG_double( SMPEG* mpeg, int big );

/* Move the video display area within the destination surface */
extern void SMPEG_move( SMPEG* mpeg, int x, int y );

/* Play an SMPEG object */
extern void SMPEG_play( SMPEG* mpeg );

/* Pause/Resume playback of an SMPEG object */
extern void SMPEG_pause( SMPEG* mpeg );

/* Stop playback of an SMPEG object */
extern void SMPEG_stop( SMPEG* mpeg );

/* Rewind the play position of an SMPEG object to the beginning of the MPEG */
extern void SMPEG_rewind( SMPEG* mpeg );

/* Render a particular frame in the MPEG video */
extern void SMPEG_renderFrame( SMPEG* mpeg, int framenum, SDL_Surface* dst,
                               int x, int y );

/* Render the last frame of an MPEG video */
extern void SMPEG_renderFinal( SMPEG* mpeg, SDL_Surface* dst, int x, int y );

/* Return NULL if there is no error in the MPEG stream, or an error message
   if there was a fatal error in the MPEG stream for the SMPEG object.
*/
extern char *SMPEG_error( SMPEG* mpeg );

/* Exported callback function for SDL audio playback.
   The data parameter must be a pointer to the SMPEG object, casted to void *.
*/
extern void SMPEG_playAudio( void *mpeg, Uint8 *stream, int len );

/* Get the best SDL audio spec for the audio stream */
extern int SMPEG_wantedSpec( SMPEG *mpeg, SDL_AudioSpec *wanted );

/* Inform SMPEG of the actual SDL audio spec used for sound playback */
extern void SMPEG_actualSpec( SMPEG *mpeg, SDL_AudioSpec *spec );

#ifdef __cplusplus
};
#endif
#endif /* _SMPEG_H_ */
