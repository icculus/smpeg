/* A class based on the MPEG stream class, used to parse the system stream */

#ifndef _MPEGSYSTEM_H_
#define _MPEGSYSTEM_H_
#define USE_SYSTEM_TIMESTAMP

#include "SDL.h"
#include "SDL_thread.h"
#include "MPEGerror.h"

class MPEGstream;

/* MPEG System library
   by Vivien Chappelier */

/* The system class is necessary for splitting the MPEG stream into */
/* peaces of data that will be sent to the audio or video decoder.  */

class MPEGsystem : public MPEGerror
{
public:
    MPEGsystem(int MPEG_Fd);
    virtual ~MPEGsystem();

    /* Buffered I/O functions */
    void RequestBuffer();
    Uint32 Tell();
    void Rewind();
    void Loop(bool toggle);
    bool Eof() const;
    double Seek(int length);
    /* Skip "seconds" seconds */
    void Skip(double seconds);

    /* Create all the streams present in the MPEG */
    MPEGstream ** GetStreamList();

    /* Insert a stream in the list */
    void add_stream(MPEGstream * stream);

    /* Search for a stream in the list */
    MPEGstream * get_stream(Uint8 stream_id);

    /* Test if a stream is in the list */
    Uint8 exist_stream(Uint8 stream_id, Uint8 mask);

    /* Reset all the system streams */
    void reset_all_streams();
    
    /* Set eof for all streams */
    void end_all_streams();
    
    /* Set looping for all streams */
    void loop_all_streams(bool toggle);

    /* Seek the next header */
    bool seek_next_header();

protected:
    /* Fill a buffer */
    Uint8 FillBuffer();

    /* Read a new packet */
    void Read();

    /* The system thread which fills the FIFO */
    static int SystemThread(void * udata);

    int mpeg_fd;

    SDL_Thread * system_thread;
    bool system_thread_running;

    MPEGstream ** stream_list;

    Uint8 * read_buffer;
    Uint8 * pointer;
    int read_size;
    Uint32 read_total;
    Uint32 packet_total;
    int request;

    bool endofstream;
    bool errorstream;
    bool looping;

#ifdef USE_SYSTEM_TIMESTAMP
    /* Current timestamp for this stream */
    double timestamp;
    double timedrift;
    double skip_timestamp;
#endif
};
#endif

