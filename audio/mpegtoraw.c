/* MPEG/WAVE Sound library

   (C) 1997 by Jung woo-jae */

// Mpegtoraw.cc
// Server which get mpeg format and put raw format.

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "MPEGaudio.h"
#include "MPEGstream.h"
#if defined(_WIN32)
#include <windows.h>
#endif

#define MY_PI 3.14159265358979323846

#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif


#undef _THIS
#define _THIS MPEGaudio *self

#define hcos_64 MPEGaudio_hcos_64
#define hcos_32 MPEGaudio_hcos_32
#define hcos_16 MPEGaudio_hcos_16
#define hcos_8 MPEGaudio_hcos_8
#define hcos_4 MPEGaudio_hcos_4


int
MPEGaudio_getbits( _THIS, int bits )
{
    union
    {
        char store[4];
        int current;
    } u;
    int bi;

    if( ! bits )
        return 0;

    u.current = 0;
    bi = (self->bitindex & 7);
    u.store[ _KEY ] = self->_buffer[ self->bitindex >> 3 ] << bi;
    bi = 8 - bi;
    self->bitindex += bi;

    while( bits )
    {
        if( ! bi )
        {
            u.store[ _KEY ] = self->_buffer[ self->bitindex >> 3 ];
            self->bitindex += 8;
            bi = 8;
        }

        if( bits >= bi )
        {
            u.current <<= bi;
            bits -= bi;
            bi = 0;
        }
        else
        {
            u.current <<= bits;
            bi -= bits;
            bits = 0;
        }
    }
    self->bitindex -= bi;

    return( u.current >> 8 );
}


// Convert mpeg to raw
// Mpeg headder class
void
MPEGaudio_initialize (_THIS)
{
  static bool initialized = false;

  register int i;
  register REAL *s1,*s2;
  REAL *s3,*s4;

  self->last_speed = 0;
  self->stereo = true;
  self->forcetomonoflag = false;
  self->forcetostereoflag = false;
  self->downfrequency = 0;

  self->scalefactor=SCALE;
  self->calcbufferoffset=15;
  self->currentcalcbuffer=0;

  s1 = self->calcbufferL[0];
  s2 = self->calcbufferR[0];
  s3 = self->calcbufferL[1];
  s4 = self->calcbufferR[1];
  for(i=CALCBUFFERSIZE-1;i>=0;i--)
  {
    self->calcbufferL[0][i]=self->calcbufferL[1][i]=
    self->calcbufferR[0][i]=self->calcbufferR[1][i]=0.0;
  }

  if( ! initialized )
  {
    for(i=0;i<16;i++) hcos_64[i] = (float)
			(1.0/(2.0*cos(MY_PI*(double)(i*2+1)/64.0)));
    for(i=0;i< 8;i++) hcos_32[i] = (float)
			(1.0/(2.0*cos(MY_PI*(double)(i*2+1)/32.0)));
    for(i=0;i< 4;i++) hcos_16[i] = (float)
			(1.0/(2.0*cos(MY_PI*(double)(i*2+1)/16.0)));
    for(i=0;i< 2;i++) hcos_8 [i] = (float)
			(1.0/(2.0*cos(MY_PI*(double)(i*2+1)/ 8.0)));
    hcos_4 = (float)(1.0f / (2.0f * cos( MY_PI * 1.0 / 4.0 )));
    initialized = true;
  }

  MPEGaudio_layer3initialize(self);

#ifdef THREADED_AUDIO
  self->decode_thread = NULL;
  self->ring = NULL;
#endif
  MPEGaudio_Rewind(self);
  MPEGaudio_ResetSynchro(self, 0);
};


bool
MPEGaudio_loadheader (_THIS)
{
    register int c;
    bool flag;
    int speed;

    flag = false;
    do
    {
        if( (c = MPEGstream_copy_byte(self->mpeg)) < 0 )
            break;

        if( c == 0xff )
        {
            while( ! flag )
            {
                if( (c = MPEGstream_copy_byte(self->mpeg)) < 0 )
                {
                    flag = true;
                    break;
                }
                if( (c & 0xf0) == 0xf0 )
                {
                    flag = true;
                    break;
                }
                else if( c != 0xff )
                {
                    break;
                }
            }
        }
    } while( ! flag );

    if( c < 0 )
        return false;


    // Analyzing

    c &= 0xf;
    self->protection = c & 1;
    self->layer = 4 - ((c >> 1) & 3);
    self->version = (MPEGaudio_mpegversion) ((c >> 3) ^ 1);

    c = MPEGstream_copy_byte(self->mpeg) >> 1;
    self->padding = (c & 1);
    c >>= 1;
    self->frequency = (MPEGaudio_frequency) (c&3);
    if (self->frequency == 3)
        return false;
    c >>= 2;
    self->bitrateindex = (int) c;
    if( self->bitrateindex == 15 )
        return false;

    c = ((unsigned int)MPEGstream_copy_byte(self->mpeg)) >> 4;
    self->extendedmode = c & 3;
    self->mode = (MPEGaudio_mode) (c >> 2);


    // Making information

    self->inputstereo = (self->mode == single) ? 0 : 1;

    self->forcetomonoflag = (!self->stereo && self->inputstereo);
    self->forcetostereoflag = (self->stereo && !self->inputstereo);

    if(self->forcetomonoflag)
        self->outputstereo=0;
    else
        self->outputstereo=self->inputstereo;

    self->channelbitrate=self->bitrateindex;
    if(self->inputstereo)
    {
        if(self->channelbitrate==4)
            self->channelbitrate=1;
        else
            self->channelbitrate-=4;
    }

    if(self->channelbitrate==1 || self->channelbitrate==2)
        self->tableindex=0;
    else
        self->tableindex=1;

  if(self->layer==1)
      self->subbandnumber=MAXSUBBAND;
  else
  {
    if(!self->tableindex)
      if(self->frequency==frequency32000)self->subbandnumber=12; else self->subbandnumber=8;
    else if(self->frequency==frequency48000||
        (self->channelbitrate>=3 && self->channelbitrate<=5))
      self->subbandnumber=27;
    else self->subbandnumber=30;
  }

  if(self->mode==single)self->stereobound=0;
  else if(self->mode==joint)self->stereobound=(self->extendedmode+1)<<2;
  else self->stereobound=self->subbandnumber;

  if(self->stereobound>self->subbandnumber)self->stereobound=self->subbandnumber;

  // framesize & slots
  if(self->layer==1)
  {
    self->framesize=(12000*MPEGaudio_bitrate[self->version][0][self->bitrateindex])/
              MPEGaudio_frequencies[self->version][self->frequency];
    if(self->frequency==frequency44100 && self->padding)self->framesize++;
    self->framesize<<=2;
  }
  else
  {
    self->framesize=(144000*MPEGaudio_bitrate[self->version][self->layer-1][self->bitrateindex])/
      (MPEGaudio_frequencies[self->version][self->frequency]<<self->version);
    if(self->padding)self->framesize++;
    if(self->layer==3)
    {
      if(self->version)
    self->layer3slots=self->framesize-((self->mode==single)?9:17)
                         -(self->protection?0:2)
                         -4;
      else
    self->layer3slots=self->framesize-((self->mode==single)?17:32)
                         -(self->protection?0:2)
                         -4;
    }
  }

#ifdef DEBUG_AUDIO
  fprintf(stderr, "MPEG %d audio layer %d (%d kbps), at %d Hz %s [%d]\n", self->version+1, self->layer,  MPEGaudio_bitrate[self->version][self->layer-1][self->bitrateindex], MPEGaudio_frequencies[self->version][self->frequency], (self->mode == single) ? "mono" : "stereo", self->framesize);
#endif

  /* Fill the buffer with new data */
  if(!MPEGaudio_fillbuffer(self, self->framesize-4))
    return false;

  if(!self->protection)
  {
    MPEGaudio_getbyte(self);                      // CRC, Not check!!
    MPEGaudio_getbyte(self);
  }

  // Sam 7/17 - skip sequences of quickly varying frequencies
  speed = MPEGaudio_frequencies[self->version][self->frequency];
  if ( speed != self->last_speed ) {
    self->last_speed = speed;
    if ( self->rawdatawriteoffset ) {
        ++self->decodedframe;
#ifndef THREADED_AUDIO
        ++self->currentframe;
#endif
    }
    return MPEGaudio_loadheader(self);
  }

  return true;
}


bool
MPEGaudio_run ( _THIS, int frames, double *timestamp)
{
    double last_timestamp = -1;
    int totFrames = frames;

    for( ; frames; frames-- )
    {
        if( MPEGaudio_loadheader(self) == false ) {
	  return false;	  
        }

        if (frames == totFrames  && timestamp != NULL) {
            if (last_timestamp != self->mpeg->timestamp){
		if (self->mpeg->timestamp_pos <= self->_buffer_pos)
		    last_timestamp = *timestamp = self->mpeg->timestamp;
	    }
            else
                *timestamp = -1;
        }

        if     ( self->layer == 3 ) MPEGaudio_extractlayer3(self);
        else if( self->layer == 2 ) MPEGaudio_extractlayer2(self);
        else if( self->layer == 1 ) MPEGaudio_extractlayer1(self);

        /* Handle expanding to stereo output */
        if ( self->forcetostereoflag ) {
            Sint16 *in, *out;

            in = self->rawdata+self->rawdatawriteoffset;
            self->rawdatawriteoffset *= 2;
            out = self->rawdata+self->rawdatawriteoffset;
            while ( in > self->rawdata ) {
                --in;
                *(--out) = *in;
                *(--out) = *in;
            }
        }

        // Sam 10/5 - If there is no data, don't increment frames
        if ( self->rawdatawriteoffset ) {
            ++self->decodedframe;
#ifndef THREADED_AUDIO
            ++self->currentframe;
#endif
        }
    }

    return(true);
}

#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata)
{
    MPEGaudio *audio = (MPEGaudio *)udata;
    double timestamp;

#if defined(_WIN32)
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);
#endif

    while ( audio->decoding && ! MPEGstream_eof(audio->mpeg) ) {
        audio->rawdata = (Sint16 *)MPEG_ring_NextWriteBuffer(audio->ring);

        if ( audio->rawdata ) {
            audio->rawdatawriteoffset = 0;
            /* Sam 10/5/2000 - Added while to prevent empty buffer in ring */
            while ( MPEGaudio_run(audio, 1, &timestamp) &&
                    (audio->rawdatawriteoffset == 0) ) {
                /* Keep looping */ ;
            }
            if((Uint32)audio->rawdatawriteoffset*2 <= MPEG_ring_BufferSize(audio->ring))
              MPEG_ring_WriteDone(audio->ring, audio->rawdatawriteoffset*2, timestamp);
        }
    }

    audio->decoding = false;
/* Patch to allow audio to loop infinitely */
//    audio->decode_thread = NULL;
    return(0);
}
#endif /* THREADED_AUDIO */

// Helper function for SDL audio
int Play_MPEGaudio(MPEGaudio *audio, Uint8 *stream, int len)
{
    int volume;
    long copylen;
    int mixed = 0;

    /* Bail if audio isn't playing */
    if ( MPEGaudio_GetStatus(audio) != MPEG_PLAYING ) {
        return(0);
    }
    volume = audio->volume;

    /* Increment the current play time (assuming fixed frag size) */
    switch (audio->frags_playing++) {
      // Vivien: Well... the theorical way seems good to me :-)
        case 0:        /* The first audio buffer is being filled */
            break;
        case 1:        /* The first audio buffer is starting playback */
            audio->frag_time = SDL_GetTicks();
            break;
        default:    /* A buffer has completed, filling a new one */
            audio->frag_time = SDL_GetTicks();
            audio->play_time += ((double)len)/audio->rate_in_s;
            break;
    }

    /* Copy the audio data to output */
#ifdef THREADED_AUDIO
{
    Uint8 *rbuf;
    int i;
    assert(audio);
    assert(audio->ring);
    do {
	/* this is empirical, I don't realy know how to find out when
	   a certain piece of audio has finished playing or even if
	   the timestamps refer to the time when the frame starts
	   playing or then the frame ends playing, but as is works
	   quite right */
        copylen = MPEG_ring_NextReadBuffer(audio->ring, &rbuf);
        if ( copylen > len ) {
            SDL_MixAudio(stream, rbuf, len, volume);
            mixed += len;
            MPEG_ring_ReadSome(audio->ring, len);
            len = 0;
	    for (i=0; i < N_TIMESTAMPS -1; i++)
		audio->timestamp[i] = audio->timestamp[i+1];
	    audio->timestamp[N_TIMESTAMPS-1] = MPEG_ring_ReadTimeStamp(audio->ring);
        } else {
            SDL_MixAudio(stream, rbuf, copylen, volume);
            mixed += copylen;
            ++audio->currentframe;
            MPEG_ring_ReadDone(audio->ring);
//fprintf(stderr, "-");
            len -= copylen;
            stream += copylen;
        }
	if (audio->timestamp[0] != -1){
	    double timeshift = MPEGaudio_Time(audio) - audio->timestamp[0];
	    double correction = 0;
	    assert(audio->timestamp >= 0);
	    if (fabs(timeshift) > 1.0){
	        correction = -timeshift;
#ifdef DEBUG_TIMESTAMP_SYNC
                fprintf(stderr, "audio jump %f\n", timeshift);
#endif
            } else
	        correction = -timeshift/100;
#ifdef USE_TIMESTAMP_SYNC
	    audio->play_time += correction;
#endif
#ifdef DEBUG_TIMESTAMP_SYNC
	    fprintf(stderr, "\raudio: time:%8.3f shift:%8.4f",
                    MPEGaudio_Time(audio), timeshift);
#endif
	    audio->timestamp[0] = -1;
	}
    } while ( copylen && (len > 0) && ((audio->currentframe < audio->decodedframe) || audio->decoding));
}
#else
    /* The length is interpreted as being in samples */
    len /= 2;

    /* Copy in any saved data */
    if ( audio->rawdatawriteoffset >= audio->rawdatareadoffset) {
        copylen = (audio->rawdatawriteoffset-audio->rawdatareadoffset);
        assert(copylen >= 0);
        if ( copylen >= len ) {
            SDL_MixAudio(stream, (Uint8 *)&audio->spillover[audio->rawdatareadoffset], len*2, volume);
            mixed += len*2;
            audio->rawdatareadoffset += len;
            goto finished_mixing;
        }
        SDL_MixAudio(stream, (Uint8 *)&audio->spillover[audio->rawdatareadoffset], copylen*2, volume);
        mixed += copylen*2;
        len -= copylen;
        stream += copylen*2;
    }

    /* Copy in any new data */
    audio->rawdata = (Sint16 *)stream;
    audio->rawdatawriteoffset = 0;
    MPEGaudio_run(audio, len/audio->samplesperframe, NULL);
    mixed += audio->rawdatawriteoffset*2;
    len -= audio->rawdatawriteoffset;
    stream += audio->rawdatawriteoffset*2;

    /* Write a save buffer for remainder */
    audio->rawdata = audio->spillover;
    audio->rawdatawriteoffset = 0;
    if ( MPEGaudio_run(audio, 1, NULL) ) {
        assert(audio->rawdatawriteoffset > len);
        SDL_MixAudio(stream, (Uint8 *) audio->spillover, len*2, volume);
        mixed += len*2;
        audio->rawdatareadoffset = len;
    } else {
        audio->rawdatareadoffset = 0;
    }
finished_mixing:
#endif
    return(mixed);
}
void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len)
{
    MPEGaudio *audio = (MPEGaudio *)udata;
    Play_MPEGaudio(audio, stream, len);
}

// EOF
