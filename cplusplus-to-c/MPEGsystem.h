/* A class based on the MPEG stream class, used to parse the system stream */
    
/* - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn - */

#ifndef _MPEGSYSTEM_H_
#define _MPEGSYSTEM_H_
#define USE_SYSTEM_TIMESTAMP

#include "SDL.h"
#include "SDL_thread.h"
#include "MPEGerror.h"


#define bool int
#define false 0
#define true (!false)

//class MPEGstream;
struct MPEGstream;

/* MPEG System library
   by Vivien Chappelier */

/* The system class is necessary for splitting the MPEG stream into */
/* peaces of data that will be sent to the audio or video decoder.  */

struct MPEGsystem {
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

    struct MPEGerror *error;
};

typedef struct MPEGsystem MPEGsystem;


MPEGsystem *MPEGsystem_new();
MPEGsystem *MPEGsystem_new_rwops(SDL_RWops *mpeg_source);
void MPEGsystem_destroy(MPEGsystem *self);

/* Buffered I/O functions */
void MPEGsystem_RequestBuffer(MPEGsystem *self);
bool MPEGsystem_Wait(MPEGsystem *self);
Uint32 MPEGsystem_Tell(MPEGsystem *self);
void MPEGsystem_Rewind(MPEGsystem *self);
void MPEGsystem_Start(MPEGsystem *self);
void MPEGsystem_Stop(MPEGsystem *self);
bool MPEGsystem_Eof(MPEGsystem *self); // const;
bool MPEGsystem_Seek(MPEGsystem *self, int length);
Uint32 MPEGsystem_TotalSize(MPEGsystem *self);
double MPEGsystem_TotalTime(MPEGsystem *self);
double MPEGsystem_TimeElapsedAudio(MPEGsystem *self, int atByte);

/* Skip "seconds" seconds */
void MPEGsystem_Skip(MPEGsystem *self, double seconds);

/* Create all the streams present in the MPEG */
struct MPEGstream ** MPEGsystem_GetStreamList(MPEGsystem *self);

/* Insert a stream in the list */
void MPEGsystem_add_stream(MPEGsystem *self, struct MPEGstream * stream);

/* Search for a stream in the list */
struct MPEGstream * MPEGsystem_get_stream(MPEGsystem *self, Uint8 stream_id);

/* Test if a stream is in the list */
Uint8 MPEGsystem_exist_stream(MPEGsystem *self, Uint8 stream_id, Uint8 mask);

/* Reset all the system streams */
void MPEGsystem_reset_all_streams(MPEGsystem *self);

/* Set eof for all streams */
void MPEGsystem_end_all_streams(MPEGsystem *self);

/* Seek the first header */
bool MPEGsystem_seek_first_header(MPEGsystem *self);

/* Seek the next header */
bool MPEGsystem_seek_next_header(MPEGsystem *self);
#endif

