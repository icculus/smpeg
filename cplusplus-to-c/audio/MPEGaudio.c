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

#include "MPEGaudio.h"
#include "MPEGstream.h"

#undef _THIS
#define _THIS MPEGaudio *self
#undef METH /* method */
#define METH(name) MPEGaudio_##name
#undef PROP /* property */
#define PROP(name) (self->name)

MPEGaudio *
METH(init) (_THIS, MPEGstream *stream, bool initSDL)
{
    int i;
    self->sdl_audio = initSDL;

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
                METH(SetError)(self->error, SDL_GetError());
            }
            SDL_PauseAudio(0);
        } else { /* The stream is always valid if we don't initialize SDL */
            PROP(valid_stream) = true; 
        }
//        METH(Volume)(self->audioaction, 100);
        MPEGaudioaction_Volume(self->audioaction, 100);
    }

    /* For using system timestamp */
    for (i=0; i<N_TIMESTAMPS; i++)
      PROP(timestamp[i]) = -1;
}

void
METH(destroy) (_THIS)
{
#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    METH(StopDecoding)(self);
#endif

    /* Remove ourselves from the mixer hooks */
//    METH(Stop)(_THIS);
    self->audioaction->Stop(self->audioaction);
    if ( PROP(sdl_audio) ) {
        /* Close up the audio so others may play */
        SDL_CloseAudio();
    }
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
//    wanted->userdata = this;
    wanted->userdata = (void*)self;
    return true;
}

void
METH(ActualSpec)(_THIS, const SDL_AudioSpec *actual)
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
        PROP(ring) = MPEG_ring_new(PROP(samplesperframe)*2);
    }
    if ( ! PROP(decode_thread) ) {
        PROP(decode_thread) = SDL_CreateThread(Decode_MPEGaudio, this);
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
    double play_time;

    play_time = self->audioaction->play_time;
    if ( PROP(frag_time) ) {
        now = (play_time + (double)(SDL_GetTicks() - PROP(frag_time))/1000.0);
    } else {
        now = play_time;
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
//        playing = true;
        PROP(audioaction->playing) = true;
    }
}
void
METH(Stop) (_THIS)
{
    if ( PROP(valid_stream) ) {
        if ( PROP(sdl_audio) )
            SDL_LockAudio();

//        playing = false;
        PROP(audioaction->playing) = false;

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
METH(ResetSynchro)(_THIS, double time)
{
    int i;
    PROP(audioaction->play_time) = time;
    PROP(frag_time) = 0;

    /* Reinit the timestamp FIFO */
    for (i=0; i<N_TIMESTAMPS; i++)
      PROP(timestamp[i]) = -1;
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
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  conflict name in popcorn */
MPEGstatus
METH(GetStatus) (_THIS)
{
    if ( PROP(valid_stream) ) {
        /* Has decoding stopped because of end of stream? */
//        if ( mpeg->eof() && (decodedframe <= currentframe) ) {
        if (MPEG_eof(self->mpeg) && (PROP(decodedframe) <= PROP(currentframe))) {
            return(MPEG_STOPPED);
        }
        /* Have we been told to play? */
        if ( PROP(audioaction->playing) ) {
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
      PROP(bitwindow.bitindex)=0;
      PROP(_buffer_pos) = PROP(mpeg->pos);
//      return(mpeg->copy_data(_buffer, size) > 0);
      return (MPEG_copy_data(self->mpeg, PROP(_buffer), size) > 0);
  };
  
void
METH(sync) (_THIS)
{
  PROP(bitwindow.bitindex)=(PROP(bitwindow.bitindex)+7)&0xFFFFFFF8;
}
  
bool
METH(issync) (_THIS)
{
  return (PROP(bitwindow.bitindex)&7) != 0;
}
  
int 
METH(getbyte) (_THIS)
{
  int r=(unsigned char)PROP(_buffer[PROP(bitwindow.bitindex)>>3]);

  PROP(bitwindow.bitindex)+=8;
  return r;
}
  
int 
METH(getbit) (_THIS)
{
  register int r=(PROP(_buffer[PROP(bitwindow.bitindex)>>3])>>(7-(PROP(bitwindow.bitindex)&7)))&1;

  PROP(bitwindow.bitindex)++;
  return r;
}
  
int 
METH(getbits8) (_THIS)
{
  register unsigned short a;
  { int offset=PROP(bitwindow.bitindex)>>3;

  a=(((unsigned char)PROP(_buffer[offset]))<<8) | ((unsigned char)PROP(_buffer[offset+1]));
  }
  a<<=(PROP(bitwindow.bitindex)&7);
  PROP(bitwindow.bitindex)+=8;
  return (int)((unsigned int)(a>>8));
}
  
int 
METH(getbits9) (_THIS, int bits)
{
  register unsigned short a;
  { int offset=PROP(bitwindow.bitindex)>>3;

  a=(((unsigned char)PROP(_buffer[offset])<<8)) | ((unsigned char)PROP(_buffer[offset+1]));
  }
  a<<=(PROP(bitwindow.bitindex)&7);
  PROP(bitwindow.bitindex)+=bits;
  return (int)((unsigned int)(a>>(16-bits)));
}
