/* A class based on the MPEG stream class, used to parse the system stream */
    
/* - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn - */

#ifndef _MPEGSYSTEM_H_
#define _MPEGSYSTEM_H_
#define USE_SYSTEM_TIMESTAMP

#include "SDL.h"
#include "SDL_thread.h"
#include "MPEGerror.h"

//class MPEGstream;
struct MPEGstream;

/* MPEG System library
   by Vivien Chappelier */

/* The system class is necessary for splitting the MPEG stream into */
/* peaces of data that will be sent to the audio or video decoder.  */

#if 0
class MPEGsystem : public MPEGerror
{
public:
	/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    MPEGsystem() {}
    MPEGsystem(SDL_RWops *mpeg_source);
    virtual ~MPEGsystem();

    /* Buffered I/O functions */
    void RequestBuffer();
    bool Wait();
    Uint32 Tell();
    void Rewind();
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    virtual void Start();
    void Stop();
    bool Eof() const;
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    virtual bool Seek(int length);
    virtual Uint32 TotalSize();
    virtual double TotalTime();
    virtual double TimeElapsedAudio(int atByte);

    /* Skip "seconds" seconds */
    void Skip(double seconds);

    /* Create all the streams present in the MPEG */
    struct MPEGstream ** GetStreamList();

    /* Insert a stream in the list */
    void add_stream(struct MPEGstream * stream);

    /* Search for a stream in the list */
    struct MPEGstream * get_stream(Uint8 stream_id);

    /* Test if a stream is in the list */
    Uint8 exist_stream(Uint8 stream_id, Uint8 mask);

    /* Reset all the system streams */
    void reset_all_streams();
    
    /* Set eof for all streams */
    void end_all_streams();
    
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    /* Seek the first header */
    virtual bool seek_first_header();

		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    /* Seek the next header */
    virtual bool seek_next_header();

protected:
    /* Run the loop to fill the stream buffers */
    static bool SystemLoop(MPEGsystem *system);

		/* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
    /* Fill a buffer */
    virtual Uint8 FillBuffer();

    /* Read a new packet */
    virtual void Read();

    /* The system thread which fills the FIFO */
    static int SystemThread(void * udata);

    SDL_RWops *source;

    SDL_Thread * system_thread;
    bool system_thread_running;

    struct MPEGstream ** stream_list;

    Uint8 * read_buffer;
    Uint8 * pointer;
    int read_size;
    Uint32 read_total;
    Uint32 packet_total;
    int request;
    SDL_semaphore * request_wait;
    SDL_mutex * system_mutex;

    bool endofstream;
    bool errorstream;

    double frametime;
    double stream_timestamp;

#ifdef USE_SYSTEM_TIMESTAMP
    /* Current timestamp for this stream */
    double timestamp;
    double timedrift;
    double skip_timestamp;
#endif
};
#endif /* 0 */



struct MPEGsystem {
  MPEGerror *error;

//protected:
  /* Run the loop to fill the stream buffers */
//  static bool SystemLoop(MPEGsystem *system);

  /* Michel Darricau from eProcess <mdarricau@eprocess.fr>  need for override in popcorn */
  /* Fill a buffer */
//  virtual Uint8 FillBuffer();

  /* Read a new packet */
//  virtual void Read();

  /* The system thread which fills the FIFO */
//  static int SystemThread(void * udata);

  SDL_RWops *source;

  SDL_Thread * system_thread;
  bool system_thread_running;

  struct MPEGstream ** stream_list;

  Uint8 * read_buffer;
  Uint8 * pointer;
  int read_size;
  Uint32 read_total;
  Uint32 packet_total;
  int request;
  struct SDL_semaphore * request_wait;
  struct SDL_mutex * system_mutex;

  bool endofstream;
  bool errorstream;

  double frametime;
  double stream_timestamp;

#ifdef USE_SYSTEM_TIMESTAMP
  /* Current timestamp for this stream */
  double timestamp;
  double timedrift;
  double skip_timestamp;
#endif

};

typedef struct MPEGsystem MPEGsystem;


#undef _THIS
#define _THIS MPEGsystem *self


/* Run the loop to fill the stream buffers */
bool MPEGsystem_SystemLoop (MPEGsystem *system);
/* Fill a buffer */
/*virtual*/ Uint8 MPEGsystem_FillBuffer (_THIS);
/* Read a new packet */
/*virtual*/ void MPEGsystem_Read (_THIS);
/* The system thread which fills the FIFO */
int MPEGsystem_SystemThread (void * udata);


MPEGsystem * MPEGsystem_init (_THIS);
MPEGsystem * MPEGsystem_init_mpeg_source (_THIS, SDL_RWops *mpeg_source);
void MPEGsystem_destroy (_THIS);

/* Buffered I/O functions */
void MPEGsystem_RequestBuffer (_THIS);
bool MPEGsystem_Wait (_THIS);
Uint32 MPEGsystem_Tell (_THIS);
void MPEGsystem_Rewind (_THIS);
/*virtual*/ void MPEGsystem_Start (_THIS);
void MPEGsystem_Stop (_THIS);
bool MPEGsystem_Eof (_THIS); //const;
/*virtual*/ bool MPEGsystem_Seek (_THIS, int length);
/*virtual*/ Uint32 MPEGsystem_TotalSize (_THIS);
/*virtual*/ double MPEGsystem_TotalTime (_THIS);
/*virtual*/ double MPEGsystem_TimeElapsedAudio (_THIS, int atByte);

/* Skip "seconds" seconds */
void MPEGsystem_Skip (_THIS, double seconds);

/* Create all the streams present in the MPEG */
struct MPEGstream ** MPEGsystem_GetStreamList (_THIS);

/* Insert a stream in the list */
void MPEGsystem_add_stream (_THIS, struct MPEGstream * stream);

/* Search for a stream in the list */
struct MPEGstream * MPEGsystem_get_stream (_THIS, Uint8 stream_id);

/* Test if a stream is in the list */
Uint8 MPEGsystem_exist_stream (_THIS, Uint8 stream_id, Uint8 mask);

/* Reset all the system streams */
void MPEGsystem_reset_all_streams (_THIS);

/* Set eof for all streams */
void MPEGsystem_end_all_streams (_THIS);

/* Seek the first header */
/*virtual*/ bool MPEGsystem_seek_first_header (_THIS);

/* Seek the next header */
/*virtual*/ bool MPEGsystem_seek_next_header (_THIS);





#endif /* _MPEGSYSTEM_H_ */
