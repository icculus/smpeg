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
#include <unistd.h>
#include <assert.h>

#include "MPEGaudio.h"

#define MY_PI 3.14159265358979323846


#if SDL_BYTEORDER == SDL_LIL_ENDIAN
#define _KEY 0
#else
#define _KEY 3
#endif

int MPEGaudio::getbits( int bits )
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
    bi = (bitindex & 7);
    u.store[ _KEY ] = _buffer[ bitindex >> 3 ] << bi;
    bi = 8 - bi;
    bitindex += bi;

    while( bits )
    {
        if( ! bi )
        {
            u.store[ _KEY ] = _buffer[ bitindex >> 3 ];
            bitindex += 8;
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
    bitindex -= bi;

    return( u.current >> 8 );
}


// Convert mpeg to raw
// Mpeg headder class
void MPEGaudio::initialize()
{
  static bool initialized = false;

  register int i;
  register REAL *s1,*s2;
  REAL *s3,*s4;

  forcetomonoflag = false;
  forcetostereoflag = false;
  downfrequency = 0;

  scalefactor=SCALE;
  calcbufferoffset=15;
  currentcalcbuffer=0;

  s1 = calcbufferL[0];
  s2 = calcbufferR[0];
  s3 = calcbufferL[1];
  s4 = calcbufferR[1];
  for(i=CALCBUFFERSIZE-1;i>=0;i--)
  {
    calcbufferL[0][i]=calcbufferL[1][i]=
    calcbufferR[0][i]=calcbufferR[1][i]=0.0;
  }

  if( ! initialized )
  {
    for(i=0;i<16;i++) hcos_64[i] = 1.0/(2.0*cos(MY_PI*double(i*2+1)/64.0));
    for(i=0;i< 8;i++) hcos_32[i] = 1.0/(2.0*cos(MY_PI*double(i*2+1)/32.0));
    for(i=0;i< 4;i++) hcos_16[i] = 1.0/(2.0*cos(MY_PI*double(i*2+1)/16.0));
    for(i=0;i< 2;i++) hcos_8 [i] = 1.0/(2.0*cos(MY_PI*double(i*2+1)/ 8.0));
    hcos_4 = 1.0 / (2.0 * cos( MY_PI * 1.0 / 4.0 ));
    initialized = true;
  }

  layer3initialize();

#ifdef THREADED_AUDIO
  decode_thread = NULL;
  ring = NULL;
#endif
  Rewind();
};


bool MPEGaudio::loadheader()
{
    register int c;
    bool flag;

    flag = false;
    do
    {
        if( (c = mpeg->copy_byte()) < 0 )
            break;

        if( c == 0xff )
        {
            while( ! flag )
            {
                if( (c = mpeg->copy_byte()) < 0 )
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
    protection = c & 1;
    layer = 4 - ((c >> 1) & 3);
    version = (_mpegversion) ((c >> 3) ^ 1);

    c = mpeg->copy_byte() >> 1;
    padding = (c & 1);
    c >>= 1;
    frequency = (_frequency) (c&2);
    c >>= 2;
    bitrateindex = (int) c;
    if( bitrateindex == 15 )
        return false;

    c = ((unsigned int)mpeg->copy_byte()) >> 4;
    extendedmode = c & 3;
    mode = (_mode) (c >> 2);


    // Making information

    inputstereo = (mode == single) ? 0 : 1;
    if(forcetomonoflag)
        outputstereo=0;
    else
        outputstereo=inputstereo;

    channelbitrate=bitrateindex;
    if(inputstereo)
    {
        if(channelbitrate==4)
            channelbitrate=1;
        else
            channelbitrate-=4;
    }

    if(channelbitrate==1 || channelbitrate==2)
        tableindex=0;
    else
        tableindex=1;

  if(layer==1)
      subbandnumber=MAXSUBBAND;
  else
  {
    if(!tableindex)
      if(frequency==frequency32000)subbandnumber=12; else subbandnumber=8;
    else if(frequency==frequency48000||
        (channelbitrate>=3 && channelbitrate<=5))
      subbandnumber=27;
    else subbandnumber=30;
  }

  if(mode==single)stereobound=0;
  else if(mode==joint)stereobound=(extendedmode+1)<<2;
  else stereobound=subbandnumber;

  if(stereobound>subbandnumber)stereobound=subbandnumber;

  // framesize & slots
  if(layer==1)
  {
    framesize=(12000*bitrate[version][0][bitrateindex])/
              frequencies[version][frequency];
    if(frequency==frequency44100 && padding)framesize++;
    framesize<<=2;
  }
  else
  {
    framesize=(144000*bitrate[version][layer-1][bitrateindex])/
      (frequencies[version][frequency]<<version);
    if(padding)framesize++;
    if(layer==3)
    {
      if(version)
    layer3slots=framesize-((mode==single)?9:17)
                         -(protection?0:2)
                         -4;
      else
    layer3slots=framesize-((mode==single)?17:32)
                         -(protection?0:2)
                         -4;
    }
  }

#ifdef DEBUG_AUDIO
  static int printed = 0;
  if ( ! printed ) {
    printf("MPEG audio stream layer %d, at %d Hz %s\n", layer, frequencies[version][frequency], (mode == single) ? "mono" : "stereo");
    printed = 1;
  }
#endif

  /* Fill the buffer with new data */
  if(!fillbuffer(framesize-4))
    return false;

  if(!protection)
  {
    getbyte();                      // CRC, Not check!!
    getbyte();
  }
  return true;
}


bool MPEGaudio::run( int frames )
{
    for( ; frames; frames-- )
    {
        if( loadheader() == false ) {
            if ( looping ) {
                mpeg->reset_stream();
                if ( loadheader() == false ) {
                    return(false);
                }
            } else {
                return false;
            }
        }

        if     ( layer == 3 ) extractlayer3();
        else if( layer == 2 ) extractlayer2();
        else if( layer == 1 ) extractlayer1();

        /* Handle expanding to stereo output */
        if ( forcetostereoflag ) {
            Sint16 *in, *out;

            in = rawdata+rawdatawriteoffset;
            rawdatawriteoffset *= 2;
            out = rawdata+rawdatawriteoffset;
            while ( in > rawdata ) {
                --in;
                *(--out) = *in;
                *(--out) = *in;
            }
        }

        ++decodedframe;
#ifndef THREADED_AUDIO
        ++currentframe;
#endif
    }

    return(true);
}

#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata)
{
    MPEGaudio *audio = (MPEGaudio *)udata;

    while ( audio->decoding && ! audio->mpeg->eof() ) {
        audio->rawdata = (Sint16 *)audio->ring->NextWriteBuffer();
        if ( audio->rawdata ) {
            audio->rawdatawriteoffset = 0;
            audio->run(1);
            audio->ring->WriteDone(audio->rawdatawriteoffset*2);
        } else {
            SDL_Delay(100);  /* The write buffer is full, wait a bit */
        }
    }
    return(0);
}
#endif /* THREADED_AUDIO */

// Helper function for SDL audio
void Play_MPEGaudio(void *udata, Uint8 *stream, int len)
{
    MPEGaudio *audio = (MPEGaudio *)udata;
    int volume;
    long copylen;

    /* Bail if audio isn't playing */
    if ( audio->Status() != MPEG_PLAYING ) {
        return;
    }
    volume = audio->volume;

    /* Increment the current play time (assuming fixed frag size) */
    switch (audio->frags_playing++) {
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
    Uint8 *rbuf;
    assert(audio);
    assert(audio->ring);
    do {
        copylen = audio->ring->NextReadBuffer(&rbuf);
        if ( copylen > len ) {
            SDL_MixAudio(stream, rbuf, len, volume);
            audio->ring->ReadSome(len);
            len = 0;
        } else {
            SDL_MixAudio(stream, rbuf, copylen, volume);
            ++audio->currentframe;
            audio->ring->ReadDone();
//fprintf(stderr, "-");
            len -= copylen;
            stream += copylen;
        }
    } while ( copylen && (len > 0) );
#else
    /* The length is interpreted as being in samples */
    len /= 2;

    /* Copy in any saved data */
    if ( audio->rawdatawriteoffset > 0 ) {
        copylen = (audio->rawdatawriteoffset-audio->rawdatareadoffset);
        assert(copylen >= 0);
        if ( copylen >= len ) {
            SDL_MixAudio(stream, &audio->spillover[audio->rawdatareadoffset],
                                                       len*2, volume);
            audio->rawdatareadoffset += len;
            return;
        }
        SDL_MixAudio(stream, &audio->spillover[audio->rawdatareadoffset],
                                                       copylen*2, volume);
        len -= copylen;
        stream += copylen*2;
    }

    /* Copy in any new data */
    audio->rawdata = (Sint16 *)stream;
    audio->rawdatawriteoffset = 0;
    audio->run(len/audio->samplesperframe);
    len -= audio->rawdatawriteoffset;
    stream += audio->rawdatawriteoffset*2;

    /* Write a save buffer for remainder */
    audio->rawdata = audio->spillover;
    audio->rawdatawriteoffset = 0;
    if ( audio->run(1) ) {
        assert(audio->rawdatawriteoffset > len);
        SDL_MixAudio(stream, audio->spillover, len*2, volume);
        audio->rawdatareadoffset = len;
    } else {
        audio->rawdatareadoffset = 0;
    }
#endif
}

// EOF
