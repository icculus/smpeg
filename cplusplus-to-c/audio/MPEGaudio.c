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

/* A class based on the MPEG stream class, used to parse and play audio */

#include <stdlib.h>
#include <string.h>

#include "MPEGaudio.h"
#include "MPEGstream.h"


#undef _THIS
#define _THIS MPEGaudio *self
#undef METH
#define METH(m) MPEGaudio_##m
#undef PROP
#define PROP(p) (self->p)

MPEGaudio *
METH(init) (_THIS, MPEGstream *stream, bool initSDL)
{
    int i;

    MAKE_OBJECT(MPEGaudio);
    self->sdl_audio = initSDL;
    self->error = MPEGerror_init(self->error);
//    self->action = MPEGaction_init(self->action);
    PROP(playing) = false;
    PROP(paused) = false;
    PROP(looping) = false;
    PROP(play_time) = 0.0;

    /* Initialize MPEG audio */
    PROP(mpeg) = stream;
    METH(initialize) (self);

    /* Just be paranoid.  If all goes well, this will be set to true */
    PROP(valid_stream) = false;

    /* Analyze the MPEG audio stream */
    if ( METH(loadheader)(self) ) {
        SDL_AudioSpec wanted;
        METH(WantedSpec)(self, &wanted);

        /* Calculate the samples per frame */
        PROP(samplesperframe) = 32*wanted.channels;
        if( PROP(layer) == 3 ) {
            PROP(samplesperframe) *= 18;
            if ( PROP(version) == 0 ) {
                PROP(samplesperframe) *= 2;
            }
        } else {
            PROP(samplesperframe) *= SCALEBLOCK;
            if ( PROP(layer) == 2 ) {
                PROP(samplesperframe) *= 3;
            }
        }
        if ( PROP(sdl_audio) ) {
            /* Open the audio, get actual audio hardware format and convert */
            bool audio_active;
            SDL_AudioSpec actual;
            audio_active = (SDL_OpenAudio(&wanted, &actual) == 0);
            if ( audio_active ) {
                METH(ActualSpec)(self, &actual);
                PROP(valid_stream) = true;
            } else {
                MPEGerror_SetError(PROP(error), SDL_GetError());
            }
            SDL_PauseAudio(0);
        } else { /* The stream is always valid if we don't initialize SDL */
            PROP(valid_stream) = true; 
        }
        METH(Volume)(self, 100);
    }

    /* For using system timestamp */
    for (i=0; i<N_TIMESTAMPS; i++)
      PROP(timestamp[i]) = -1;

  return self;
}

void
METH(destroy) (_THIS)
{
#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    METH(StopDecoding)(self);
#endif

    /* Remove ourselves from the mixer hooks */
    METH(Stop)(self);
    if ( PROP(sdl_audio) ) {
        /* Close up the audio so others may play */
        SDL_CloseAudio();
    }

//  MPEGaction_destroy(PROP(action));
//  free(PROP(action));
//  PROP(action) = NULL;
  PROP(playing) = false;
  PROP(paused) = false;
  PROP(looping) = false;
  PROP(play_time) = 0.0;

  MPEGerror_destroy(PROP(error));
  free(PROP(error));
  PROP(error) = NULL;

}

bool
METH(WantedSpec) (_THIS, SDL_AudioSpec *wanted)
{
    wanted->freq = MPEGaudio_frequencies[PROP(version)][PROP(frequency)];
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    wanted->format = AUDIO_S16LSB;
#else
    wanted->format = AUDIO_S16MSB;
#endif
    if ( PROP(outputstereo) ) {
        wanted->channels = 2;
    } else {
        wanted->channels = 1;
    }
    wanted->samples = 4096;
    wanted->callback = Play_MPEGaudioSDL;
    wanted->userdata = (void*)self;
    return true;
}

void
METH(ActualSpec) (_THIS, const SDL_AudioSpec *actual)
{
    /* Splay can optimize some of the conversion */
    if ( actual->channels == 1 && PROP(outputstereo) ) {
        PROP(forcetomonoflag) = true;
    }
    if ( actual->channels == 2 && !PROP(outputstereo) ) {
        PROP(forcetostereoflag) = true;
        PROP(samplesperframe) *= 2;
    }
    /* FIXME: Create an audio conversion block */
    if ( (actual->freq/100) == ((MPEGaudio_frequencies[PROP(version)][PROP(frequency)]/2)/100) ) {
        PROP(downfrequency) = 1;
    } else if ( actual->freq != MPEGaudio_frequencies[PROP(version)][PROP(frequency)] ) {
#ifdef VERBOSE_WARNINGS
        fprintf(stderr, "Warning: wrong audio frequency (wanted %d, got %d)\n",
		MPEGaudio_frequencies[PROP(version)][PROP(frequency)], actual->freq);
#else
	;
#endif
    }
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    if ( actual->format != AUDIO_S16LSB)
#else
    if ( actual->format != AUDIO_S16MSB)
#endif
    {
        fprintf(stderr, "Warning: incorrect audio format\n");
    }
    PROP(rate_in_s)=((double)((actual->format&0xFF)/8)*actual->channels*actual->freq);
    PROP(stereo)=((actual->channels-1) > 0);
}

#ifdef THREADED_AUDIO
void
METH(StartDecoding) (_THIS)
{
    PROP(decoding) = true;
    /* Create the ring buffer to hold audio */
    if ( ! PROP(ring) ) {
//        ring = new MPEG_ring(samplesperframe*2);
        PROP(ring) = MPEG_ring_init(NULL, PROP(samplesperframe) * 2, 16);
    }
    if ( ! PROP(decode_thread) ) {
        PROP(decode_thread) = SDL_CreateThread(Decode_MPEGaudio, self);
    }
}
void
METH(StopDecoding) (_THIS)
{
    PROP(decoding) = false;
    if ( PROP(decode_thread) ) {
        if( PROP(ring) ) MPEG_ring_ReleaseThreads(PROP(ring));
        SDL_WaitThread(PROP(decode_thread), NULL);
        PROP(decode_thread) = NULL;
    }
    if ( PROP(ring) ) {
        MPEG_ring_destroy(PROP(ring));
        free(PROP(ring));
        PROP(ring) = NULL;
    }
}
#endif

/* MPEG actions */
double
METH(Time) (_THIS)
{
    double now;

    if ( PROP(frag_time) ) {
        now = (PROP(play_time) + (double)(SDL_GetTicks() - PROP(frag_time))/1000.0);
    } else {
        now = PROP(play_time);
    }
    return now;
}
void
METH(Play) (_THIS)
{
    METH(ResetPause)(self);
    if ( PROP(valid_stream) ) {
#ifdef THREADED_AUDIO
        METH(StartDecoding)(self);
#endif
        PROP(playing) = true;
    }
}
void
METH(Stop) (_THIS)
{
    if ( PROP(valid_stream) ) {
        if ( PROP(sdl_audio) )
            SDL_LockAudio();

        PROP(playing) = false;

        if ( PROP(sdl_audio) )
            SDL_UnlockAudio();
    }
    METH(ResetPause)(self);
}
void
METH(Rewind) (_THIS)
{
    METH(Stop)(self);

#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    METH(StopDecoding)(self);
#endif

    METH(clearrawdata)(self);
    PROP(decodedframe) = 0;
    PROP(currentframe) = 0;
    PROP(frags_playing) = 0;
}
void
METH(ResetSynchro) (_THIS, double time)
{
    int i;

    PROP(play_time) = time;
    PROP(frag_time) = 0;

    /* Reinit the timestamp FIFO */
    for (i=0; i<N_TIMESTAMPS; i++)
      PROP(timestamp)[i] = -1;
}
void
METH(Skip) (_THIS, float seconds)
{
   /* Called only when there is no timestamp info in the MPEG */
   printf("Audio: Skipping %f seconds...\n", seconds);
   while(seconds > 0)
   {
     seconds -= (float) PROP(samplesperframe) / ((float) MPEGaudio_frequencies[PROP(version)][PROP(frequency)]*(1+PROP(inputstereo)));
     if(!METH(loadheader)(self)) break;
   }
 }
void
METH(Volume) (_THIS, int vol)
{
    if ( (vol >= 0) && (vol <= 100) ) {
        PROP(volume) = (vol*SDL_MIX_MAXVOLUME)/100;
    }
}
MPEGstatus
METH(GetStatus) (_THIS)
{
    if ( PROP(valid_stream) ) {
        /* Has decoding stopped because of end of stream? */
        if ( MPEGstream_eof(PROP(mpeg)) && (PROP(decodedframe) <= PROP(currentframe)) ) {
            return(MPEG_STOPPED);
        }
        /* Have we been told to play? */
        if ( PROP(playing) ) {
            return(MPEG_PLAYING);
        } else {
            return(MPEG_STOPPED);
        }
    } else {
        return(MPEG_ERROR);
    }
}

bool
METH(GetAudioInfo) (_THIS, MPEG_AudioInfo *info)
{
    if ( info ) {
      info->mpegversion = PROP(version);
      info->mode = PROP(mode);
      info->frequency = MPEGaudio_frequencies[PROP(version)][PROP(frequency)];
      info->layer = PROP(layer);
      info->bitrate = MPEGaudio_bitrate[PROP(version)][PROP(layer)-1][PROP(bitrateindex)];
      info->current_frame = PROP(currentframe);
    }
    return true;
}
bool
METH(fillbuffer) (_THIS, int size)
  {
      PROP(bitindex)=0;
      PROP(_buffer_pos) = PROP(mpeg)->pos;
      return (MPEGstream_copy_data(PROP(mpeg), PROP(_buffer), size, false) > 0);
  };
  
void
METH(sync) (_THIS)
{
  PROP(bitindex)=(PROP(bitindex)+7)&0xFFFFFFF8;
}
  
bool
METH(issync) (_THIS)
{
  return (PROP(bitindex)&7) != 0;
}
  
int 
METH(getbyte) (_THIS)
{
//  int r=(unsigned char)_buffer[bitindex>>3];
  int r;

  r = (unsigned char)(PROP(_buffer)[PROP(bitindex)>>3]);

  PROP(bitindex)+=8;
  return r;
}
  
int 
METH(getbit) (_THIS)
{
  register int r=(PROP(_buffer)[PROP(bitindex)>>3]>>(7-(PROP(bitindex)&7)))&1;

  PROP(bitindex)++;
  return r;
}
  
int 
METH(getbits8) (_THIS)
{
  register unsigned short a;
  { int offset=PROP(bitindex)>>3;

//  a=(((unsigned char)_buffer[offset])<<8) | ((unsigned char)_buffer[offset+1]);
  a=(((unsigned char)(PROP(_buffer)[offset]))<<8) | ((unsigned char)(PROP(_buffer)[offset+1]));
  }
  a<<=(PROP(bitindex)&7);
  PROP(bitindex)+=8;
  return (int)((unsigned int)(a>>8));
}
  
int 
METH(getbits9) (_THIS, int bits)
{
  register unsigned short a;
  { int offset=PROP(bitindex)>>3;

  a=(((unsigned char)(PROP(_buffer)[offset]))<<8) | ((unsigned char)(PROP(_buffer)[offset+1]));
  }
  a<<=(PROP(bitindex)&7);
  PROP(bitindex)+=bits;
  return (int)((unsigned int)(a>>(16-bits)));
}




/* other virtual methods of MPEGaction. */

void
METH(ResetPause) (_THIS)
{
  PROP(paused) = false;
}

void
METH(Pause) (_THIS)
{
  if (PROP(paused))
    {
      PROP(paused) = false;
      METH(Play)(self);
    }
  else
    {
      METH(Stop)(self);
      PROP(paused) = true;
    }
} 

