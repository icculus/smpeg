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

#ifndef _MPEGAUDIO_H_
#define _MPEGAUDIO_H_

#include "SDL.h"
#include "MPEGerror.h"
#include "MPEGaction.h"

#ifdef THREADED_AUDIO
#include "MPEGring.h"
#endif

//class MPEGstream;

/* MPEG/WAVE Sound library

   (C) 1997 by Woo-jae Jung */

/**************************/
/* Define values for MPEG */
/**************************/
#define SCALEBLOCK     12
#define CALCBUFFERSIZE 512
#define MAXSUBBAND     32
#define MAXCHANNEL     2
#define MAXTABLE       2
#define SCALE          32768
#define MAXSCALE       (SCALE-1)
#define MINSCALE       (-SCALE)
#define RAWDATASIZE    (2*2*32*SSLIMIT)

#define LS 0
#define RS 1

#define SSLIMIT      18
#define SBLIMIT      32

#define WINDOWSIZE    4096

// Huffmancode
#define HTN 34

/********************/
/* Type definitions */
/********************/
typedef float REAL;

typedef struct
{
  bool         generalflag;
  unsigned int part2_3_length;
  unsigned int big_values;
  unsigned int global_gain;
  unsigned int scalefac_compress;
  unsigned int window_switching_flag;
  unsigned int block_type;
  unsigned int mixed_block_flag;
  unsigned int table_select[3];
  unsigned int subblock_gain[3];
  unsigned int region0_count;
  unsigned int region1_count;
  unsigned int preflag;
  unsigned int scalefac_scale;
  unsigned int count1table_select;
}layer3grinfo;

typedef struct
{
  unsigned main_data_begin;
  unsigned private_bits;
  struct
  {
    unsigned scfsi[4];
    layer3grinfo gr[2];
  }ch[2];
}layer3sideinfo;

typedef struct
{
  int l[23];            /* [cb] */
  int s[3][13];         /* [window][cb] */
}layer3scalefactor;     /* [ch] */

typedef struct
{
  int tablename;
  unsigned int xlen,ylen;
  unsigned int linbits;
  unsigned int treelen;
  const unsigned int (*val)[2];
}HUFFMANCODETABLE;


#if 0
// Class for Mpeg layer3
class Mpegbitwindow
{
public:
  Mpegbitwindow(){bitindex=point=0;};

  void initialize(void)  {bitindex=point=0;};
  int  gettotalbit(void) const {return bitindex;};
  void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;};
  void wrap(void);
  void rewind(int bits)  {bitindex-=bits;};
  void forward(int bits) {bitindex+=bits;};
  int getbit(void) {
      register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;
      bitindex++;
      return r;
  }
  int getbits9(int bits)
  {
      register unsigned short a;
      { int offset=bitindex>>3;

        a=(((unsigned char)buffer[offset])<<8) | ((unsigned char)buffer[offset+1]);
      }
      a<<=(bitindex&7);
      bitindex+=bits;
      return (int)((unsigned int)(a>>(16-bits)));
  }
  int  getbits(int bits);

private:
  int  point,bitindex;
  char buffer[2*WINDOWSIZE];
};
#endif /* 0 */

// Class for Mpeg layer3
struct Mpegbitwindow
{

  int  point,bitindex;
  char buffer[2*WINDOWSIZE];
};

typedef struct Mpegbitwindow Mpegbitwindow;

#undef _THIS
#define _THIS struct Mpegbitwindow *self
//  Mpegbitwindow(){bitindex=point=0;};
//#define Mpegbitwindow_new() (Mpegbitwindow*)(calloc(sizeof(struct Mpegbitwindow)))
Mpegbitwindow * Mpegbitwindow_new (_THIS);
Mpegbitwindow * Mpegbitwindow_init (_THIS);

//  void initialize(void)  {bitindex=point=0;};
#define Mpegbitwindow_initialize(self) ((self)->bitindex = (self)->point = 0)

//  int  gettotalbit(void) const {return bitindex;};
#define Mpegbitwindow_gettotalbit(self) ((self)->bitindex)

//  void putbyte(int c)    {buffer[point&(WINDOWSIZE-1)]=c;point++;};
//#define Mpegbitwindow_putbyte(self, c) ((self)->buffer[(self)->point&(WINDOWSIZE-1)]=c, (self)->point++)
#define Mpegbitwindow_putbyte(self, c) ((self)->buffer [ (self)->point & (WINDOWSIZE-1) ] = c, (self)->point++, 0)

//  void wrap(void);
void Mpegbitwindow_wrap (_THIS);

//  void rewind(int bits)  {bitindex-=bits;};
#define Mpegbitwindow_rewind(self, bits) ((self)->bitindex -= bits)

//  void forward(int bits) {bitindex+=bits;};
#define Mpegbitwindow_forward(self, bits) ((self)->bitindex += bits)


//  int getbit(void) {
//      register int r=(buffer[bitindex>>3]>>(7-(bitindex&7)))&1;
//      bitindex++;
//      return r;
//  }
#if 0
inline static int Mpegbitwindow_getbit (_THIS) {
  register int r=(self->buffer[self->bitindex>>3]>>(7-(self->bitindex&7)))&1;
  self->bitindex++;
  return r;
}
#endif /* 0 */

//  int getbits9(int bits)
//  {
//      register unsigned short a;
//      { int offset=bitindex>>3;
//
//        a=(((unsigned char)buffer[offset])<<8) | ((unsigned char)buffer[offset+1]);
//      }
//      a<<=(bitindex&7);
//      bitindex+=bits;
//      return (int)((unsigned int)(a>>(16-bits)));
//  }
#if 0
inline static int Mpegbitwindow_getbits9 (_THIS, int bits)
{
  register unsigned short a;
  int offset = self->bitindex>>3;

  a=(((unsigned char)self->buffer[offset])<<8) | ((unsigned char)self->buffer[offset+1]);

  a<<=(self->bitindex&7);
  self->bitindex+=bits;
  return (int)((unsigned int)(a>>(16-bits)));
}
#endif /* 0 */


int Mpegbitwindow_getbit (_THIS);
int Mpegbitwindow_getbits9 (_THIS, int bits);
int Mpegbitwindow_getbits (_THIS, int bits);



#if 0
/* The actual MPEG audio class */
class MPEGaudio : public MPEGerror, public MPEGaudioaction {

    friend void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len);
    friend int Play_MPEGaudio(MPEGaudio *audio, Uint8 *stream, int len);
#ifdef THREADED_AUDIO
    friend int Decode_MPEGaudio(void *udata);
#endif

public:
    MPEGaudio(MPEGstream *stream, bool initSDL = true);
    virtual ~MPEGaudio();

    /* MPEG actions */
    bool GetAudioInfo(MPEG_AudioInfo *info);
    double Time(void);
    void Play(void);
    void Stop(void);
    void Rewind(void);
    void ResetSynchro(double time);
    void Skip(float seconds);
    void Volume(int vol);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name in popcorn */
    MPEGstatus GetStatus(void);

    /* Returns the desired SDL audio spec for this stream */
    bool WantedSpec(SDL_AudioSpec *wanted);

    /* Inform SMPEG of the actual audio format if configuring SDL
       outside of this class */
    void ActualSpec(const SDL_AudioSpec *actual);

protected:
    bool sdl_audio;
    MPEGstream *mpeg;
    int valid_stream;
    bool stereo;
    double rate_in_s;
    Uint32 frags_playing;
    Uint32 frag_time;
#ifdef THREADED_AUDIO
    bool decoding;
    SDL_Thread *decode_thread;

    void StartDecoding(void);
    void StopDecoding(void);
#endif

/* Code from splay 1.8.2 */

  /*****************************/
  /* Constant tables for layer */
  /*****************************/
private:
  static const int bitrate[2][3][15],frequencies[2][3];
  static const REAL scalefactorstable[64];
  static const HUFFMANCODETABLE ht[HTN];
  static const REAL filter[512];
  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

  /*************************/
  /* MPEG header variables */
  /*************************/
private:
  int last_speed;
  int layer,protection,bitrateindex,padding,extendedmode;
  enum _mpegversion  {mpeg1,mpeg2}                               version;
  enum _mode    {fullstereo,joint,dual,single}                   mode;
  enum _frequency {frequency44100,frequency48000,frequency32000} frequency;

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
private:
  bool forcetomonoflag;
  bool forcetostereoflag;
  int  downfrequency;

public:
  void setforcetomono(bool flag);
  void setdownfrequency(int value);

  /******************************/
  /* Frame management variables */
  /******************************/
private:
  int decodedframe,currentframe,totalframe;

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/
private:
  int tableindex,channelbitrate;
  int stereobound,subbandnumber,inputstereo,outputstereo;
  REAL scalefactor;
  int framesize;

  /*******************/
  /* Mpegtoraw class */
  /*******************/
public:
  void initialize();
  bool run(int frames, double *timestamp = NULL);
  void clearbuffer(void);

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
private:
  Uint8 _buffer[4096];
  Uint32 _buffer_pos;
  int  bitindex;
  bool fillbuffer(int size);
  void sync(void);
  bool issync(void);
  int getbyte(void);
  int getbit(void);
  int getbits8(void);
  int getbits9(int bits);
  int getbits(int bits);


  /********************/
  /* Global variables */
  /********************/
private:
  int lastfrequency,laststereo;

  // for Layer3
  int layer3slots,layer3framestart,layer3part2start;
  REAL prevblck[2][2][SBLIMIT][SSLIMIT];
  int currentprevblock;
  layer3sideinfo sideinfo;
  layer3scalefactor scalefactors[2];

  Mpegbitwindow bitwindow;
  int wgetbit  (void)    {return bitwindow.getbit  ();    }
  int wgetbits9(int bits){return bitwindow.getbits9(bits);}
  int wgetbits (int bits){return bitwindow.getbits (bits);}


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/
private:
  bool loadheader(void);

  //
  // Subbandsynthesis
  //
  REAL calcbufferL[2][CALCBUFFERSIZE],calcbufferR[2][CALCBUFFERSIZE];
  int  currentcalcbuffer,calcbufferoffset;

  void computebuffer(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle(void);
  void generate(void);
  void subbandsynthesis(REAL *fractionL,REAL *fractionR);

  void computebuffer_2(REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
  void generatesingle_2(void);
  void generate_2(void);
  void subbandsynthesis_2(REAL *fractionL,REAL *fractionR);

  // Extarctor
  void extractlayer1(void);    // MPEG-1
  void extractlayer2(void);
  void extractlayer3(void);
  void extractlayer3_2(void);  // MPEG-2


  // Functions for layer 3
  void layer3initialize(void);
  bool layer3getsideinfo(void);
  bool layer3getsideinfo_2(void);
  void layer3getscalefactors(int ch,int gr);
  void layer3getscalefactors_2(int ch);
  void layer3huffmandecode(int ch,int gr,int out[SBLIMIT][SSLIMIT]);
  REAL layer3twopow2(int scale,int preflag,int pretab_offset,int l);
  REAL layer3twopow2_1(int a,int b,int c);
  void layer3dequantizesample(int ch,int gr,int   in[SBLIMIT][SSLIMIT],
                                REAL out[SBLIMIT][SSLIMIT]);
  void layer3fixtostereo(int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
  void layer3reorderandantialias(int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
                               REAL out[SBLIMIT][SSLIMIT]);

  void layer3hybrid(int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
                          REAL out[SSLIMIT][SBLIMIT]);
  
  void huffmandecoder_1(const HUFFMANCODETABLE *h,int *x,int *y);
  void huffmandecoder_2(const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

  /********************/
  /* Playing raw data */
  /********************/
private:
  int     samplesperframe;
  int     rawdatareadoffset, rawdatawriteoffset;
  Sint16 *rawdata;
#ifdef THREADED_AUDIO
  MPEG_ring *ring;
#else
  Sint16  spillover[ RAWDATASIZE ];
#endif
  int volume;

  void clearrawdata(void)    {
        rawdatareadoffset=0;
        rawdatawriteoffset=0;
        rawdata=NULL;
  }
  void putraw(short int pcm) {rawdata[rawdatawriteoffset++]=pcm;}

  /********************/
  /* Timestamp sync   */
  /********************/
public:
#define N_TIMESTAMPS 5

  double timestamp[N_TIMESTAMPS];
};
#endif /* 0 */




enum MPEGaudio_mpegversion  {mpeg1,mpeg2};
typedef enum MPEGaudio_mpegversion MPEGaudio_mpegversion;

enum MPEGaudio_mode    {fullstereo,joint,dual,single};
typedef enum MPEGaudio_mode MPEGaudio_mode;

enum MPEGaudio_frequency {frequency44100,frequency48000,frequency32000};
typedef enum MPEGaudio_frequency MPEGaudio_frequency;



/* MPEG audio state information. */

struct MPEGaudio {
  MPEGerror *error;

/*  MPEGaction *action; */
  bool playing;
  bool paused;
  bool looping;
  double play_time;

    bool sdl_audio;
    struct MPEGstream *mpeg;
    int valid_stream;
    bool stereo;
    double rate_in_s;
    Uint32 frags_playing;
    Uint32 frag_time;
#ifdef THREADED_AUDIO
    bool decoding;
    SDL_Thread *decode_thread;

#endif

/* from splay 1.8.2 */

  /*****************************/
  /* Constant tables for layer */
  /*****************************/
/* XXX: SHIT */
//  static const int bitrate[2][3][15],frequencies[2][3];
//  static const REAL scalefactorstable[64];
//  static const HUFFMANCODETABLE ht[HTN];
//  static const REAL filter[512];
//  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

  /*************************/
  /* MPEG header variables */
  /*************************/
  int last_speed;
  int layer,protection,bitrateindex,padding,extendedmode;
  enum MPEGaudio_mpegversion   version;
  enum MPEGaudio_mode          mode;
  enum MPEGaudio_frequency     frequency;

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
  bool forcetomonoflag;
  bool forcetostereoflag;
  int  downfrequency;

  /******************************/
  /* Frame management variables */
  /******************************/
  int decodedframe,currentframe,totalframe;

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/
  int tableindex,channelbitrate;
  int stereobound,subbandnumber,inputstereo,outputstereo;
  REAL scalefactor;
  int framesize;

  /*******************/
  /* Mpegtoraw class */
  /*******************/

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
  Uint8 _buffer[4096];
  Uint32 _buffer_pos;
  int  bitindex;


  /********************/
  /* Global variables */
  /********************/
  int lastfrequency,laststereo;

  // for Layer3
  int layer3slots,layer3framestart,layer3part2start;
  REAL prevblck[2][2][SBLIMIT][SSLIMIT];
  int currentprevblock;
  layer3sideinfo sideinfo;
  layer3scalefactor scalefactors[2];

  struct Mpegbitwindow bitwindow;
//  int wgetbit  (void)    {return bitwindow.getbit  ();    }
#define MPEGaudio_wgetbit(self) (Mpegbitwindow_getbit(&(self->bitwindow)))
//  int wgetbits9(int bits){return bitwindow.getbits9(bits);}
#define MPEGaudio_wgetbits9(self, bits) (Mpegbitwindow_getbits9(&(self->bitwindow), bits))
//  int wgetbits (int bits){return bitwindow.getbits (bits);}
#define MPEGaudio_wgetbits(self, bits) (Mpegbitwindow_getbits(&(self->bitwindow), bits))


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/

  //
  // Subbandsynthesis
  //
  REAL calcbufferL[2][CALCBUFFERSIZE],calcbufferR[2][CALCBUFFERSIZE];
  int  currentcalcbuffer,calcbufferoffset;


  // Extractor

  // Functions for layer 3

  /********************/
  /* Playing raw data */
  /********************/
  int     samplesperframe;
  int     rawdatareadoffset, rawdatawriteoffset;
  Sint16 *rawdata;
#ifdef THREADED_AUDIO
  MPEG_ring *ring;
#else
  Sint16  spillover[ RAWDATASIZE ];
#endif
  int volume;

//  void clearrawdata(void)    {
//        rawdatareadoffset=0;
//        rawdatawriteoffset=0;
//        rawdata=NULL;
//  }
#define MPEGaudio_clearrawdata(self) (self->rawdatareadoffset = 0, self->rawdatawriteoffset = 0, self->rawdata = 0)
//  void putraw(short int pcm) {rawdata[rawdatawriteoffset++]=pcm;}
#define MPEGaudio_putraw(self, pcm) (self->rawdata[self->rawdatawriteoffset++] = pcm)

  /********************/
  /* Timestamp sync   */
  /********************/
#define N_TIMESTAMPS 5

  double timestamp[N_TIMESTAMPS];

//  MPEGerror *error;
};

typedef struct MPEGaudio MPEGaudio;













/* MPEG audio methods. */

#undef _THIS
#define _THIS struct MPEGaudio*
/* The actual MPEG audio class */

void Play_MPEGaudioSDL(void *udata, Uint8 *stream, int len);
int Play_MPEGaudio(struct MPEGaudio *audio, Uint8 *stream, int len);
#ifdef THREADED_AUDIO
int Decode_MPEGaudio(void *udata);
#endif

_THIS MPEGaudio_init (_THIS, struct MPEGstream *stream, bool initSDL);
void MPEGaudio_destroy (_THIS);

/* MPEG actions */
bool MPEGaudio_GetAudioInfo (_THIS, MPEG_AudioInfo *info);
double MPEGaudio_Time (_THIS);
void MPEGaudio_Play (_THIS);
void MPEGaudio_Stop (_THIS);
void MPEGaudio_Rewind (_THIS);
void MPEGaudio_Pause (_THIS);
void MPEGaudio_ResetSynchro (_THIS, double time);
void MPEGaudio_Skip (_THIS, float seconds);
void MPEGaudio_Volume (_THIS, int vol);
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name in popcorn */
MPEGstatus MPEGaudio_GetStatus (_THIS);

/* Returns the desired SDL audio spec for this stream */
bool MPEGaudio_WantedSpec (_THIS, SDL_AudioSpec *wanted);

/* Inform SMPEG of the actual audio format if configuring SDL
   outside of this class */
void MPEGaudio_ActualSpec (_THIS, const SDL_AudioSpec *actual);

#ifdef THREADED_AUDIO
void MPEGaudio_StartDecoding (_THIS);
void MPEGaudio_StopDecoding (_THIS);
#endif

/* from splay 1.8.2 */

  /*****************************/
  /* Constant tables for layer */
  /*****************************/
/* XXX: shit */
//  static const int bitrate[2][3][15],frequencies[2][3];
//  static const REAL scalefactorstable[64];
//  static const HUFFMANCODETABLE ht[HTN];
//  static const REAL filter[512];
//  static REAL hcos_64[16],hcos_32[8],hcos_16[4],hcos_8[2],hcos_4;

  /*************************/
  /* MPEG header variables */
  /*************************/

  /***************************************/
  /* Interface for setting music quality */
  /***************************************/
void MPEGaudio_setforcetomono (_THIS, bool flag);
void MPEGaudio_setdownfrequency (_THIS, int value);

  /******************************/
  /* Frame management variables */
  /******************************/

  /***************************************/
  /* Variables made by MPEG-Audio header */
  /***************************************/

  /*******************/
  /* Mpegtoraw class */
  /*******************/
void MPEGaudio_initialize (_THIS);
bool MPEGaudio_run (_THIS, int frames, double *timestamp);
void MPEGaudio_clearbuffer (_THIS);

  /*****************************/
  /* Loading MPEG-Audio stream */
  /*****************************/
bool MPEGaudio_fillbuffer (_THIS, int size);
void MPEGaudio_sync (_THIS);
bool MPEGaudio_issync (_THIS);
int MPEGaudio_getbyte (_THIS);
int MPEGaudio_getbit (_THIS);
int MPEGaudio_getbits8 (_THIS);
int MPEGaudio_getbits9 (_THIS, int bits);
int MPEGaudio_getbits (_THIS, int bits);


  /********************/
  /* Global variables */
  /********************/


  /*************************************/
  /* Decoding functions for each layer */
  /*************************************/
bool MPEGaudio_loadheader (_THIS);

  //
  // Subbandsynthesis
  //

void MPEGaudio_computebuffer (_THIS, REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
void MPEGaudio_generatesingle (_THIS);
void MPEGaudio_generate (_THIS);
void MPEGaudio_subbandsynthesis (_THIS, REAL *fractionL,REAL *fractionR);

void MPEGaudio_computebuffer_2 (_THIS, REAL *fraction,REAL buffer[2][CALCBUFFERSIZE]);
void MPEGaudio_generatesingle_2 (_THIS);
void MPEGaudio_generate_2 (_THIS);
void MPEGaudio_subbandsynthesis_2 (_THIS, REAL *fractionL,REAL *fractionR);

  // Extractor
void MPEGaudio_extractlayer1 (_THIS);    // MPEG-1
void MPEGaudio_extractlayer2 (_THIS);
void MPEGaudio_extractlayer3 (_THIS);
void MPEGaudio_extractlayer3_2 (_THIS);  // MPEG-2


  // Functions for layer 3
void MPEGaudio_layer3initialize (_THIS);
bool MPEGaudio_layer3getsideinfo (_THIS);
bool MPEGaudio_layer3getsideinfo_2 (_THIS);
void MPEGaudio_layer3getscalefactors (_THIS, int ch,int gr);
void MPEGaudio_layer3getscalefactors_2 (_THIS, int ch);
void MPEGaudio_layer3huffmandecode (_THIS, int ch,int gr,int out[SBLIMIT][SSLIMIT]);
REAL MPEGaudio_layer3twopow2 (_THIS, int scale,int preflag,int pretab_offset,int l);
REAL MPEGaudio_layer3twopow2_1 (_THIS, int a,int b,int c);
void MPEGaudio_layer3dequantizesample (_THIS, int ch,int gr,int   in[SBLIMIT][SSLIMIT],
                                REAL out[SBLIMIT][SSLIMIT]);
void MPEGaudio_layer3fixtostereo (_THIS, int gr,REAL  in[2][SBLIMIT][SSLIMIT]);
void MPEGaudio_layer3reorderandantialias (_THIS, int ch,int gr,REAL  in[SBLIMIT][SSLIMIT],
                               REAL out[SBLIMIT][SSLIMIT]);

void MPEGaudio_layer3hybrid (_THIS, int ch,int gr,REAL in[SBLIMIT][SSLIMIT],
                          REAL out[SSLIMIT][SBLIMIT]);
  
void MPEGaudio_huffmandecoder_1 (_THIS, const HUFFMANCODETABLE *h,int *x,int *y);
void MPEGaudio_huffmandecoder_2 (_THIS, const HUFFMANCODETABLE *h,int *x,int *y,int *v,int *w);

  /********************/
  /* Playing raw data */
  /********************/







  /*****************************/
  /* Constant tables for layer */
  /*****************************/
/* XXX: SHIT */
const int MPEGaudio_bitrate[2][3][15],
          MPEGaudio_frequencies[2][3];
const REAL MPEGaudio_scalefactorstable[64];
const HUFFMANCODETABLE MPEGaudio_ht[HTN];
const REAL MPEGaudio_filter[512];
REAL MPEGaudio_hcos_64[16],
     MPEGaudio_hcos_32[8],
     MPEGaudio_hcos_16[4],
     MPEGaudio_hcos_8[2],
     MPEGaudio_hcos_4;



/* other virtual methods in MPEGaction */
void MPEGaudio_ResetPause (_THIS);


#endif /* _MPEGAUDIO_H_ */
