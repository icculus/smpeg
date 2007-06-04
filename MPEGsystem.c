#include <stdlib.h>        /* for realloc() */
#include <string.h>        /* for memmove() */
#include <errno.h>
#include <assert.h>
#ifdef WIN32
#include <sys/types.h>
#include <io.h>
#include <winsock.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#ifndef __BEOS__
#include <sys/select.h>
#endif
#endif

#include "MPEGsystem.h"
#include "MPEGstream.h"

//static bool MPEGsystem_SystemLoop(MPEGsystem *self);
/* virtual */ Uint8 MPEGsystem_FillBuffer(MPEGsystem *self);
/* virtual */ void MPEGsystem_Read(MPEGsystem *self);
//static int MPEGsystem_SystemThread(void * udata);

/* Define this if you want to debug the system stream parsing */
//#define DEBUG_SYSTEM

/* Define this if you want to use a separate thread for stream decoding */
//#define USE_SYSTEM_THREAD

Uint8 const PACKET_CODE[]       = { 0x00, 0x00, 0x01, 0xba };
Uint8 const PACKET_MASK[]       = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END_CODE[]          = { 0x00, 0x00, 0x01, 0xb9 };
Uint8 const END_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END2_CODE[]         = { 0x00, 0x00, 0x01, 0xb7 };
Uint8 const END2_MASK[]         = { 0xff, 0xff, 0xff, 0xff };
Uint8 const VIDEOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xe0 };
Uint8 const VIDEOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xe0 };
Uint8 const AUDIOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xc0 };
Uint8 const AUDIOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xc0 };
Uint8 const PADSTREAM_CODE[]    = { 0x00, 0x00, 0x01, 0xbe };
Uint8 const PADSTREAM_MASK[]    = { 0xff, 0xff, 0xff, 0xff };
Uint8 const SYSTEMSTREAM_CODE[] = { 0x00, 0x00, 0x01, 0xbb };
Uint8 const SYSTEMSTREAM_MASK[] = { 0xff, 0xff, 0xff, 0xff };
Uint8 const USERSTREAM_CODE[]   = { 0x00, 0x00, 0x01, 0xb2 };
Uint8 const USERSTREAM_MASK[]   = { 0xff, 0xff, 0xff, 0xff };
Uint8 const VIDEO_CODE[]        = { 0x00, 0x00, 0x01, 0xb3 };
Uint8 const VIDEO_MASK[]        = { 0xff, 0xff, 0xff, 0xff };
Uint8 const AUDIO_CODE[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const AUDIO_MASK[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const GOP_CODE[]          = { 0x00, 0x00, 0x01, 0xb8 };
Uint8 const GOP_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const PICTURE_CODE[]      = { 0x00, 0x00, 0x01, 0x00 };
Uint8 const PICTURE_MASK[]      = { 0xff, 0xff, 0xff, 0xff };
Uint8 const SLICE_CODE[]        = { 0x00, 0x00, 0x01, 0x01 };
Uint8 const SLICE_MASK[]        = { 0xff, 0xff, 0xff, 0x00 };
Uint8 const ZERO_CODE[]         = { 0x00, 0x00, 0x00, 0x00 };
Uint8 const FULL_MASK[]         = { 0xff, 0xff, 0xff, 0xff };

/* The size is arbitrary but should be sufficient to contain */
/* two MPEG packets and reduce disk (or network) access.     */
// Hiroshi Yamashita notes that the original size was too large
#define MPEG_BUFFER_SIZE (16 * 1024)

/* The granularity (2^LG2_GRANULARITY) determine what length of read data */
/* will be a multiple of, e.g. setting LG2_GRANULARITY to 12 will make    */
/* read size (when calling the read function in Read method) to be a      */
/* multiple of 4096                                                       */
#define LG2_GRANULARITY 12
#define READ_ALIGN(x) (((x) >> LG2_GRANULARITY) << LG2_GRANULARITY)

/* This defines the maximum number of buffers that can be preread */
/* It is to prevent filling the whole memory with buffers on systems */
/* where read is immediate such as in the case of files */
#define MPEG_BUFFER_MAX 16

/* Timeout before read fails */
#define READ_TIME_OUT 1000000

/* timestamping */
#define FLOAT_0x10000 (double)((Uint32)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ 90000L

/* This work only on little endian systems */
/*
#define REV(x) ((((x)&0x000000FF)<<24)| \
                (((x)&0x0000FF00)<< 8)| \
                (((x)&0x00FF0000)>> 8)| \
                (((x)&0xFF000000)>>24))
#define MATCH4(x, y, m) (((x) & REV(m)) == REV(y))
*/
const int audio_frequencies[2][3]=
{
  {44100,48000,32000}, // MPEG 1
  {22050,24000,16000}  // MPEG 2
};

const int audio_bitrate[2][3][15]=
{
  // MPEG 1
  {{0,32,64,96,128,160,192,224,256,288,320,352,384,416,448},
   {0,32,48,56,64,80,96,112,128,160,192,224,256,320,384},
   {0,32,40,48,56,64,80,96,112,128,160,192,224,256,320}},

  // MPEG 2
  {{0,32,48,56,64,80,96,112,128,144,160,176,192,224,256},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160},
   {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160}}
};

/* Match two 4-byte codes */
static inline bool Match4(Uint8 const code1[4], Uint8 const code2[4], Uint8 const mask[4])
{
  return( ((code1[0] & mask[0]) == (code2[0] & mask[0])) &&
	  ((code1[1] & mask[1]) == (code2[1] & mask[1])) &&
	  ((code1[2] & mask[2]) == (code2[2] & mask[2])) &&
	  ((code1[3] & mask[3]) == (code2[3] & mask[3])) );
}

static inline double read_time_code(Uint8 *pointer)
{ 
  double timestamp;
  Uint8 hibit; Uint32 lowbytes;

  hibit = (pointer[0]>>3)&0x01;
  lowbytes = (((Uint32)pointer[0] >> 1) & 0x03) << 30;
  lowbytes |= (Uint32)pointer[1] << 22;
  lowbytes |= ((Uint32)pointer[2] >> 1) << 15;
  lowbytes |= (Uint32)pointer[3] << 7;
  lowbytes |= ((Uint32)pointer[4]) >> 1;
  timestamp = (double)hibit*FLOAT_0x10000*FLOAT_0x10000+(double)lowbytes;
  timestamp /= STD_SYSTEM_CLOCK_FREQ;

  return timestamp;
}

/* Return true if there is a valid audio header at the beginning of pointer */
static inline Uint32 audio_header(Uint8 * pointer, Uint32 * framesize, double * frametime)
{
  Uint32 layer, version, frequency, bitrate, mode, padding, size;

  if(((pointer[0] & 0xff) != 0xff) || // No sync bits
     ((pointer[1] & 0xf0) != 0xf0) || //
     ((pointer[2] & 0xf0) == 0x00) || // Bitrate is 0
     ((pointer[2] & 0xf0) == 0xf0) || // Bitrate is 15
     ((pointer[2] & 0x0c) == 0x0c) || // Frequency is 3
     ((pointer[1] & 0x06) == 0x00))   // Layer is 4
     return(0);

  layer = 4 - (((pointer)[1] >> 1) & 3);
  version = (((pointer)[1] >> 3) & 1) ^ 1;
  padding = ((pointer)[2] >> 1) & 1;
  frequency = audio_frequencies[version][(((pointer)[2] >> 2) & 3)];
  bitrate = audio_bitrate[version][layer-1][(((pointer)[2] >> 4) & 15)];
  mode = ((pointer)[3] >> 6) & 3;

  if(layer==1)
  {
      size = 12000 * bitrate / frequency;
      if(frequency==0 && padding) size++;
      size <<= 2;
  }
  else
  {
      size = 144000 * bitrate / (frequency<<version);
      if(padding) size++;
  }
  if(framesize) *framesize = size;
  if(frametime) *frametime = 8.0 * size / (1000. * bitrate);
  
  return(4); /* Audio header size */
}

/* Search the next valid audio header */
static inline bool audio_aligned(Uint8 *pointer, Uint32 size)
{
    Uint32 i, s;

#if 1 // Sam 10/27 - Don't choke on partially corrupt data
    size = 4;
#endif
    /* Check on all data available that next headers are aligned too */
    for(i = 0; i + 3 < size && audio_header(pointer+i, &s, 0); i+=s);

    if(i + 3 < size)
	return(false);
    else
	return(true);
}

/* Return true if there is a valid sequence header at the beginning of pointer */
static inline Uint32 sequence_header(Uint8 * pointer, Uint32 size, double * _frametime)
{
  double frametime;
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, VIDEO_CODE, VIDEO_MASK)) 
    return(0); /* Not a sequence start code */
  
  /* Parse the sequence header information */
  if((header_size+=8) >= size) return(0);
  switch(pointer[7]&0xF)                /*  4 bits of fps */
  {
    case 1: frametime = 1.0/23.97; break;
    case 2: frametime = 1.0/24.00; break;
    case 3: frametime = 1.0/25.00; break;
    case 4: frametime = 1.0/29.97; break;
    case 5: frametime = 1.0/30.00; break;
    case 6: frametime = 1.0/50.00; break;
    case 7: frametime = 1.0/59.94; break;
    case 8: frametime = 1.0/60.00; break;
    case 9: frametime = 1.0/15.00; break;
    default: frametime = 1.0/30.00; break;
  }

  if(_frametime) *_frametime = frametime;
  
  return(header_size); /* sequence header size */
}

/* Return true if there is a valid gop header at the beginning of pointer */
static inline Uint32 gop_header(Uint8 * pointer, Uint32 size, double * timestamp)
{
  Uint32 header_size;
  Uint32 hour, min, sec, frame;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, GOP_CODE, GOP_MASK)) 
    return(0); /* Not a gop start code */
  
  /* Parse the gop header information */
  hour = (pointer[4] >> 2) & 31;
  min = ((pointer[4] & 3) << 4) | ((pointer[5] >> 4) & 15);
  sec = ((pointer[5] & 7) << 3) | ((pointer[6] >> 5) & 7);
  frame = ((pointer[6] & 31) << 1) | ((pointer[7] >> 7) & 1);
  if((header_size+=4) >= size) return(0);

  if(timestamp) *timestamp = sec + 60.*min + 3600.*hour;
  
  return(header_size); /* gop header size */
}

/* Return true if there is a valid picture header at the beginning of pointer */
static inline Uint32 picture_header(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);

  if(!Match4(pointer, PICTURE_CODE, PICTURE_MASK)) 
    return(0); /* Not a picture start code */
  
  /* Parse the picture header information */
  if((header_size+=4) >= size) return(0);
  
  return(header_size); /* picture header size */
}

/* Return true if there is a valid slice header at the beginning of pointer */
static inline Uint32 slice_header(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!(Match4(pointer, SLICE_CODE, SLICE_MASK) && 
       pointer[3] >= 0x01 && pointer[3] <= 0xaf)) 
    return(0); /* Not a slice start code */
  
  return(header_size); /* slice header size */
}

/* Return true if there is a valid packet header at the beginning of pointer */
static inline Uint32 packet_header(Uint8 * pointer, Uint32 size, double * _timestamp)
{
  double timestamp;
  Uint32 header_size;

  header_size = 0;
  if((header_size+=4) >= size) return(0);
  if(!Match4(pointer, PACKET_CODE, PACKET_MASK)) 
    return(0); /* Not a packet start code */

  /* Parse the packet information */
  if((header_size+=8) >= size) return(0);
  timestamp = read_time_code(pointer+4);

  if(_timestamp) *_timestamp = timestamp;
  
  return(header_size); /* packet header size */
}

/* Return true if there is a valid stream header at the beginning of pointer */
static inline Uint32 stream_header(Uint8 * pointer, Uint32 size, Uint32 * _packet_size, Uint8 * _stream_id, double * _stream_timestamp, double timestamp)
{
  Uint32 header_size, packet_size;
  Uint8 stream_id;
  double stream_timestamp;

  header_size = 0;
  if((header_size += 4) >= size) return(0); 

  if(!Match4(pointer, SYSTEMSTREAM_CODE, SYSTEMSTREAM_MASK) &&
     !Match4(pointer, AUDIOSTREAM_CODE, AUDIOSTREAM_MASK)   &&
     !Match4(pointer, VIDEOSTREAM_CODE, VIDEOSTREAM_MASK)   &&
     !Match4(pointer, PADSTREAM_CODE, PADSTREAM_MASK)       &&
     !Match4(pointer, USERSTREAM_CODE, USERSTREAM_MASK))
    return(0); /* Unknown encapsulated stream */

  /* Parse the stream packet */
  /* Get the stream id, and packet length */
  stream_id = pointer[3];

  pointer += 4;
  if((header_size += 2) >= size) return(0); 
  packet_size = (((unsigned short) pointer[0] << 8) | pointer[1]);
  pointer += 2;

  /* Skip stuffing bytes */
  while ( pointer[0] == 0xff ) {
      ++pointer;
      if((++header_size) >= size) return(0); 
      --packet_size;
  }
  if ( (pointer[0] & 0x40) == 0x40 ) {
      pointer += 2;
      if((header_size += 2) >= size) return(0); 
      packet_size -= 2;
  }
  if ( (pointer[0] & 0x20) == 0x20 ) {
      /* get the PTS */
      stream_timestamp = read_time_code(pointer);
      /* we don't care about DTS */
      if ( (pointer[0] & 0x30) == 0x30 ){
	pointer += 5;
	if((header_size += 5) >= size) return(0); 
	packet_size -= 5;
      }
      pointer += 4;
      if((header_size += 4) >= size) return(0); 
      packet_size -= 4;
  }
  else if ( pointer[0] != 0x0f && pointer[0] != 0x80)
      return(0);      /* not a valid header */
  else
      stream_timestamp = timestamp;
    
  if((++header_size) >= size) return(0); 
  --packet_size;

  if(_packet_size) *_packet_size = packet_size;
  if(_stream_id) *_stream_id = stream_id;
  if(_stream_timestamp) *_stream_timestamp = stream_timestamp;
  
  return(header_size);
}

/* Search the next valid audio header */
static inline bool system_aligned(Uint8 *pointer, Uint32 size)
{
    Uint32 i, s;

    /* Check that packet contains at least one stream */
    i = 0;
    while((s = packet_header(pointer+i, size-i, 0)) != 0)
      if((i+=s) >= size) return(true);

    if((s = stream_header(pointer+i, size-i, 0, 0, 0, 0)) != 0)
      return(true);
    else
      return(false);
}

/* Skip possible zeros at the beggining of the packet */
Uint32 skip_zeros(Uint8 * pointer, Uint32 size)
{
  Uint32 header_size;
  Uint8 const one[4]  = {0x00,0x00,0x00,0x01};

  if(!size) return(0);

  header_size = 0;
  while(Match4(pointer, ZERO_CODE, FULL_MASK))
  {
    pointer++;
    if((++header_size) >= size - 4) return(0); 

    if(Match4(pointer, one, FULL_MASK))
    {
      pointer++;
      if((++header_size) >= size - 4) return(0); 
    }
  }
  return(header_size);
}



#undef _THIS
#define _THIS MPEGsystem *self



MPEGsystem *
MPEGsystem_init (_THIS)
{
  MAKE_OBJECT(MPEGsystem);

  self->error = MPEGerror_init(NULL);
  return self;
}

MPEGsystem *
MPEGsystem_init_mpeg_source (_THIS, SDL_RWops *mpeg_source)
{
//    MPEGsystem *self = MPEGsystem_init(NULL);
    int tries = 0;

    self = MPEGsystem_init(self);
    self->source = mpeg_source;

    /* Create a new buffer for reading */
//    self->read_buffer = new Uint8[MPEG_BUFFER_SIZE];
    self->read_buffer = (Uint8*)calloc(MPEG_BUFFER_SIZE, sizeof(Uint8));

    /* Create a mutex to avoid concurrent access to the stream */
    self->system_mutex = SDL_CreateMutex();
    self->request_wait = SDL_CreateSemaphore(0);

    /* Invalidate the read buffer */
    self->pointer = self->read_buffer;
    self->read_size = 0;
    self->read_total = 0;
    self->packet_total = 0;
    self->endofstream = self->errorstream = false;
    self->frametime = 0.0;
    self->stream_timestamp = 0.0;

    /* Create an empty stream list */
    self->stream_list =
	(MPEGstream **) malloc(sizeof(MPEGstream *));
    self->stream_list[0] = 0;

    /* Create the system stream and add it to the list */
    if(!MPEGsystem_get_stream(self, SYSTEM_STREAMID))
//	MPEGsystem_add_stream(ret, new MPEGstream(this, SYSTEM_STREAMID));
	MPEGsystem_add_stream(self, MPEGstream_init(NULL, self, SYSTEM_STREAMID));

    self->timestamp = 0.0;
    self->timedrift = 0.0;
    self->skip_timestamp = -1;
    self->system_thread_running = false;
    self->system_thread = 0;

    /* Search the MPEG for the first header */
    if(!MPEGsystem_seek_first_header(self))
    {
	self->errorstream = true;
	MPEGerror_SetError(self->error, "Could not find the beginning of MPEG data\n");
	return self;
    }

#ifdef USE_SYSTEM_THREAD
    /* Start the system thread */
    self->system_thread = SDL_CreateThread(SystemThread, this);

    /* Wait for the thread to start */
    while(!self->system_thread_running && !MPEGsystem_Eof(self))
	SDL_Delay(1);
#else
    self->system_thread_running = true;
#endif

    /* Look for streams */
    do
    {
	MPEGsystem_RequestBuffer(self);
	MPEGsystem_Wait(self);
	if ( tries++ < 20 ) {  // Adjust this to catch more streams
	    if ( MPEGsystem_exist_stream(self, VIDEO_STREAMID, 0xF0) &&
		 MPEGsystem_exist_stream(self, AUDIO_STREAMID, 0xF0) ) {
		break;
	    }
	} else {
	    if ( MPEGsystem_exist_stream(self, VIDEO_STREAMID, 0xF0) ||
		 MPEGsystem_exist_stream(self, AUDIO_STREAMID, 0xF0) ) {
		break;
	    }
	}
    }
    while(!MPEGsystem_Eof(self));

    return self;
}

void
MPEGsystem_destroy (_THIS)
{
    MPEGstream ** list;

    /* Kill the system thread */
    MPEGsystem_Stop(self);

    SDL_DestroySemaphore(self->request_wait);
    SDL_DestroyMutex(self->system_mutex);

    /* Delete the streams */
    for(list = self->stream_list; *list; list ++)
      {
//	delete *list;
        MPEGstream_destroy(*list);
        free(*list);
      }
    list = NULL;

    free(self->stream_list);
    self->stream_list = NULL;

    /* Delete the read buffer */
//    delete[] self->read_buffer;
    free(self->read_buffer);
    self->read_buffer = NULL;

  MPEGerror_destroy(self->error);
  free(self->error);
  self->error = NULL;
}

MPEGstream **
MPEGsystem_GetStreamList (_THIS)
{
    return(self->stream_list);
}

void
MPEGsystem_Read (_THIS)
{
    int remaining;
    int timeout;

    /* Lock to prevent concurrent access to the stream */
    SDL_mutexP(self->system_mutex);

    timeout = READ_TIME_OUT;
    remaining = self->read_buffer + self->read_size - self->pointer;

    /* Only read data if buffer is rather empty */
    if(remaining < MPEG_BUFFER_SIZE / 2)
    {
	if(remaining < 0)
	{
	    /* Hum.. we'd better stop if we have already read past the buffer size */
	    self->errorstream = true;
	    SDL_mutexV(self->system_mutex);
	    return;
	}

	/* Replace unread data at the beginning of the stream */
	memmove(self->read_buffer, self->pointer, remaining);

#ifdef NO_GRIFF_MODS
	self->read_size = SDL_RWread(self->source,
				     self->read_buffer + remaining,
				     READ_ALIGN(MPEG_BUFFER_SIZE - remaining));
	if(self->read_size < 0)
	{
	    perror("Read");
	    self->errorstream = true;
	    SDL_mutexV(self->system_mutex);
	    return;
	}
#else
{
	/* Read new data */
	int bytes_read    = 0;
	int buffer_offset = remaining;
	int bytes_to_read = READ_ALIGN(MPEG_BUFFER_SIZE - remaining);
	int read_at_once = 0;

	self->read_size = 0;
	do
	{
	    read_at_once =
		SDL_RWread(self->source, self->read_buffer + buffer_offset, 1, bytes_to_read );

	    if(read_at_once < 0)
	    {
		perror("Read");
		self->errorstream = true;
		SDL_mutexV(self->system_mutex);
		return;
	    }
	    else
	    {
		bytes_read    += read_at_once;
		buffer_offset += read_at_once;
		self->read_size += read_at_once;
		bytes_to_read -= read_at_once;
	    }
	}
	while( read_at_once>0 && bytes_to_read>0 );
}
#endif

	self->read_total += self->read_size;

	self->packet_total ++;

	if((MPEG_BUFFER_SIZE - remaining) != 0 && self->read_size <= 0)
	{
	    if(self->read_size != 0)
	    {
		self->errorstream = true;
		SDL_mutexV(self->system_mutex);
		return;
	    }
	}

	self->read_size += remaining;

	/* Move the pointer */
	self->pointer = self->read_buffer;

	if(self->read_size == 0)
	{
	    /* There is no more data */
	    self->endofstream = true;
	    SDL_mutexV(self->system_mutex);
	    return;
	}
    }

    SDL_mutexV(self->system_mutex);
}

/* ASSUME: stream_list[0] = system stream */
/*         packet length < MPEG_BUFFER_SIZE */
Uint8
MPEGsystem_FillBuffer (_THIS)
{
    Uint8 stream_id;
    Uint32 packet_size;
    Uint32 header_size;
    MPEGstream * stream;


    /* - Read a new packet - */
    MPEGsystem_Read(self);

    if(MPEGsystem_Eof(self))
    {
	MPEGsystem_RequestBuffer(self);
	return(0);
    }

    self->pointer += skip_zeros(self->pointer, self->read_buffer + self->read_size - self->pointer);

    if((header_size = packet_header(self->pointer, self->read_buffer + self->read_size - self->pointer,
				    &self->timestamp)) != 0)
    {
        self->pointer += header_size;
	self->stream_list[0]->pos += header_size;
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "MPEG packet header time: %lf\n", self->timestamp);
#endif
    }

    if((header_size = stream_header(self->pointer, self->read_buffer + self->read_size - self->pointer,
				    &packet_size, &stream_id, &self->stream_timestamp, self->timestamp)) != 0)
    {
	self->pointer += header_size;
	self->stream_list[0]->pos += header_size;
#ifdef DEBUG_SYSTEM
	fprintf(stderr, "[%d] MPEG stream header [%d|%d] id: %d streamtime: %lf\n",
		self->read_total - self->read_size + (self->pointer - self->read_buffer),
		header_size, packet_size, stream_id, self->stream_timestamp);
#endif
    }
    else
	if(Match4(self->pointer, END_CODE, END_MASK) ||
	   Match4(self->pointer, END2_CODE, END2_MASK))
	{
	    /* End codes belong to video stream */
#ifdef DEBUG_SYSTEM
	    fprintf(stderr, "[%d] MPEG end code\n",
		    self->read_total - self->read_size + (self->pointer - self->read_buffer));
#endif
	    stream_id = MPEGsystem_exist_stream(self, VIDEO_STREAMID, 0xF0);
	    packet_size = 4;
	}
	else
	{
	    stream_id = self->stream_list[0]->streamid;

	    if(!self->stream_list[1])
	    {
		//Uint8 * packet_end;

		packet_size = 0;

		/* There is no system info in the stream */

		/* If we're still a system stream, morph to an audio */
		/* or video stream */

		/* Sequence header -> gives framerate */
		while((header_size = sequence_header(self->pointer+packet_size,
						     self->read_buffer + self->read_size - self->pointer -
						     packet_size,
						     &self->frametime)) != 0)
		{
		    stream_id = VIDEO_STREAMID;
		    self->stream_list[0]->streamid = stream_id;
		    packet_size += header_size;
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "MPEG sequence header  frametime: %lf\n", self->frametime);
#endif
		}

		/* GOP header */
		while((header_size = gop_header(self->pointer+packet_size,
						self->read_buffer + self->read_size - self->pointer -
						packet_size,
						0)) != 0)
		{
		    packet_size += header_size;
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "MPEG gop header\n");
#endif
		}

		/* Picture header */
		while((header_size = picture_header(self->pointer+packet_size,
						    self->read_buffer + self->read_size - self->pointer -
						    packet_size)) != 0)
		{
		    /* Warning: header size not quite correct (can be not byte aligned) but this is
		       compensated by skipping a little more, as we don't need to be header aligned after
		       this, since we then check for the next header to know the slice size. */
		    packet_size += header_size;
		    self->stream_timestamp += self->frametime;
		    packet_size += 4;
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "MPEG picture header\n");
#endif
		}

		/* Slice header */
		while((header_size = slice_header(self->pointer+packet_size,
						  self->read_buffer + self->read_size - self->pointer -
						  packet_size)) != 0)
		{
		    packet_size += header_size;
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "MPEG slice header\n");
#endif
		}

		/* Audio frame */
		if(audio_header(self->pointer+packet_size, &packet_size, &self->frametime))
		{
		    stream_id = AUDIO_STREAMID;
		    self->stream_list[0]->streamid = stream_id;
		    self->stream_timestamp += self->frametime;
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "MPEG audio header [%d] time: %lf @ %lf\n",
			    packet_size, self->frametime, self->stream_timestamp);
#endif
		}
		else
		{
		    /* Check for next slice, picture, gop or sequence header */
		    register Uint8 * p;
		    register Uint8 c;

		    p = self->pointer + packet_size;
		state0:
		    c = *p;
		    p++;
		    if(p >= self->read_buffer + self->read_size) goto end;
		    if(c != 0) goto state0;
		    /* Not explicitly reached:
		     state1:
		     */
		    c = *p;
		    p++;
		    if(p >= self->read_buffer + self->read_size) goto end;
		    if(c != 0) goto state0;
		state2:
		    c = *p;
		    p++;
		    if(p >= self->read_buffer + self->read_size) goto end;
		    if(c == 0) goto state2;
		    if(c != 1) goto state0;
		    /* Not explicitly reached:
		     state3:
		     */
		    c = *p;
		    p++;
		    if(p >= self->read_buffer + self->read_size) goto end;
		    if(c <= 0xaf) goto end;
		    if(c == 0xb8) goto end;
		    if(c == 0xb3) goto end;
		    goto state0;
		end:

		    if(p >= self->read_buffer + self->read_size)
			packet_size = (self->read_buffer + self->read_size) - self->pointer;
		    else
			packet_size = p - self->pointer - 4;
		}

		if(stream_id == SYSTEM_STREAMID)
		    stream_id = 0;
	    }
	    else
	    {
#ifdef DEBUG_SYSTEM
		fprintf(stderr,
			"Warning: unexpected header %02x%02x%02x%02x at offset %d\n",
			self->pointer[0],
			self->pointer[1],
			self->pointer[2],
			self->pointer[3],
			self->read_total - self->read_size + (self->pointer - self->read_buffer));
#endif
		self->pointer++;
		self->stream_list[0]->pos++;
		MPEGsystem_seek_next_header(self);
		MPEGsystem_RequestBuffer(self);
		return(0);
	    }
	}

    if(MPEGsystem_Eof(self))
    {
	MPEGsystem_RequestBuffer(self);
	return(0);
    }

    assert(packet_size <= MPEG_BUFFER_SIZE);

    if(self->skip_timestamp > self->timestamp){
	int cur_seconds=(int)(self->timestamp)%60;

	if (cur_seconds%5==0){
	    fprintf(stderr, "Skipping to %02d:%02d (%02d:%02d)\r",
		    (int)(self->skip_timestamp)/60, (int)(self->skip_timestamp)%60,
		    (int)(self->timestamp)/60, cur_seconds);
	}
	self->pointer += packet_size;
	self->stream_list[0]->pos += packet_size;
	/* since we skip data, request more */
	MPEGsystem_RequestBuffer(self);
	return (0);
    }

    switch(stream_id)
    {
    case 0:
	/* Unknown stream, just get another packet */
	self->pointer += packet_size;
	self->stream_list[0]->pos += packet_size;
	MPEGsystem_RequestBuffer(self);
	return(0);

    case SYSTEM_STREAMID:
	/* System header */

	/* This MPEG contain system information */
	/* Parse the system header and create MPEG streams  */

	/* Read the stream table */
	self->pointer += 5;
	self->stream_list[0]->pos += 5;

	while (self->pointer[0] & 0x80 )
	{
	    /* If the stream doesn't already exist */
	    if(!MPEGsystem_get_stream(self, self->pointer[0]))
	    {
		/* Create a new stream and add it to the list */
//		MPEGsystem_add_stream(self, new MPEGstream(self, self->pointer[0]));
		MPEGsystem_add_stream(self, MPEGstream_init(NULL, self, self->pointer[0]));
	    }
	    self->pointer += 3;
	    self->stream_list[0]->pos += 3;
	}
	/* Hack to detect video streams that are not advertised */
	if ( ! MPEGsystem_exist_stream(self, VIDEO_STREAMID, 0xF0) ) {
	    if ( self->pointer[3] == 0xb3 ) {
		MPEGsystem_add_stream(self, MPEGstream_init(NULL, self, VIDEO_STREAMID));
	    }
	}
	MPEGsystem_RequestBuffer(self);
	return(stream_id);

    default:
	/* Look for the stream the data must be given to */
	stream = MPEGsystem_get_stream(self, stream_id);

	if(!stream)
	{
	    /* Hack to detect video or audio streams that are not declared in system header */
	    if ( ((stream_id & 0xF0) == VIDEO_STREAMID) && !MPEGsystem_exist_stream(self, stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
		fprintf(stderr, "Undeclared video packet, creating a new video stream\n");
#endif
		stream = MPEGstream_init(NULL, self, stream_id);
		MPEGsystem_add_stream(self, stream);
	    }
	    else
		if ( ((stream_id & 0xF0) == AUDIO_STREAMID) &&
		     !MPEGsystem_exist_stream(self, stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
		    fprintf(stderr, "Undeclared audio packet, creating a new audio stream\n");
#endif
		    stream = MPEGstream_init(NULL, self, stream_id);
		    MPEGsystem_add_stream(self, stream);
		}
		else
		{
		    /* No stream found for packet, skip it */
		    self->pointer += packet_size;
		    self->stream_list[0]->pos += packet_size;
		    MPEGsystem_RequestBuffer(self);
		    return(stream_id);
		}
	}

	/* Insert the new data at the end of the stream */
	if(self->pointer + packet_size <= self->read_buffer + self->read_size)
	{
//	    if(packet_size) stream->insert_packet(self->pointer, packet_size, self->stream_timestamp);
	    if(packet_size) MPEGstream_insert_packet(stream, self->pointer, packet_size, self->stream_timestamp);
	    self->pointer += packet_size;
	}
	else
	{
//	    stream->insert_packet(self->pointer, 0, self->stream_timestamp);
            MPEGstream_insert_packet(stream, self->pointer, 0, self->stream_timestamp);
	    self->errorstream = true;
	    self->pointer = self->read_buffer + self->read_size;
	}
	return(stream_id);
    }
}

void
MPEGsystem_Skip (_THIS, double time)
{
    if (self->skip_timestamp < self->timestamp)
	self->skip_timestamp = self->timestamp;
    self->skip_timestamp += time;
}
 
Uint32
MPEGsystem_Tell (_THIS)
{
    register Uint32 t;
    register int i;

    /* Sum all stream positions */
    for(i = 0, t = 0; self->stream_list[i]; i++)
	t += self->stream_list[i]->pos;

    if(t > MPEGsystem_TotalSize(self))
	return(MPEGsystem_TotalSize(self));
    else
	return(t);
}

Uint32
MPEGsystem_TotalSize (_THIS)
{
    off_t size;
    off_t pos;

    /* Lock to avoid concurrent access to the stream */
    SDL_mutexP(self->system_mutex);

    /* I made it this way (instead of fstat) to avoid #ifdef WIN32 everywhere */
    /* in case 'in some weird perversion of nature' someone wants to port this to Win32 :-) */
    if((pos = SDL_RWtell(self->source)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(0);
    }

    if((size = SDL_RWseek(self->source, 0, SEEK_END)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(0);
    }

    if((pos = SDL_RWseek(self->source, pos, SEEK_SET)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(0);
    }

    SDL_mutexV(self->system_mutex);
    return(size);
}

double
MPEGsystem_TotalTime (_THIS)
{
    off_t size, pos;
    off_t file_ptr;
    Uint8 * buffer, * p = 0;
    double time;
	Uint32 framesize;
	double frametime;
	Uint32 totalsize;

    /* Lock to avoid concurrent access to the stream */
    SDL_mutexP(self->system_mutex);

    /* Save current position */
    if((pos = SDL_RWtell(self->source)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(false);
    }

    file_ptr = 0;
//    buffer = new Uint8[MPEG_BUFFER_SIZE];
    buffer = (Uint8*)calloc(MPEG_BUFFER_SIZE, sizeof(Uint8));
    time = 0;

    /* If audio, compute total time according to bitrate of the first header and total size */
    /* Note: this doesn't work on variable bitrate streams */
    if(self->stream_list[0]->streamid == AUDIO_STREAMID)
    {
	do
	{
	    if((size = SDL_RWseek(self->source, file_ptr, SEEK_SET)) < 0)
	    {
		if(errno != ESPIPE)
		{
		    self->errorstream = true;
		    MPEGerror_SetError(self->error, strerror(errno));
		}
		SDL_mutexV(self->system_mutex);
		return(false);
	    }

	    if(SDL_RWread(self->source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;

	    /* Search for a valid audio header */
	    for(p = buffer; p < buffer + MPEG_BUFFER_SIZE; p++)
		if(audio_aligned(p, buffer + MPEG_BUFFER_SIZE - p)) break;

	    file_ptr += MPEG_BUFFER_SIZE;
	}
	while(p >= MPEG_BUFFER_SIZE + buffer);

	/* Extract time info from the first header */
	audio_header(p, &framesize, &frametime);
	totalsize = MPEGsystem_TotalSize(self);
	if(framesize)
	    time = frametime * totalsize / framesize;
    }
    else
    {
	bool last_chance = false;
	do
	{
	    /* Otherwise search the stream backwards for a valid header */
	    file_ptr -= MPEG_BUFFER_SIZE;
	    if ( file_ptr < -(Sint32)MPEGsystem_TotalSize(self) ) {
		last_chance = true;
		file_ptr = -(Sint32)MPEGsystem_TotalSize(self);
	    }

	    if((size = SDL_RWseek(self->source, file_ptr, SEEK_END)) < 0)
	    {
		if(errno != ESPIPE)
		{
		    self->errorstream = true;
		    MPEGerror_SetError(self->error, strerror(errno));
		}
		SDL_mutexV(self->system_mutex);
		return(false);
	    }

	    if(SDL_RWread(self->source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;

	    if(self->stream_list[0]->streamid == SYSTEM_STREAMID)
		for(p = buffer + MPEG_BUFFER_SIZE - 1; p >= buffer;)
		{
		    if(*p-- != 0xba) continue; // Packet header
		    if(*p-- != 1) continue;
		    if(*p-- != 0) continue;
		    if(*p-- != 0) continue;
		    ++p;
		    break;
		}
	    if(self->stream_list[0]->streamid == VIDEO_STREAMID)
		for(p = buffer + MPEG_BUFFER_SIZE - 1; p >= buffer;)
		{
		    if(*p-- != 0xb8) continue; // GOP header
		    if(*p-- != 1) continue;
		    if(*p-- != 0) continue;
		    if(*p-- != 0) continue;
		    ++p;
		    break;
		}
	}
	while( !last_chance && (p < buffer) );

	if ( p >= buffer ) {
	    /* Extract time info from the last header */
	    if(self->stream_list[0]->streamid == SYSTEM_STREAMID)
		packet_header(p, buffer + MPEG_BUFFER_SIZE - p, &time);

	    if(self->stream_list[0]->streamid == VIDEO_STREAMID)
		gop_header(p, buffer + MPEG_BUFFER_SIZE - p, &time);
	}
    }

//    delete[] buffer;
    free(buffer); buffer = NULL;

    /* Get back to saved position */
    if((pos = SDL_RWseek(self->source, pos, SEEK_SET)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	time = 0;
    }

    SDL_mutexV(self->system_mutex);

    return(time);
}

double
MPEGsystem_TimeElapsedAudio (_THIS, int atByte)
{
    off_t size, pos;
    off_t file_ptr;
    Uint8 * buffer, * p = 0;
    double time;
	Uint32 framesize;
	double frametime;
	Uint32 totalsize;

    if (atByte < 0)
    {
	return -1;
    }

    /* Lock to avoid concurrent access to the stream */
    SDL_mutexP(self->system_mutex);

    /* Save current position */
    if((pos = SDL_RWtell(self->source)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(false);
    }

    file_ptr = 0;
//    buffer = new Uint8[MPEG_BUFFER_SIZE];
    buffer = (Uint8*)calloc(MPEG_BUFFER_SIZE, sizeof(Uint8));

    /* If audio, compute total time according to bitrate of the first header and total size */
    /* Note: this doesn't work on variable bitrate streams */
    if(self->stream_list[0]->streamid == AUDIO_STREAMID)
    {
	do
	{
	    if((size = SDL_RWseek(self->source, file_ptr, SEEK_SET)) < 0)
	    {
		if(errno != ESPIPE)
		{
		    self->errorstream = true;
		    MPEGerror_SetError(self->error, strerror(errno));
		}
		SDL_mutexV(self->system_mutex);
		return(false);
	    }
    
	    if(SDL_RWread(self->source, buffer, 1, MPEG_BUFFER_SIZE) < 0) break;

	    /* Search for a valid audio header */
	    for(p = buffer; p < buffer + MPEG_BUFFER_SIZE; p++)
		if(audio_aligned(p, buffer + MPEG_BUFFER_SIZE - p)) break;

	    file_ptr += MPEG_BUFFER_SIZE;
	}
	while(p >= MPEG_BUFFER_SIZE + buffer);

	/* Extract time info from the first header */
	audio_header(p, &framesize, &frametime);
	totalsize = MPEGsystem_TotalSize(self);
	if(framesize)
	    //is there a better way to do this?
	    time = (frametime * (atByte ? atByte:totalsize)) / framesize;
	else
	    time = 0;
    }
    else
	//This is not a purely audio stream. This doesn't make sense!
    {
	time = -1;
    }

//    delete buffer;
    free(buffer); buffer = NULL;

    /* Get back to saved position */
    if((pos = SDL_RWseek(self->source, pos, SEEK_SET)) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	SDL_mutexV(self->system_mutex);
	return(0);
    }

    SDL_mutexV(self->system_mutex);
    return(time);
}

void
MPEGsystem_Rewind (_THIS)
{
    MPEGsystem_Seek(self, 0);
}

bool
MPEGsystem_Seek (_THIS, int length)
{
    /* Stop the system thread */
    MPEGsystem_Stop(self);

    /* Lock to avoid concurrent access to the stream */
    SDL_mutexP(self->system_mutex);

    /* Get into the stream */
    if(SDL_RWseek(self->source, length, SEEK_SET) < 0)
    {
	if(errno != ESPIPE)
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, strerror(errno));
	}
	return(false);
    }

    /* Reinitialize the read buffer */
    self->pointer = self->read_buffer;
    self->read_size = 0;
    self->read_total = length;
    self->stream_list[0]->pos += length;
    self->packet_total = 0;
    self->endofstream = false;
    self->errorstream = false;
    self->timestamp = 0.0;
    self->skip_timestamp = -1;
    MPEGsystem_reset_all_streams(self);

    SDL_mutexV(self->system_mutex);

    /* Restart the system thread */
    MPEGsystem_Start(self);

    return(true);
}

void
MPEGsystem_RequestBuffer (_THIS)
{
    SDL_SemPost(self->request_wait);
}

void
MPEGsystem_Start (_THIS)
{
    if(self->system_thread_running) return;

    /* Get the next header */
    if(!MPEGsystem_seek_next_header(self))
    {
	if(!MPEGsystem_Eof(self))
	{
	    self->errorstream = true;
	    MPEGerror_SetError(self->error, "Could not find the beginning of MPEG data\n");
	}
    }

#ifdef USE_SYSTEM_THREAD
    /* Start the system thread */
    self->system_thread = SDL_CreateThread(MPEGsystem_SystemThread, self);

    /* Wait for the thread to start */
    while(!self->system_thread_running && !MPEGsystem_Eof(self))
	SDL_Delay(1);
#else
    self->system_thread_running = true;
#endif
}

void
MPEGsystem_Stop (_THIS)
{
    if(!self->system_thread_running) return;

    /* Force the system thread to die */
    self->system_thread_running = false;
#ifdef USE_SYSTEM_THREAD
    SDL_SemPost(self->request_wait);
    SDL_WaitThread(self->system_thread, NULL);
#endif

    /* Reset the streams */
    MPEGsystem_reset_all_streams(self);
}

bool
MPEGsystem_Wait (_THIS)
{
#ifdef USE_SYSTEM_THREAD
    if ( ! self->errorstream ) {
	while(SDL_SemValue(self->request_wait) != 0)
	    SDL_Delay(1);
    }
#else
    while(SDL_SemValue(self->request_wait) != 0)
	if ( ! MPEGsystem_SystemLoop(self) ) break;
#endif
    return(!self->errorstream);
}

bool
MPEGsystem_Eof (_THIS) //const
{
    return(self->errorstream || self->endofstream);
}

bool
MPEGsystem_SystemLoop (MPEGsystem *system)
{
    /* Check for end of file */
    if(MPEGsystem_Eof(system))
    {
	/* Set the eof mark on all streams */
	MPEGsystem_end_all_streams(system);

	/* Get back to the beginning of the stream if possible */
	if(SDL_RWseek(system->source, 0, SEEK_SET) < 0)
	{
	    if(errno != ESPIPE)
	    {
		system->errorstream = true;
		MPEGerror_SetError(system->error, strerror(errno));
	    }
	    return(false);
	}

	/* Reinitialize the read buffer */
	system->pointer = system->read_buffer;
	system->read_size = 0;
	system->read_total = 0;
	system->packet_total = 0;
	system->endofstream = false;
	system->errorstream = false;

	/* Get the first header */
	if(!MPEGsystem_seek_first_header(system))
	{
	    system->errorstream = true;
	    MPEGerror_SetError(system->error, "Could not find the beginning of MPEG data\n");
	    return(false);
	}
    }

    /* Wait for a buffer request */
    SDL_SemWait(system->request_wait);

    /* Read the buffer */
    MPEGsystem_FillBuffer(system);

    return(true);
}

int
MPEGsystem_SystemThread (void * udata)
{
    MPEGsystem * system = (MPEGsystem *) udata;

    system->system_thread_running = true;

    while(system->system_thread_running)
    {
	if ( ! MPEGsystem_SystemLoop(system) ) {
	    system->system_thread_running = false;
	}
    }
    return(true);
}

void
MPEGsystem_add_stream (_THIS, MPEGstream * stream)
{
    register int i;

    /* Go to the end of the list */
    for(i = 0; self->stream_list[i]; i++);

    /* Resize list */
    self->stream_list =
	(MPEGstream **) realloc(self->stream_list, (i+2)*sizeof(MPEGstream *));

    /* Write the stream */
    self->stream_list[i] = stream;

    /* Put the end marker (null) */
    self->stream_list[i+1] = 0;
}

MPEGstream *
MPEGsystem_get_stream (_THIS, Uint8 stream_id)
{
    register int i;

    for(i = 0; self->stream_list[i]; i++)
	if(self->stream_list[i]->streamid == stream_id)
	    break;

    return(self->stream_list[i]);
}

Uint8
MPEGsystem_exist_stream (_THIS, Uint8 stream_id, Uint8 mask)
{
    register int i;

    for(i = 0; self->stream_list[i]; i++)
	if(((self->stream_list[i]->streamid) & mask) == (stream_id & mask))
	    return(self->stream_list[i]->streamid);

    return(0);
}

void
MPEGsystem_reset_all_streams (_THIS)
{
    register int i;

    /* Reset the streams */
    for(i = 0; self->stream_list[i]; i++)
//	self->stream_list[i]->reset_stream();
        MPEGstream_reset_stream(self->stream_list[i]);
}

void
MPEGsystem_end_all_streams (_THIS)
{
    register int i;

    /* End the streams */
    /* We use a null buffer as the end of stream marker */
    for(i = 0; self->stream_list[i]; i++)
//	self->stream_list[i]->insert_packet(0, 0);
        MPEGstream_insert_packet(self->stream_list[i], 0, 0, -1);
}

bool
MPEGsystem_seek_first_header (_THIS)
{
    MPEGsystem_Read(self);

    if(MPEGsystem_Eof(self))
	return(false);

    while(!(audio_aligned(self->pointer, self->read_buffer + self->read_size - self->pointer) ||
	    system_aligned(self->pointer, self->read_buffer + self->read_size - self->pointer) ||
	    Match4(self->pointer, VIDEO_CODE, VIDEO_MASK)))
    {
	++self->pointer;
	self->stream_list[0]->pos++;
	/* Make sure buffer is always full */
	MPEGsystem_Read(self);

	if(MPEGsystem_Eof(self))
	    return(false);
    }
    return(true);
}

bool
MPEGsystem_seek_next_header (_THIS)
{
    MPEGsystem_Read(self);

    if(MPEGsystem_Eof(self))
	return(false);

    while(!( (self->stream_list[0]->streamid == AUDIO_STREAMID &&
	      audio_aligned(self->pointer, self->read_buffer + self->read_size - self->pointer)) ||
	     (self->stream_list[0]->streamid == SYSTEM_STREAMID &&
	      system_aligned(self->pointer, self->read_buffer + self->read_size - self->pointer)) ||
	     (self->stream_list[0]->streamid == VIDEO_STREAMID &&
	      Match4(self->pointer, GOP_CODE, GOP_MASK))
	   ) )
    {
	++self->pointer;
	self->stream_list[0]->pos++;
	/* Make sure buffer is always full */
	MPEGsystem_Read(self);

	if(MPEGsystem_Eof(self))
	    return(false);
    }

    return(true);
}
