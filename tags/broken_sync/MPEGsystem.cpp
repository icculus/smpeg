#include <stdlib.h>        /* for realloc() */
#include <string.h>        /* for memmove() */
#include <errno.h>
#include <assert.h>
#ifdef WIN32
#include <io.h>
#include <winsock.h>
#else
#include <unistd.h>
#endif

#include "MPEGsystem.h"
#include "MPEGstream.h"

Uint8 const PACKET_CODE[]       = { 0x00, 0x00, 0x01, 0xba };
Uint8 const PACKET_MASK[]       = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END_CODE[]          = { 0x00, 0x00, 0x01, 0xb9 };
Uint8 const END_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const END2_CODE[]         = { 0x00, 0x00, 0x01, 0xb7 };
Uint8 const END2_MASK[]         = { 0xff, 0xff, 0xff, 0xff };
Uint8 const VIDEO_CODE[]        = { 0x00, 0x00, 0x01, 0xb3 };
Uint8 const VIDEO_MASK[]        = { 0xff, 0xff, 0xff, 0xff };
Uint8 const AUDIO_CODE[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const AUDIO_MASK[]        = { 0xff, 0xf0, 0x00, 0x00 };
Uint8 const VIDEOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xe0 };
Uint8 const VIDEOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xe0 };
Uint8 const AUDIOSTREAM_CODE[]  = { 0x00, 0x00, 0x01, 0xc0 };
Uint8 const AUDIOSTREAM_MASK[]  = { 0xff, 0xff, 0xff, 0xc0 };
Uint8 const PADSTREAM_CODE[]    = { 0x00, 0x00, 0x01, 0xbe };
Uint8 const PADSTREAM_MASK[]    = { 0xff, 0xff, 0xff, 0xff };
Uint8 const SYSTEMSTREAM_CODE[] = { 0x00, 0x00, 0x01, 0xbb };
Uint8 const SYSTEMSTREAM_MASK[] = { 0xff, 0xff, 0xff, 0xff };
Uint8 const GOP_CODE[]          = { 0x00, 0x00, 0x01, 0xb8 };
Uint8 const GOP_MASK[]          = { 0xff, 0xff, 0xff, 0xff };
Uint8 const USER_CODE[]         = { 0x00, 0x00, 0x01, 0xb2 };
Uint8 const USER_MASK[]         = { 0xff, 0xff, 0xff, 0xff };
Uint8 const PICTURE_CODE[]      = { 0x00, 0x00, 0x01, 0x00 };
Uint8 const PICTURE_MASK[]      = { 0xff, 0xff, 0xff, 0x00 };

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

/* This defines the maximum data that can be preread */
/* It is to prevent filling the whole memory with buffers on systems */
/* where read is immediate such as in the case of files */
#define PRE_BUFFERED_MAX (256 * 1024)

/* Timeout before read fails */
#define READ_TIME_OUT 1000000

/* This work only on little endian systems */
/*
#define REV(x) ((((x)&0x000000FF)<<24)| \
                (((x)&0x0000FF00)<< 8)| \
                (((x)&0x00FF0000)>> 8)| \
                (((x)&0xFF000000)>>24))
#define MATCH4(x, y, m) (((x) & REV(m)) == REV(y))
*/

/* Match two 4-byte codes */
static inline bool Match4(Uint8 const code1[4], Uint8 const code2[4], Uint8 const mask[4])
{
  return( ((code1[0] & mask[0]) == (code2[0] & mask[0])) &&
	  ((code1[1] & mask[1]) == (code2[1] & mask[1])) &&
	  ((code1[2] & mask[2]) == (code2[2] & mask[2])) &&
	  ((code1[3] & mask[3]) == (code2[3] & mask[3])) );
}

MPEGsystem::MPEGsystem(int Mpeg_FD)
{
  mpeg_fd = Mpeg_FD;

  /* Create a new buffer for reading */
  read_buffer = new Uint8[MPEG_BUFFER_SIZE];

  /* Invalidate the read buffer */
  pointer = read_buffer;
  read_size = 0;
  read_total = 0;
  packet_total = 0;
  endofstream = errorstream = false;
  looping = false;

  /* Create an empty stream list */
  stream_list = 
    (MPEGstream **) malloc(sizeof(MPEGstream *));
  stream_list[0] = 0;

  /* Create the system stream and add it to the list */
  if(!get_stream(SYSTEM_STREAMID))
    add_stream(new MPEGstream(this, SYSTEM_STREAMID));

#ifdef USE_SYSTEM_TIMESTAMP
  timestamp = 0.0;
  timedrift = 0.0;
  skip_timestamp = 0.0;
#endif

  /* Search the MPEG for the first header */
  if(!seek_next_header())
  {
    errorstream = true;
    SetError("Could not find the beginning of MPEG data\n");
    return;
  }

  request = PRE_BUFFERED_MAX;

  /* Start the system thread */
  system_thread_running = false;
  system_thread = SDL_CreateThread(SystemThread, this);

  /* Wait for the thread to start */
  while(!system_thread_running && !Eof())
    SDL_Delay(1);

  /* Wait for prebuffering */
  while(request > 0 && !Eof())
    SDL_Delay(1);

  /* Look for streams */
  do
    RequestBuffer();
  while(!exist_stream(VIDEO_STREAMID, 0xF0) &&
	!exist_stream(AUDIO_STREAMID, 0xF0) &&
	!Eof());

  /* Wait for prebuffering */
  while(request > 0 && !Eof())
    SDL_Delay(1);
}

MPEGsystem::~MPEGsystem()
{
  MPEGstream ** list;

  /* Kill the system thread */
  system_thread_running = false;
  SDL_WaitThread(system_thread, NULL);

  /* Delete the streams */
  for(list = stream_list; *list; list ++)
    delete *list;

  free(stream_list);
}

MPEGstream ** MPEGsystem::GetStreamList()
{
  return(stream_list);
}

void MPEGsystem::Read()
{
  int remaining;
  int timeout;

  timeout = READ_TIME_OUT;
  remaining = read_buffer + read_size - pointer;

  /* Only read data if buffer is rather empty */
  if(remaining < MPEG_BUFFER_SIZE / 2)
  {
    if(remaining < 0)
    {
      /* Hum.. we'd better stop if we have already read past the buffer size */
      errorstream = true;
      return;
    }

    /* Replace unread data at the beginning of the stream */
    memmove(read_buffer, pointer, remaining);

#ifndef WIN32
    /* Wait for new data */
    fd_set fdset;
    do {
       FD_ZERO(&fdset);
       FD_SET(mpeg_fd, &fdset);
    } while ( select(mpeg_fd+1, &fdset, NULL, NULL, NULL) < 0 );
#endif

    /* Read new data */
    read_size = 
      read(mpeg_fd, read_buffer + remaining, 
	   READ_ALIGN(MPEG_BUFFER_SIZE - remaining));

    if(read_size < 0)
    {
      perror("Read");
      errorstream = true;
      return;
    }
    
    read_total += read_size;
    request -= read_size;

    packet_total ++;

    if((MPEG_BUFFER_SIZE - remaining) != 0 && read_size <= 0)
    {
      if(read_size != 0)
      {
	errorstream = true;
	return;
      }
    }

    read_size += remaining;

    if(read_size == 0)
    {
      /* There is no more data */
      endofstream = true;
      return;
    }

    /* Move the pointer */
    pointer = read_buffer;  
  }

}

/* ASSUME: stream_list[0] = system stream */
/*         packet length < MPEG_BUFFER_SIZE */
Uint8 MPEGsystem::FillBuffer()
{
  Uint8 stream_id;
  Uint32 packet_size;
  Uint8 const zero[4] = {0x00,0x00,0x00,0x00};
  Uint8 const one[4]  = {0x00,0x00,0x00,0x01};
  Uint8 const mask[4] = {0xff,0xff,0xff,0xff};

  /* - Read a new packet - */
  Read();

  if(Eof()) 
    return(0);

  /* If there is only audio or video information give the packet */
  /* without parsing it */
  if(((stream_list[0]->streamid & 0xc0) == 0xc0) || /* audio only stream */
     ((stream_list[0]->streamid & 0xe0) == 0xe0))   /* video only stream */
  {
    packet_size = read_buffer + read_size - pointer;

    /* Insert the new data at the end of the stream */
    stream_list[0]->insert_packet(pointer, packet_size);
    pointer += packet_size;
    return(stream_list[0]->streamid);
  }

  /* Skip possible zeros at the beggining of the packet */
  while(Match4(pointer, zero, mask))
  {
    pointer++;
    Read();
    if(Eof())
      return(0);

    if(Match4(pointer, one, mask))
      pointer++;
  }

  /* Parse the packet header */
  if(Match4(pointer, PACKET_CODE, PACKET_MASK))
  {
    /* Parse the packet information */
    pointer += 4;

    /* The system stream timestamp is not very useful to us since we
       don't coordinate the audio and video threads very tightly, and
       we read in large amounts of data at once in the video stream.
       BUT... if you need it, here it is.
    */

#ifdef USE_SYSTEM_TIMESTAMP
#define FLOAT_0x10000 (double)((Uint32)1 << 16)
#define STD_SYSTEM_CLOCK_FREQ 90000L
    { 
      Uint8 hibit; Uint32 lowbytes;

      hibit = (pointer[0]>>3)&0x01;
      lowbytes = (((Uint32)pointer[0] >> 1) & 0x03) << 30;
      lowbytes |= (Uint32)pointer[1] << 22;
      lowbytes |= ((Uint32)pointer[2] >> 1) << 15;
      lowbytes |= (Uint32)pointer[3] << 7;
      lowbytes |= ((Uint32)pointer[4]) >> 1;
      timestamp = (double)hibit*FLOAT_0x10000*FLOAT_0x10000+(double)lowbytes;
      timestamp /= STD_SYSTEM_CLOCK_FREQ;
    }
#endif
    pointer += 8;
  }

  /* Parse the stream packet */
  if(Match4(pointer, SYSTEMSTREAM_CODE, SYSTEMSTREAM_MASK) ||
     Match4(pointer, PADSTREAM_CODE, PADSTREAM_MASK) ||
     Match4(pointer, AUDIOSTREAM_CODE, AUDIOSTREAM_MASK) ||
     Match4(pointer, VIDEOSTREAM_CODE, VIDEOSTREAM_MASK))
  {
    /* Get the stream id, and packet length */
    stream_id = pointer[3];
    pointer += 4;
    packet_size = (((unsigned short) pointer[0] << 8) | pointer[1]);
    pointer += 2;

    /* Skip stuffing bytes */
    while ( pointer[0] == 0xff ) {
      ++pointer;
      --packet_size;
    }
    if ( (pointer[0] & 0x40) == 0x40 ) {
      pointer += 2;
      packet_size -= 2;
    }
    if ( (pointer[0] & 0x30) == 0x30 ) {
      pointer += 9;
      packet_size -= 9;
    } else if ( (pointer[0] & 0x20) == 0x20 ) {
      pointer += 4;
      packet_size -= 4;
    } else if ( pointer[0] != 0x0f && pointer[0] != 0x80) {
      /* Huh? there is something wrong, try to catch next header */
      pointer++;
      if(!seek_next_header())
      	errorstream = true;
      return(0);
    }
    ++pointer;
    --packet_size;

    if(read_buffer + read_size - pointer < 0)
    {
      errorstream = true;
      return(0);
    }
  }
  else
  if(Match4(pointer, END_CODE, END_MASK) ||
     Match4(pointer, END2_CODE, END2_MASK))
  {
    /* End codes belong to video stream */
    stream_id = exist_stream(VIDEO_STREAMID, 0xF0);
    packet_size = 4;
  }
  else
  {
    stream_id = stream_list[0]->streamid;

    if(!stream_list[1])
    {
      /* There is no system info in the stream */

      /* If we're still a system stream, morph to an audio */
      /* or video stream */

      if(Match4(pointer, AUDIO_CODE, AUDIO_MASK) &&
	 (pointer[2] & 0xf0) != 0xf0 &&
	 (pointer[2] & 0xf0) != 0x00) /* Check for a valid mpeg audio header : bitrate != 0 */
      {
	stream_id = AUDIO_STREAMID;
	stream_list[0]->streamid = stream_id;
      }
      if(Match4(pointer, VIDEO_CODE, VIDEO_MASK))
      {
	stream_id = VIDEO_STREAMID;
	stream_list[0]->streamid = stream_id;
      }

      if(stream_id == SYSTEM_STREAMID)
	stream_id = 0;

      packet_size = read_buffer + read_size - pointer;
    }
    else
    {
#ifdef DEBUG_SYSTEM
	fprintf(stderr,
		"Warning: unexpected header %02x%02x%02x%02x at offset %d\n",
		pointer[0],
		pointer[1],
		pointer[2],
		pointer[3],
		Tell() - read_size + (pointer - read_buffer));
#endif
	pointer++;
	seek_next_header();
	return(0);
    }
  }

  if(Eof())
    return(0);

  assert(packet_size <= MPEG_BUFFER_SIZE);

  if(skip_timestamp > timestamp){
    int cur_seconds=int(timestamp)%60;

    if (cur_seconds%5==0){
      fprintf(stderr, "Skiping to %02d:%02d (%02d:%02d)\r",
              int(skip_timestamp)/60, int(skip_timestamp)%60,
              int(timestamp)/60, cur_seconds);
    }
    pointer += packet_size;
    /* since we skip data, request more */
    RequestBuffer();
    return (1);
  }
  switch(stream_id)
  {
    case 0:
      /* Unknown stream, just get another packet */
      pointer += packet_size;
    return(stream_id);

    case SYSTEM_STREAMID:
      /* System header */

      /* This MPEG contain system information */
      /* Parse the system header and create MPEG streams  */
    
      /* Read the stream table */
      pointer += 5;

      while (pointer[0] & 0x80 )
      {
	/* If the stream doesn't already exist */
	if(!get_stream(pointer[1]))
	{
	  /* Create a new stream and add it to the list */
	  add_stream(new MPEGstream(this, pointer[1]));
	}
	pointer += 3;
      }          
      /* Hack to detect video streams that are not advertised */
      if ( ! exist_stream(VIDEO_STREAMID, 0xF0) ) {
	if ( pointer[3] == 0xb3 ) {
	  add_stream(new MPEGstream(this, VIDEO_STREAMID));
	}
      }
    return(stream_id);

    default:
      MPEGstream * stream;

      /* Look for the stream the data must be given to */
      stream = get_stream(stream_id);

      if(!stream)
      {	
	/* Hack to detect video or audio streams that are not declared in system header */
	if ( ((stream_id & 0xF0) == VIDEO_STREAMID) && !exist_stream(stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
	  fprintf(stderr, "Undeclared video packet, creating a new video stream\n");
#endif
	    stream = new MPEGstream(this, stream_id);
	    add_stream(stream);
	}
	else
	if ( ((stream_id & 0xF0) == AUDIO_STREAMID) && !exist_stream(stream_id, 0xFF) ) {
#ifdef DEBUG_SYSTEM
	  fprintf(stderr, "Undeclared audio packet, creating a new audio stream\n");
#endif
	    stream = new MPEGstream(this, stream_id);
	    add_stream(stream);
	}
	else
	{
	  /* No stream found for packet, skip it */
	  pointer += packet_size;
	  return(stream_id);
	}
      }

      /* Insert the new data at the end of the stream */
      stream->insert_packet(pointer, packet_size, timestamp);
      pointer += packet_size;
    return(stream_id);
  }
}
void MPEGsystem::Skip(double time)
{
  if (skip_timestamp < timestamp)
    skip_timestamp = timestamp;
  skip_timestamp += time;
}

Uint32 MPEGsystem::Tell()
{
  /* Warning: 32 bits means that files > 4Go might return bad values */
  return(read_total);
}

void MPEGsystem::Rewind()
{
  Seek(0);
}

double MPEGsystem::Seek(int length)
{
  request = 0;

  /* Force the system thread to die */
  system_thread_running = false;
  SDL_WaitThread(system_thread, NULL);

  /* Reset the streams */
  reset_all_streams();

  /* Get into the stream */
  if(lseek(mpeg_fd, length, SEEK_SET) == (off_t) -1)
  {
    if(errno != ESPIPE)
    {
      errorstream = true;
      SetError(strerror(errno));
    }
    return (0);
  }

  /* Reinitialize the read buffer */
  pointer = read_buffer;
  read_size = 0;
  read_total = 0;
  packet_total = 0;
  endofstream = false;
  errorstream = false;

  /* Get the first header */
  if(!seek_next_header())
  {
    errorstream = true;
    SetError("Could not find the beginning of MPEG data\n");
    return (0);
  }

  request = PRE_BUFFERED_MAX;

  /* Start the system thread */
  system_thread = SDL_CreateThread(SystemThread, this);

  /* Wait for the thread to start */
  while(!system_thread_running && !Eof())
    SDL_Delay(1);

  /* Wait for prebuffering */
  while(request > 0 && !Eof())
    SDL_Delay(1);

  /* Get current play time */
  FillBuffer();

  return(timestamp);
}

void MPEGsystem::Loop(bool toggle)
{
    looping = toggle;
    loop_all_streams(looping);
}

void MPEGsystem::RequestBuffer()
{
  if(request < PRE_BUFFERED_MAX)
    request += MPEG_BUFFER_SIZE;
}

bool MPEGsystem::Eof() const
{
  return(errorstream || endofstream);
}

int MPEGsystem::SystemThread(void * udata)
{
  MPEGsystem * system = (MPEGsystem *) udata;

#ifndef WIN32
  /* Set low priority */
  nice(1);
#endif

  system->system_thread_running = true;

  while(system->system_thread_running)
  {
    int delay = 1;

    /* Check for end of file */
    if(system->Eof())
    {
      /* Set the eof mark on all streams */
      system->end_all_streams();

      /* Get back to the beginning of the stream if possible */
      if(lseek(system->mpeg_fd, 0, SEEK_SET) == (long) -1)
      {
	if(errno != ESPIPE)
	{
	  system->errorstream = true;
	  system->SetError(strerror(errno));
	}
	break;
      }

      /* Reinitialize the read buffer */
      system->pointer = system->read_buffer;
      system->read_size = 0;
      system->read_total = 0;
      system->packet_total = 0;
      system->endofstream = false;
      system->errorstream = false;

      /* Get the first header */
      if(!system->seek_next_header())
      {
	system->errorstream = true;
	system->SetError("Could not find the beginning of MPEG data\n");
	break;
      }
    }

    /* Is a buffer needed? */
    if(system->request > 0)
    {
      /* Read the buffer */
      while (system->FillBuffer()==1) {};
      delay >>= 1;
    }
    else
    {
      /* Wait more and more time to avoid take too much cpu time when */
      /* there are no packets requested */
      if(delay >= 100) delay = 100;
      SDL_Delay(delay++);
    }
  }
  system->system_thread_running = false;

  return(true);
}

void MPEGsystem::add_stream(MPEGstream * stream)
{
  register int i;

  /* Go to the end of the list */
  for(i = 0; stream_list[i]; i++);

  /* Resize list */
  stream_list = 
    (MPEGstream **) realloc(stream_list, (i+2)*sizeof(MPEGstream *));

  /* Write the stream */
  stream_list[i] = stream;

  /* Set the looping flag for that stream */
  stream->loop(looping);
  
  /* Put the end marker (null) */
  stream_list[i+1] = 0;
}

MPEGstream * MPEGsystem::get_stream(Uint8 stream_id)
{
  register int i;

  for(i = 0; stream_list[i]; i++)
    if(stream_list[i]->streamid == stream_id)
      break;

  return(stream_list[i]);
}

Uint8 MPEGsystem::exist_stream(Uint8 stream_id, Uint8 mask)
{
  register int i;

  for(i = 0; stream_list[i]; i++)
    if(((stream_list[i]->streamid) & mask) == (stream_id & mask))
      return(stream_list[i]->streamid);

  return(0);
}

void MPEGsystem::reset_all_streams()
{
  register int i;

  /* Reset the streams */
  for(i = 0; stream_list[i]; i++)
    stream_list[i]->reset_stream();
}

void MPEGsystem::end_all_streams()
{
  register int i;

  /* End the streams */
  /* We use a null buffer as the end of stream marker */
  for(i = 0; stream_list[i]; i++)
    stream_list[i]->insert_packet(0, 0);
}

void MPEGsystem::loop_all_streams(bool toggle)
{
  register int i;

  /* Set loop flag on all streams */
  for(i = 0; stream_list[i]; i++)
    stream_list[i]->loop(toggle);
}

bool MPEGsystem::seek_next_header()
{
  Read();

  if(Eof())
    return(false);

  while(!(Match4(pointer, PACKET_CODE, PACKET_MASK) ||
	  Match4(pointer, VIDEO_CODE, VIDEO_MASK) ||
	  (Match4(pointer, AUDIO_CODE, AUDIO_MASK) &&
	   /* Check for a valid mpeg audio header : bitrate != 0 */ 
	   ((pointer[2] & 0xf0) != 0xf0) && ((pointer[2] & 0xf0) != 0)) ||
	  Match4(pointer, END_CODE, END_MASK) ||
	  Match4(pointer, VIDEOSTREAM_CODE, VIDEOSTREAM_MASK) ||
	  Match4(pointer, AUDIOSTREAM_CODE, AUDIOSTREAM_MASK) ||
	  Match4(pointer, GOP_CODE, GOP_MASK) ||
	  Match4(pointer, USER_CODE, USER_MASK) ||
	  Match4(pointer, PICTURE_CODE, PICTURE_MASK)))
  {
       ++pointer;

      /* Make sure buffer is always full */
      Read();

      if(Eof())
	return(false);
  }
  return(true);
}
