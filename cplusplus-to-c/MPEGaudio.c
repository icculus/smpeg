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

#include "SDL.h"
#include "MPEGerror.h"
#include "MPEGaction.h"
#include "MPEGaudio.h"







#undef _THIS
#define _THIS Mpegbitwindow *self
#undef METH
#define METH(name) Mpegbitwindow_##name

//  void wrap(void);
void METH(wrap) (_THIS)
{
}

int METH(getbits) (_THIS, int bits)
{
}





/* MPEG audio methods. */

#undef _THIS
#define _THIS struct MPEGaudio*self
#undef METH
#define METH(name) MPEGaudio_##name
/* The actual MPEG audio class */

void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len)
{
}

int Play_MPEGaudio(struct MPEGaudio *audio, Uint8 *stream, int len)
{
}

#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata)
{
}
#endif

MPEGaudio * METH(create) (struct MPEGstream *stream, bool initSDL)
{
}

void METH(destroy) (_THIS)
{
}

/* MPEG actions */
bool METH(GetAudioInfo) (_THIS, MPEG_AudioInfo *info)
{
}

double METH(Time) (void)
{
}

void METH(Play) (_THIS)
{
}

void METH(Stop) (_THIS)
{
}

void METH(Rewind) (_THIS)
{
}

void METH(ResetSynchro) (_THIS, double time)
{
}

void METH(Skip) (_THIS, float seconds)
{
}

void METH(Volume) (_THIS, int vol)
{
}

MPEGstatus METH(GetStatus) (_THIS)
{
}


/* Returns the desired SDL audio spec for this stream */
bool METH(WantedSpec) (_THIS, SDL_AudioSpec *wanted)
{
}


/* Inform SMPEG of the actual audio format if configuring SDL
   outside of this class */
void METH(ActualSpec) (_THIS, const SDL_AudioSpec *actual)
{
}


#ifdef THREADED_AUDIO
void METH(StartDecoding) (_THIS)
{
}

void METH(StopDecoding) (_THIS)
{
}

#endif

/* from splay 1.8.2 */

  /*****************************/
  /* Constant tables for layer */
  /*****************************/
/* XXX: shit */
//  static const int bitrate[2][3][15],frequencies[2][3]

//  static const REAL scalefactorstable[64]

//  static const HUFFMANCODETABLE ht[HTN]

//  static const REAL filter[512]

//  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4


  /*************************/
  /* MPEG header variables */
  /*************************/

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
void METH(setforcetomono) (_THIS, bool flag)
{
}

void METH(setdownfrequency) (_THIS, int value)
{
}


  /******************************/
  /* Frame management variables */
  /******************************/

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/

  /*******************/
  /* Mpegtoraw class */
  /*******************/
void METH(initialize) (_THIS)
{
}

bool METH(run) (_THIS, int frames, double *timestamp)
{
}

void METH(clearbuffer) (_THIS)
{
}


  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
bool METH(fillbuffer) (_THIS, int size)
{
}

void METH(sync) (_THIS)
{
}

bool METH(issync) (_THIS)
{
}

int METH(getbyte) (_THIS)
{
}

int METH(getbit) (_THIS)
{
}

int METH(getbits8) (_THIS)
{
}

int METH(getbits9) (_THIS, int bits)
{
}

int METH(getbits) (_THIS, int bits)
{
}



  /********************/
  /* Global variables */
  /********************/


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/
bool METH(loadheader) (_THIS)
{
}


  //
  // Subbandsynthesis
  //

void METH(computebuffer) (_THIS, REAL *fraction,REAL buffer[2][CALCBUFFERSIZE])
{
}

void METH(generatesingle) (_THIS)
{
}

void METH(generate) (_THIS)
{
}

void METH(subbandsynthesis) (_THIS, REAL *fractionL,REAL *fractionR)
{
}


void METH(computebuffer_2) (_THIS, REAL *fraction,REAL buffer[2][CALCBUFFERSIZE])
{
}

void METH(generatesingle_2) (_THIS)
{
}

void METH(generate_2) (_THIS)
{
}

void METH(subbandsynthesis_2) (_THIS, REAL *fractionL,REAL *fractionR)
{
}


  // Extractor
void METH(extractlayer1) (_THIS);    // MPEG-1
void METH(extractlayer2) (_THIS)
{
}

void METH(extractlayer3) (_THIS)
{
}

void METH(extractlayer3_2) (_THIS);  // MPEG-2


  // Functions for layer 3
void METH(layer3initialize) (_THIS)
{
}

bool METH(layer3getsideinfo) (_THIS)
{
}

bool METH(layer3getsideinfo_2) (_THIS)
{
}

void METH(layer3getscalefactors) (_THIS, int ch,int gr)
{
}

void METH(layer3getscalefactors_2) (_THIS, int ch)
{
}

void METH(layer3huffmandecode) (_THIS, int ch,int gr,int out[SBLIMIT][SSLIMIT])
{
}

REAL METH(layer3twopow2) (_THIS, int scale,int preflag,int pretab_offset,int l)
{
}

REAL METH(layer3twopow2_1) (_THIS, int a,int b,int c)
{
}

void METH(layer3dequantizesample) (_THIS, int ch,int gr,int   in[SBLIMIT][SSLIMIT],
                                REAL out[SBLIMIT][SSLIMIT])
{
}

void METH(layer3fixtostereo) (_THIS, int gr,REAL  in[2][SBLIMIT][SSLIMIT])
{
}

void METH(layer3reorderandantialias) (_THIS, int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
                               REAL out[SBLIMIT][SSLIMIT])
{
}


void METH(layer3hybrid) (_THIS, int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
                          REAL out[SSLIMIT][SBLIMIT])
{
}

  
void METH(huffmandecoder_1) (_THIS, const HUFFMANCODETABLE *h,int *x,int *y)
{
}

void METH(huffmandecoder_2) (_THIS, const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w)
{
}


  /********************/
  /* Playing raw data */
  /********************/


