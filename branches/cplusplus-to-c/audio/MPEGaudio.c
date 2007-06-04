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

MPEGaudio *
MPEGaudio_init (_THIS, MPEGstream *stream, bool initSDL)
{
    int i;

    MAKE_OBJECT(MPEGaudio);
    self->sdl_audio = initSDL;
    self->error = MPEGerror_init(self->error);
//    self->action = MPEGaction_init(self->action);
    self->playing = false;
    self->paused = false;
    self->looping = false;
    self->play_time = 0.0;

    /* Initialize MPEG audio */
    self->mpeg = stream;
    MPEGaudio_initialize (self);

    /* Just be paranoid.  If all goes well, this will be set to true */
    self->valid_stream = false;

    /* Analyze the MPEG audio stream */
    if ( MPEGaudio_loadheader(self) ) {
        SDL_AudioSpec wanted;
        MPEGaudio_WantedSpec(self, &wanted);

        /* Calculate the samples per frame */
        self->samplesperframe = 32*wanted.channels;
        if( self->layer == 3 ) {
            self->samplesperframe *= 18;
            if ( self->version == 0 ) {
                self->samplesperframe *= 2;
            }
        } else {
            self->samplesperframe *= SCALEBLOCK;
            if ( self->layer == 2 ) {
                self->samplesperframe *= 3;
            }
        }
        if ( self->sdl_audio ) {
            /* Open the audio, get actual audio hardware format and convert */
            bool audio_active;
            SDL_AudioSpec actual;
            audio_active = (SDL_OpenAudio(&wanted, &actual) == 0);
            if ( audio_active ) {
                MPEGaudio_ActualSpec(self, &actual);
                self->valid_stream = true;
            } else {
                MPEGerror_SetError(self->error, SDL_GetError());
            }
            SDL_PauseAudio(0);
        } else { /* The stream is always valid if we don't initialize SDL */
            self->valid_stream = true; 
        }
        MPEGaudio_Volume(self, 100);
    }

    /* For using system timestamp */
    for (i=0; i<N_TIMESTAMPS; i++)
      self->timestamp[i] = -1;

  return self;
}

void
MPEGaudio_destroy (_THIS)
{
#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    MPEGaudio_StopDecoding(self);
#endif

    /* Remove ourselves from the mixer hooks */
    MPEGaudio_Stop(self);
    if ( self->sdl_audio ) {
        /* Close up the audio so others may play */
        SDL_CloseAudio();
    }

//  MPEGaction_destroy(self->action);
//  free(self->action);
//  self->action = NULL;
  self->playing = false;
  self->paused = false;
  self->looping = false;
  self->play_time = 0.0;

  MPEGerror_destroy(self->error);
  free(self->error);
  self->error = NULL;

}

bool
MPEGaudio_WantedSpec (_THIS, SDL_AudioSpec *wanted)
{
    wanted->freq = MPEGaudio_frequencies[self->version][self->frequency];
#if SDL_BYTEORDER == SDL_LIL_ENDIAN
    wanted->format = AUDIO_S16LSB;
#else
    wanted->format = AUDIO_S16MSB;
#endif
    if ( self->outputstereo ) {
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
MPEGaudio_ActualSpec (_THIS, const SDL_AudioSpec *actual)
{
    /* Splay can optimize some of the conversion */
    if ( actual->channels == 1 && self->outputstereo ) {
        self->forcetomonoflag = true;
    }
    if ( actual->channels == 2 && !self->outputstereo ) {
        self->forcetostereoflag = true;
        self->samplesperframe *= 2;
    }
    /* FIXME: Create an audio conversion block */
    if ( (actual->freq/100) == ((MPEGaudio_frequencies[self->version][self->frequency]/2)/100) ) {
        self->downfrequency = 1;
    } else if ( actual->freq != MPEGaudio_frequencies[self->version][self->frequency] ) {
#ifdef VERBOSE_WARNINGS
        fprintf(stderr, "Warning: wrong audio frequency (wanted %d, got %d)\n",
		MPEGaudio_frequencies[self->version][self->frequency], actual->freq);
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
    self->rate_in_s=((double)((actual->format&0xFF)/8)*actual->channels*actual->freq);
    self->stereo=((actual->channels-1) > 0);
}

#ifdef THREADED_AUDIO
void
MPEGaudio_StartDecoding (_THIS)
{
    self->decoding = true;
    /* Create the ring buffer to hold audio */
    if ( ! self->ring ) {
//        ring = new MPEG_ring(samplesperframe*2);
        self->ring = MPEG_ring_init(NULL, self->samplesperframe * 2, 16);
    }
    if ( ! self->decode_thread ) {
        self->decode_thread = SDL_CreateThread(Decode_MPEGaudio, self);
    }
}
void
MPEGaudio_StopDecoding (_THIS)
{
    self->decoding = false;
    if ( self->decode_thread ) {
        if( self->ring ) MPEG_ring_ReleaseThreads(self->ring);
        SDL_WaitThread(self->decode_thread, NULL);
        self->decode_thread = NULL;
    }
    if ( self->ring ) {
        MPEG_ring_destroy(self->ring);
        free(self->ring);
        self->ring = NULL;
    }
}
#endif

/* MPEG actions */
double
MPEGaudio_Time (_THIS)
{
    double now;

    if ( self->frag_time ) {
        now = (self->play_time + (double)(SDL_GetTicks() - self->frag_time)/1000.0);
    } else {
        now = self->play_time;
    }
    return now;
}
void
MPEGaudio_Play (_THIS)
{
    MPEGaudio_ResetPause(self);
    if ( self->valid_stream ) {
#ifdef THREADED_AUDIO
        MPEGaudio_StartDecoding(self);
#endif
        self->playing = true;
    }
}
void
MPEGaudio_Stop (_THIS)
{
    if ( self->valid_stream ) {
        if ( self->sdl_audio )
            SDL_LockAudio();

        self->playing = false;

        if ( self->sdl_audio )
            SDL_UnlockAudio();
    }
    MPEGaudio_ResetPause(self);
}
void
MPEGaudio_Rewind (_THIS)
{
    MPEGaudio_Stop(self);

#ifdef THREADED_AUDIO
    /* Stop the decode thread */
    MPEGaudio_StopDecoding(self);
#endif

    MPEGaudio_clearrawdata(self);
    self->decodedframe = 0;
    self->currentframe = 0;
    self->frags_playing = 0;
}
void
MPEGaudio_ResetSynchro (_THIS, double time)
{
    int i;

    self->play_time = time;
    self->frag_time = 0;

    /* Reinit the timestamp FIFO */
    for (i=0; i<N_TIMESTAMPS; i++)
      self->timestamp[i] = -1;
}
void
MPEGaudio_Skip (_THIS, float seconds)
{
   /* Called only when there is no timestamp info in the MPEG */
   printf("Audio: Skipping %f seconds...\n", seconds);
   while(seconds > 0)
   {
     seconds -= (float) self->samplesperframe / ((float) MPEGaudio_frequencies[self->version][self->frequency]*(1+self->inputstereo));
     if(!MPEGaudio_loadheader(self)) break;
   }
 }
void
MPEGaudio_Volume (_THIS, int vol)
{
    if ( (vol >= 0) && (vol <= 100) ) {
        self->volume = (vol*SDL_MIX_MAXVOLUME)/100;
    }
}
MPEGstatus
MPEGaudio_GetStatus (_THIS)
{
    if ( self->valid_stream ) {
        /* Has decoding stopped because of end of stream? */
        if ( MPEGstream_eof(self->mpeg) && (self->decodedframe <= self->currentframe) ) {
            return(MPEG_STOPPED);
        }
        /* Have we been told to play? */
        if ( self->playing ) {
            return(MPEG_PLAYING);
        } else {
            return(MPEG_STOPPED);
        }
    } else {
        return(MPEG_ERROR);
    }
}

bool
MPEGaudio_GetAudioInfo (_THIS, MPEG_AudioInfo *info)
{
    if ( info ) {
      info->mpegversion = self->version;
      info->mode = self->mode;
      info->frequency = MPEGaudio_frequencies[self->version][self->frequency];
      info->layer = self->layer;
      info->bitrate = MPEGaudio_bitrate[self->version][self->layer-1][self->bitrateindex];
      info->current_frame = self->currentframe;
    }
    return true;
}
bool
MPEGaudio_fillbuffer (_THIS, int size)
  {
      self->bitindex=0;
      self->_buffer_pos = self->mpeg->pos;
      return (MPEGstream_copy_data(self->mpeg, self->_buffer, size, false) > 0);
  };
  
void
MPEGaudio_sync (_THIS)
{
  self->bitindex=(self->bitindex+7)&0xFFFFFFF8;
}
  
bool
MPEGaudio_issync (_THIS)
{
  return (self->bitindex&7) != 0;
}
  
int 
MPEGaudio_getbyte (_THIS)
{
//  int r=(unsigned char)_buffer[bitindex>>3];
  int r;

  r = (unsigned char)(self->_buffer[self->bitindex>>3]);

  self->bitindex+=8;
  return r;
}
  
int 
MPEGaudio_getbit (_THIS)
{
  register int r=(self->_buffer[self->bitindex>>3]>>(7-(self->bitindex&7)))&1;

  self->bitindex++;
  return r;
}
  
int 
MPEGaudio_getbits8 (_THIS)
{
  register unsigned short a;
  { int offset=self->bitindex>>3;

//  a=(((unsigned char)_buffer[offset])<<8) | ((unsigned char)_buffer[offset+1]);
  a=(((unsigned char)(self->_buffer[offset]))<<8) | ((unsigned char)(self->_buffer[offset+1]));
  }
  a<<=(self->bitindex&7);
  self->bitindex+=8;
  return (int)((unsigned int)(a>>8));
}
  
int 
MPEGaudio_getbits9 (_THIS, int bits)
{
  register unsigned short a;
  { int offset=self->bitindex>>3;

  a=(((unsigned char)(self->_buffer[offset]))<<8) | ((unsigned char)(self->_buffer[offset+1]));
  }
  a<<=(self->bitindex&7);
  self->bitindex+=bits;
  return (int)((unsigned int)(a>>(16-bits)));
}




/* other virtual methods of MPEGaction. */

void
MPEGaudio_ResetPause (_THIS)
{
  self->paused = false;
}

void
MPEGaudio_Pause (_THIS)
{
  if (self->paused)
    {
      self->paused = false;
      MPEGaudio_Play(self);
    }
  else
    {
      MPEGaudio_Stop(self);
      self->paused = true;
    }
} 

