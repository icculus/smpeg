/*
   plaympeg - Sample MPEG player using the SMPEG library
   Copyright (C) 1999 Loki Entertainment Software
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#include <stdlib.h>
#include <string.h>

#include "smpeg.h"

void usage(char *argv0)
{
    printf(
"Usage: %s [options] file ...\n"
"Where the options are one of:\n"
"	--noaudio	Don't play audio stream\n"
"	--novideo	Don't play video stream\n"
"	--fullscreen	Play MPEG in fullscreen mode\n"
"	--double or -2	Play MPEG at double size\n"
"	--loop or -l	Play MPEG over and over\n"
"	--volume N or -v N Set audio volume to N (0-100)\n", argv0);
}

void update(SDL_Surface *screen, Sint32 x, Sint32 y, Uint32 w, Uint32 h)
{
    if ( screen->flags & SDL_DOUBLEBUF ) {
        SDL_Flip(screen);
    } else {
        SDL_UpdateRect(screen, x, y, w, h);
    }
}

int main(int argc, char *argv[])
{
    int use_audio, use_video;
    int fullscreen;
    int doublesize;
    int loop_play;
    int i;
    int volume;
    SMPEG *mpeg;
    SMPEG_Info info;
    char *basefile;

    /* Initialize SDL */
    if ( SDL_Init(SDL_INIT_AUDIO|SDL_INIT_VIDEO) < 0 ) {
        fprintf(stderr, "Couldn't init SDL: %s\n", SDL_GetError());
        exit(1);
    }
    atexit(SDL_Quit);

    /* Get the command line options */
    use_audio = 1;
    use_video = 1;
    fullscreen = 0;
    doublesize = 0;
    loop_play = 0;
    volume = 100;
    for ( i=1; argv[i] && (argv[i][0] == '-'); ++i ) {
        if ( strcmp(argv[i], "--noaudio") == 0 ) {
            use_audio = 0;
        } else
        if ( strcmp(argv[i], "--novideo") == 0 ) {
            use_video = 0;
        } else
        if ( strcmp(argv[i], "--fullscreen") == 0 ) {
            fullscreen = 1;
        } else
        if ((strcmp(argv[i], "--double") == 0)||(strcmp(argv[i], "-2") == 0)) {
            doublesize = 1;
        } else
        if ((strcmp(argv[i], "--loop") == 0) || (strcmp(argv[i], "-l") == 0)) {
            loop_play = 1;
        } else
        if ((strcmp(argv[i], "--volume") == 0)||(strcmp(argv[i], "-v") == 0)) {
            ++i;
            if ( argv[i] ) {
                volume = atoi(argv[i]);
            }
        } else
        if ((strcmp(argv[i], "--help") == 0) || (strcmp(argv[i], "-h") == 0)) {
            usage(argv[0]);
            exit(0);
        } else {
            fprintf(stderr, "Warning: Unknown option: %s\n", argv[i]);
        }
    }

    /* Play the mpeg files! */
    for ( ; argv[i]; ++i ) {
        /* Create the MPEG stream */
        mpeg = SMPEG_new(argv[i], &info);
        if ( SMPEG_error(mpeg) ) {
            fprintf(stderr, "%s: %s\n", argv[i], SMPEG_error(mpeg));
            SMPEG_delete(mpeg);
            continue;
        }
        SMPEG_enableaudio(mpeg, use_audio);
        SMPEG_enablevideo(mpeg, use_video);
        SMPEG_setvolume(mpeg, volume);

        /* Print information about the video */
        basefile = strrchr(argv[i], '/');
        if ( basefile ) {
            ++basefile;
        } else {
            basefile = argv[i];
        }
        if ( info.has_audio && info.has_video ) {
            printf("%s: MPEG system stream (audio/video)\n", basefile);
        } else if ( info.has_audio ) {
            printf("%s: MPEG audio stream\n", basefile);
        } else if ( info.has_video ) {
            printf("%s: MPEG video stream\n", basefile);
        }
        if ( info.has_video ) {
            printf("\tVideo %dx%d resolution\n", info.width, info.height);
        }

        /* Set up video display if needed */
        if ( info.has_video && use_video ) {
            SDL_Surface *screen;
            Uint32 video_flags;

            if ( doublesize ) {
                SMPEG_double(mpeg, 1);
                info.width *= 2;
                info.height *= 2;
            }
            video_flags = SDL_SWSURFACE;
            if ( fullscreen ) {
                video_flags = SDL_FULLSCREEN|SDL_DOUBLEBUF|SDL_HWSURFACE;
            }
            screen = SDL_SetVideoMode(info.width, info.height, 0, video_flags);
            SMPEG_setdisplay(mpeg, screen, NULL, update);
        }

        /* Set any special playback parameters */
        if ( loop_play ) {
            SMPEG_loop(mpeg, 1);
        }

        /* Play it, and wait for playback to complete */
        SMPEG_play(mpeg);
        while (!SDL_QuitRequested() && (SMPEG_status(mpeg) == SMPEG_PLAYING)) {
            SDL_Delay(1000/2);
        }
        SMPEG_delete(mpeg);
    }
    exit(0);
}
