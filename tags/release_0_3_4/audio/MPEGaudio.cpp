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

/* A class based on the MPEG stream class, used to parse and play audio */

#include "MPEGaudio.h"


MPEGaudio:: MPEGaudio(MPEGstream *stream, bool initSDL) : sdl_audio(initSDL)
{
    /* Initialize MPEG audio */
    mpeg = stream;
    initialize();

    /* Just be paranoid.  If all goes well, this will be set to true */
    valid_stream = false;

    /* Analyze the MPEG audio stream */
    if ( loadheader() ) {
        mpeg->reset_stream();

        SDL_AudioSpec wanted;
        WantedSpec(&wanted);

        /* Calculate the samples per frame */
        samplesperframe = 32*wanted.channels;
        if( layer == 3 ) {
            samplesperframe *= 18;
            if ( version == 0 ) {
                samplesperframe *= 2;
            }
        } else {
            samplesperframe *= SCALEBLOCK;
            if ( layer == 2 ) {
                samplesperframe *= 3;
            }
        }
        if ( sdl_audio ) {
            /* Open the audio, get actual audio hardware format and convert */
            bool audio_active;
            SDL_AudioSpec actual;
            audio_active = (SDL_OpenAudio(&wanted, &actual) == 0);
            if ( audio_active ) {
                ActualSpec(&actual);
                valid_stream = true;
            } else {
                SetError(SDL_GetError());
            }
            SDL_PauseAudio(0);
        } else { /* The stream is always valid if we don't initialize SDL */
            valid_stream = true; 
        }
        Volume(100);
    }
}

MPEGaudio:: ~MPEGaudio()
{
    /* Remove ourselves from the mixer hooks */
    Stop();
#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    StopDecoding();
#endif
    if ( sdl_audio ) {
        /* Close up the audio so others may play */
        SDL_CloseAudio();
    }
}

bool
MPEGaudio:: WantedSpec(SDL_AudioSpec *wanted)
{
    wanted->freq = frequencies[version][frequency];
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    wanted->format = AUDIO_S16LSB;
#else
    wanted->format = AUDIO_S16MSB;
#endif
    if ( outputstereo ) {
        wanted->channels = 2;
    } else {
        wanted->channels = 1;
    }
    wanted->samples = 4096;
    wanted->callback = Play_MPEGaudio;
    wanted->userdata = this;
    return true;
}

void
MPEGaudio:: ActualSpec(const SDL_AudioSpec *actual)
{
    /* Splay can optimize some of the conversion */
    if ( actual->channels == 1 && outputstereo ) {
        forcetomonoflag = true;
    }
    if ( actual->channels == 2 && !outputstereo ) {
        forcetostereoflag = true;
        samplesperframe *= 2;
    }
    /* FIXME: Create an audio conversion block */
    if ( (actual->freq/100) == ((frequencies[version][frequency]/2)/100) ) {
        downfrequency = 1;
    } else if ( actual->freq != frequencies[version][frequency] ) {
        fprintf(stderr, "Warning: incorrect audio frequency\n");
    }
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    if ( actual->format != AUDIO_S16LSB)
#else
    if ( actual->format != AUDIO_S16MSB)
#endif
    {
        fprintf(stderr, "Warning: incorrect audio format\n");
    }
    rate_in_s = (double)(((actual->format&0xFF)/8)*actual->channels*actual->freq);
}

#ifdef THREADED_AUDIO
void
MPEGaudio:: StartDecoding(void)
{
    decoding = true;
    /* Create the ring buffer to hold audio */
    if ( ! ring ) {
        ring = new MPEG_ring(samplesperframe*2);
    }
    if ( ! decode_thread ) {
        decode_thread = SDL_CreateThread(Decode_MPEGaudio, this);
    }
}
void
MPEGaudio:: StopDecoding(void)
{
    decoding = false;
    if ( decode_thread ) {
        SDL_WaitThread(decode_thread, NULL);
        decode_thread = NULL;
    }
    if ( ring ) {
        delete ring;
        ring = NULL;
    }
}
#endif

/* MPEG actions */
double
MPEGaudio:: Time(void)
{
    double now;

    if ( frag_time ) {
        now = (play_time + (double)(SDL_GetTicks() - frag_time)/1000.0);
    } else {
        now = 0.0;
    }
    return now;
}
void
MPEGaudio:: Play(void)
{
    ResetPause();
    if ( valid_stream ) {
#ifdef THREADED_AUDIO
        StartDecoding();
#endif
        playing = true;
    }
}
void
MPEGaudio:: Stop(void)
{
    if ( valid_stream ) {
        SDL_LockAudio();
        playing = false;
        SDL_UnlockAudio();
    }
    ResetPause();
}
void
MPEGaudio:: Rewind(void)
{
    Stop();
#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    StopDecoding();
#endif
    mpeg->reset_stream();
    clearrawdata();
    decodedframe = 0;
    currentframe = 0;
    frags_playing = 0;
    frag_time = 0;
    play_time = 0.0;
}
void
MPEGaudio:: Volume(int vol)
{
    if ( (vol >= 0) && (vol <= 100) ) {
        volume = (vol*SDL_MIX_MAXVOLUME)/100;
    }
}
MPEGstatus
MPEGaudio:: Status(void)
{
    if ( valid_stream ) {
        /* Has decoding stopped because of end of stream? */
        if ( mpeg->eof() && (decodedframe <= currentframe) ) {
            return(MPEG_STOPPED);
        }
        /* Have we been told to play? */
        if ( playing ) {
            return(MPEG_PLAYING);
        } else {
            return(MPEG_STOPPED);
        }
    } else {
        return(MPEG_ERROR);
    }
}
