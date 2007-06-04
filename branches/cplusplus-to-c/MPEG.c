#include "SDL.h"

#include "MPEG.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

#undef _THIS
#define _THIS MPEG *self

MPEG*
MPEG_init (_THIS)
{
  MAKE_OBJECT(MPEG);
  self->error = MPEGerror_init(NULL);
  return self;
}

MPEG*
MPEG_init_name (_THIS, const char *name, bool SDLaudio)
{
  SDL_RWops *source;

  self = MPEG_init (self);
  self->mpeg_mem = 0;

  source = SDL_RWFromFile(name, "rb");
  if (!source) {
    MPEG_InitErrorState (self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return self;
  }
  MPEG_Init (self, source, SDLaudio);
  return self;
}

MPEG*
MPEG_init_descr(_THIS, int Mpeg_FD, bool SDLaudio) //: MPEGerror()
{
  SDL_RWops *source;
  FILE *file;

  self = MPEG_init (self);

  self->mpeg_mem = 0;

  // *** FIXME we're leaking a bit of memory for the FILE *
  // best solution would be to have SDL_RWFromFD
  file = fdopen(Mpeg_FD, "rb");
  if (!file) {
    MPEG_InitErrorState(self);
    MPEGerror_SetError(self->error, strerror(errno));
    return self;
  }

  source = SDL_RWFromFP(file,false);
  if (!source) {
    MPEG_InitErrorState(self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return self;
  }
  MPEG_Init (self, source, SDLaudio);
  return self;
}

MPEG*
MPEG_init_data (_THIS, void *data, int size, bool SDLaudio) //: MPEGerror()
{
  SDL_RWops *source;

  // The semantics are that the data passed in should be copied
  // (?)
//  self->mpeg_mem = new char[size];
  self = MPEG_init (self);
  self->mpeg_mem = (char*)(malloc(size));
  memcpy(self->mpeg_mem, data, size);

  source = SDL_RWFromMem(self->mpeg_mem, size);
  if (!source) {
    MPEG_InitErrorState(self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return self;
  }
  MPEG_Init (self, source, SDLaudio);
  return self;
}

MPEG*
MPEG_init_rwops (_THIS, SDL_RWops *mpeg_source, bool SDLaudio) //: MPEGerror()
{
  self = MPEG_init(self);
  self->mpeg_mem = 0;
  MPEG_Init (self, mpeg_source, SDLaudio);
  return self;
}

void
MPEG_Init (_THIS, SDL_RWops *mpeg_source, bool SDLaudio)
{
    self->source = mpeg_source;
    self->sdlaudio = SDLaudio;

    /* Create the system that will parse the MPEG stream */
//    self->system = new MPEGsystem(self->source);
    self->system = MPEGsystem_init_mpeg_source(NULL, self->source);

    /* Initialize everything to invalid values for cleanup */
    self->error->error = NULL;

    self->audiostream = self->videostream = NULL;
    self->audio = NULL;
    self->video = NULL;
    self->audio_enabled = self->video_enabled = false;
    self->loop = false;
    self->pause = false;

    MPEG_parse_stream_list (self);

    MPEG_EnableAudio (self, self->audio_enabled);
    MPEG_EnableVideo (self, self->video_enabled);

    if ( ! self->audiostream && ! self->videostream ) {
      MPEGerror_SetError(self->error, "No audio/video stream found in MPEG");
    }

    if ( self->system && MPEGerror_WasError(self->system->error) ) {
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->system->error));
    }

    if ( self->audio && MPEGerror_WasError(self->audio->error) ) {
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->audio->error));
    }

    if ( self->video && MPEGerror_WasError(self->video->error) ) {
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->video->error));
    }

    if ( MPEGerror_WasError(self->error) ) {
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->error));
    }
}

void
MPEG_InitErrorState (_THIS)
{
    self->audio = NULL;
    self->video = NULL;
    self->system = NULL;
    self->error->error = NULL;
    self->source = NULL;

    self->audiostream = self->videostream = NULL;
    self->audio_enabled = self->video_enabled = false;
    self->loop = false;
    self->pause = false;
}

void
MPEG_destroy (_THIS)
{
  MPEG_Stop (self);
  if (self->video)
    {
      MPEGvideo_destroy(self->video);
      free(self->video);
      self->video = NULL;
    }
  if (self->audio)
    {
      MPEGaudio_destroy(self->audio);
      free(self->audio);
      self->audio = NULL;
    }
  if (self->system)
    {
      MPEGsystem_destroy(self->system);
      free(self->system);
      self->system = NULL;
    }
  
  if (self->source)
    {
      SDL_RWclose(self->source);
      self->source = NULL;
    }
  if ( self->mpeg_mem )
    {
      free(self->mpeg_mem);
      self->mpeg_mem = NULL;
    }

  MPEGerror_destroy(self->error);
  free(self->error);
  self->error = NULL;
}

bool
MPEG_AudioEnabled (_THIS)
{
  return (self->audio_enabled);
}

void
MPEG_EnableAudio (_THIS, bool enabled)
{
  if ( enabled && ! self->audio ) {
    enabled = false;
  }
  self->audio_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( self->audio && ! self->audio_enabled ) {
    MPEGaudio_Stop(self->audio);
  } 
  /* Set the video time source */
  if ( self->video ) {
    if ( self->audio_enabled ) {
      MPEGvideo_SetTimeSource(self->video, self->audio);
    } else {
      MPEGvideo_SetTimeSource(self->video, NULL);
    }
  }
  if(self->audiostream)
    MPEGstream_enable(self->audiostream, enabled);
}

bool
MPEG_VideoEnabled (_THIS)
{
  return (self->video_enabled);
}

void
MPEG_EnableVideo (_THIS, bool enabled)
{
  if ( enabled && ! self->video ) {
    enabled = false;
  }
  self->video_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( self->video && ! self->video_enabled ) {
    MPEGvideo_Stop(self->video);
  } 
  if(self->videostream)
    MPEGstream_enable(self->videostream, enabled);
}

/* Dethreaded video.  Call regularly (such as every 10ms). */
void
MPEG_run (_THIS)
{
#ifndef THREADED_VIDEO
  if (self->video)
    {
      MPEGvideo_run(self->video);
    }
#endif /* THREADED_VIDEO */
}

/* Dethreaded video.  Recommended delay between frames. */
int
MPEG_frametime (_THIS)
{
#ifndef THREADED_VIDEO
  if (self->video)
    {
      return self->video->frametime;
    }
  else
    {
      return 10; /* 100 fps? */
    }
#else
  return 10;
#endif /* THREADED_VIDEO */
}

/* MPEG actions */
void
MPEG_Loop (_THIS, bool toggle)
{
  self->loop = toggle;
}

void
MPEG_Play (_THIS)
{
  if ( MPEG_AudioEnabled (self) ) {
    MPEGaudio_Play(self->audio);
  }
  if ( MPEG_VideoEnabled (self) ) {
    MPEGvideo_Play(self->video);
  }
}

void
MPEG_Stop (_THIS)
{
  if ( MPEG_VideoEnabled(self) ) {
    MPEGvideo_Stop(self->video);
  }
  if ( MPEG_AudioEnabled (self) ) {
    MPEGaudio_Stop(self->audio);
  }
}

void
MPEG_Rewind (_THIS)
{
  MPEG_seekIntoStream (self, 0);
}

void
MPEG_Pause (_THIS)
{
  self->pause = !self->pause;

  if ( MPEG_VideoEnabled (self) ) {
    MPEGvideo_Pause(self->video);
  }
  if ( MPEG_AudioEnabled (self) ) {
    MPEGaudio_Pause(self->audio);
  }
}

MPEGstatus
MPEG_GetStatus (_THIS)
{
  MPEGstatus status;

  status = MPEG_STOPPED;
  if ( MPEG_VideoEnabled(self) ) {
    switch (MPEGvideo_GetStatus(self->video)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }
  if ( MPEG_AudioEnabled (self) ) {
    switch (MPEGaudio_GetStatus(self->audio)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }

  if(status == MPEG_STOPPED && self->loop && !self->pause)
  {
    /* Here we go again */
    MPEG_Rewind (self);
    MPEG_Play (self);

    if ( MPEG_VideoEnabled (self) ) {
      switch (MPEGvideo_GetStatus(self->video)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
	break;
        default:
        break;
      }
    }
    if ( MPEG_AudioEnabled (self) ) {
      switch (MPEGaudio_GetStatus(self->audio)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
	break;
        default:
        break;
      }
    }
  }

  return(status);
}


/* MPEG audio actions */
bool
MPEG_GetAudioInfo (_THIS, MPEG_AudioInfo *info)
{
  if ( MPEG_AudioEnabled (self) ) {
    return (MPEGaudio_GetAudioInfo(self->audio, info));
  }
  return(false);
}

void
MPEG_Volume (_THIS, int vol)
{
  if ( MPEG_AudioEnabled (self) ) {
    MPEGaudio_Volume(self->audio, vol);
  }
}

bool
MPEG_WantedSpec (_THIS, SDL_AudioSpec *wanted)
{
  if( self->audiostream ) {
    return (MPEGaudio_WantedSpec(MPEG_GetAudio (self), wanted));
  }
  return(false);
}

void
MPEG_ActualSpec (_THIS, const SDL_AudioSpec *actual)
{
  if( self->audiostream ) {
    return (MPEGaudio_ActualSpec(MPEG_GetAudio (self), actual));
  }
}

/* Simple accessor used in the C interface */
MPEGaudio *
MPEG_GetAudio (_THIS)
{
  return self->audio;
}

/* MPEG video actions */
bool
MPEG_GetVideoInfo (_THIS, MPEG_VideoInfo *info)
{
  if ( MPEG_VideoEnabled (self) ) {
    return MPEGvideo_GetVideoInfo(self->video, info);
  }
  return(false);
}

bool
MPEG_SetDisplay (_THIS, SDL_Surface *dst, SDL_mutex *lock, MPEG_DisplayCallback callback)
{
  if ( MPEG_VideoEnabled (self) ) {
    return MPEGvideo_SetDisplay(self->video, dst, lock, callback);
  }
  return(false);
}

void
MPEG_MoveDisplay (_THIS, int x, int y)
{
  if ( MPEG_VideoEnabled (self) ) {
    MPEGvideo_MoveDisplay(self->video, x, y);
  }
}

void
MPEG_ScaleDisplayXY (_THIS, int w, int h)
{
  if ( MPEG_VideoEnabled (self) ) {
    return (MPEGvideo_ScaleDisplayXY(self->video, w, h));
  }
}

void
MPEG_SetDisplayRegion (_THIS, int x, int y, int w, int h)
{
  if ( MPEG_VideoEnabled (self) ) {
    return MPEGvideo_SetDisplayRegion(self->video, x, y, w, h);
  }
}

void
MPEG_RenderFrame (_THIS, int frame)
{
    if ( MPEG_VideoEnabled (self) ) {
      MPEGvideo_RenderFrame(self->video, frame);
    }
}

void
MPEG_RenderFinal (_THIS, SDL_Surface *dst, int x, int y)
{
    MPEG_Stop (self);
    if ( MPEG_VideoEnabled (self) ) {
        MPEGvideo_RenderFinal(self->video, dst, x, y);
    }
    MPEG_Rewind (self);
}


SMPEG_Filter *
MPEG_Filter (_THIS, SMPEG_Filter *filter)
{
  if ( MPEG_VideoEnabled (self) ) {
    return (MPEGvideo_Filter(self->video, filter));
  }
  return 0;
}

void
MPEG_Seek (_THIS, int position)
{
  int was_playing = 0;

  /* Cannot seek past end of file */
  if((Uint32)position > MPEGsystem_TotalSize(self->system)) return;
  
  /* get info whrether we need to restart playing at the end */
  if (MPEG_GetStatus (self) == MPEG_PLAYING)
    was_playing = 1;

  if(!MPEG_seekIntoStream (self, position)) return;

  /* If we were playing and not rewind then play again */
  if (was_playing)
    MPEG_Play (self);

  if (MPEG_VideoEnabled (self) && !was_playing) 
    MPEGvideo_RenderFrame(self->video, 0);

  if ( self->pause && MPEG_VideoEnabled (self) ) {
    MPEGvideo_Pause(self->video);
  }
  if ( self->pause && MPEG_AudioEnabled (self) ) {
    MPEGaudio_Pause(self->audio);
  }
}

bool
MPEG_seekIntoStream (_THIS, int position)
{
  /* First we stop everything */
  MPEG_Stop (self);

  /* Go to the desired position into file */
  if(!MPEGsystem_Seek(self->system, position)) return(false);

  /* Seek first aligned data */
  if(self->audiostream && self->audio_enabled)
    while (MPEGstream_time(self->audiostream) == -1)
      if ( ! MPEGstream_next_packet(self->audiostream, true, true) ) return false;
  if(self->videostream && self->video_enabled)
    while (MPEGstream_time(self->videostream) == -1)
      if ( ! MPEGstream_next_packet(self->videostream, true, true) ) return false;

  /* Calculating current play time on audio only makes sense when there
     is no video */  
  if ( self->audio && !self->video) {
    MPEGaudio_Rewind(self->audio);
    MPEGaudio_ResetSynchro(self->audio, MPEGsystem_TimeElapsedAudio(self->system, position));
  }
  /* And forget what we previouly buffered */
  else if ( self->audio ) {
    MPEGaudio_Rewind(self->audio);
    MPEGaudio_ResetSynchro(self->audio, MPEGstream_time(self->audiostream));
  }
  if ( self->video ) {
    MPEGvideo_Rewind(self->video);
    MPEGvideo_ResetSynchro(self->video, MPEGstream_time(self->videostream));
  }

  return(true);
}

void
MPEG_Skip (_THIS, float seconds)
{
  if (MPEGsystem_get_stream(self->system, SYSTEM_STREAMID))
  {
    MPEGsystem_Skip(self->system, seconds);
  }
  else
  {
    /* No system information in MPEG */
    if ( MPEG_VideoEnabled(self) )
        MPEGvideo_Skip(self->video, seconds);
    if (MPEG_AudioEnabled(self))
        MPEGaudio_Skip(self->audio, seconds);
  }
}

void
MPEG_GetSystemInfo (_THIS, MPEG_SystemInfo *sinfo)
{
  sinfo->total_size = MPEGsystem_TotalSize(self->system);
  sinfo->current_offset = MPEGsystem_Tell(self->system);
  sinfo->total_time = MPEGsystem_TotalTime(self->system);

  /* Get current time from audio or video decoder */
  /* TODO: move timing reference in MPEGsystem    */
  sinfo->current_time = 0;
  if( self->video ) 
    sinfo->current_time = MPEGvideo_Time(self->video);
  if( self->audio )
    sinfo->current_time = MPEGaudio_Time(self->audio);
}

void
MPEG_parse_stream_list (_THIS)
{
  MPEGstream ** stream_list;
  register int i;

  /* A new thread is created for each video and audio */
  /* stream                                           */ 
  /* TODO: support MPEG systems containing more than  */
  /*       one audio or video stream                  */
  i = 0;
  do
  {
    /* Retreive the list of streams */
    stream_list = MPEGsystem_GetStreamList(self->system);

    switch(stream_list[i]->streamid)
    {
      case SYSTEM_STREAMID:
      break;

      case AUDIO_STREAMID:
	self->audiostream = stream_list[i];
	self->audio_enabled = true;
	MPEGstream_next_packet(self->audiostream, true, true);
	self->audio = MPEGaudio_init(self->audio, self->audiostream, self->sdlaudio);
      break;

      case VIDEO_STREAMID:
	self->videostream = stream_list[i];
	self->video_enabled = true;
	MPEGstream_next_packet(self->videostream, true, true);
	self->video = MPEGvideo_init(self->video, self->videostream);
      break;
    }

    i++;
  }
  while(stream_list[i]);
}
