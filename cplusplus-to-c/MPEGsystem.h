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
#undef METH
#define METH(m) MPEGsystem_##m


/* Run the loop to fill the stream buffers */
bool METH(SystemLoop) (MPEGsystem *system);
/* Fill a buffer */
/*virtual*/ Uint8 METH(FillBuffer) (_THIS);
/* Read a new packet */
/*virtual*/ void METH(Read) (_THIS);
/* The system thread which fills the FIFO */
int METH(SystemThread) (void * udata);


MPEGsystem * METH(init) (_THIS);
MPEGsystem * METH(init_mpeg_source) (_THIS, SDL_RWops *mpeg_source);
void METH(destroy) (_THIS);

/* Buffered I/O functions */
void METH(RequestBuffer) (_THIS);
bool METH(Wait) (_THIS);
Uint32 METH(Tell) (_THIS);
void METH(Rewind) (_THIS);
/*virtual*/ void METH(Start) (_THIS);
void METH(Stop) (_THIS);
bool METH(Eof) (_THIS); //const;
/*virtual*/ bool METH(Seek) (_THIS, int length);
/*virtual*/ Uint32 METH(TotalSize) (_THIS);
/*virtual*/ double METH(TotalTime) (_THIS);
/*virtual*/ double METH(TimeElapsedAudio) (_THIS, int atByte);

/* Skip "seconds" seconds */
void METH(Skip) (_THIS, double seconds);

/* Create all the streams present in the MPEG */
struct MPEGstream ** METH(GetStreamList) (_THIS);

/* Insert a stream in the list */
void METH(add_stream) (_THIS, struct MPEGstream * stream);

/* Search for a stream in the list */
struct MPEGstream * METH(get_stream) (_THIS, Uint8 stream_id);

/* Test if a stream is in the list */
Uint8 METH(exist_stream) (_THIS, Uint8 stream_id, Uint8 mask);

/* Reset all the system streams */
void METH(reset_all_streams) (_THIS);

/* Set eof for all streams */
void METH(end_all_streams) (_THIS);

/* Seek the first header */
/*virtual*/ bool METH(seek_first_header) (_THIS);

/* Seek the next header */
/*virtual*/ bool METH(seek_next_header) (_THIS);





#endif /* _MPEGSYSTEM_H_ */
