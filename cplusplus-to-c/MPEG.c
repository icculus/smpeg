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

MPEG*
MPEG_new (const char *name, bool SDLaudio) //: MPEGerror()
{
  MPEG* self;
  SDL_RWops *source;

  self = (MPEG*)malloc(sizeof(MPEG));

  self->mpeg_mem = 0;

  source = SDL_RWFromFile(name, "rb");
  if (!source) {
    InitErrorState();
    SetError(SDL_GetError());
    return;
  }
  MPEG_Init(self, source, SDLaudio);
}

MPEG*
MPEG_new_fd (int Mpeg_FD, bool SDLaudio) //: MPEGerror()
{
  MPEG *self;
  SDL_RWops *source;
  FILE *file;

  self = (MPEG*)malloc(sizeof(MPEG));
  self->mpeg_mem = 0;

  // *** FIXME we're leaking a bit of memory for the FILE *
  // best solution would be to have SDL_RWFromFD
  file = fdopen(Mpeg_FD, "rb");
  if (!file) {
    InitErrorState();
    SetError(strerror(errno));
    return;
  }

  source = SDL_RWFromFP(file,false);
  if (!source) {
    InitErrorState();
    SetError(SDL_GetError());
    return;
  }
  MPEG_Init(self, source, SDLaudio);
}

MPEG*
MPEG_new_data (void *data, int size, bool SDLaudio) //: MPEGerror()
{
  MPEG *self;
  SDL_RWops *source;

  // The semantics are that the data passed in should be copied
  // (?)
//  self->mpeg_mem = new char[size];
  self->mpeg_mem = (char*)(malloc(size));
  memcpy(self->mpeg_mem, data, size);

  source = SDL_RWFromMem(self->mpeg_mem, size);
  if (!source) {
    InitErrorState();
    SetError(SDL_GetError());
    return;
  }
  MPEG_Init(self, source, SDLaudio);
}

MPEG*
MPEG_new_SDL (SDL_RWops *mpeg_source, bool SDLaudio) //: MPEGerror()
{
  MPEG* self;
  self = (MPEG*)(malloc(sizeof(MPEG)));
  self->mpeg_mem = 0;
  MPEG_Init(self, mpeg_source, SDLaudio);
}

void
MPEG_Init (MPEG* self, SDL_RWops *mpeg_source, bool SDLaudio)
{
    self->source = mpeg_source;
    self->sdlaudio = SDLaudio;

    /* Create the system that will parse the MPEG stream */
//    self->system = new MPEGsystem(self->source);
    self->system = MPEGsystem_new(self->source);

    /* Initialize everything to invalid values for cleanup */
    self->MPEGerror->error = NULL;

    self->audiostream = self->videostream = NULL;
    self->audioaction = NULL;
    self->videoaction = NULL;
    self->audio = NULL;
    self->video = NULL;
    self->audioaction_enabled = self->videoaction_enabled = false;
    self->loop = false;
    self->pause = false;

    MPEG_parse_stream_list(self);

    MPEG_EnableAudio(self, self->audioaction_enabled);
    MPEG_EnableVideo(self, self->videoaction_enabled);

    if ( ! self->audiostream && ! self->videostream ) {
      SetError("No audio/video stream found in MPEG");
    }

//    if ( self->system && system->WasError() ) {
    if ( self->system && MPEGerror_WasError(self->system->MPEGerror) ) {
//      SetError(system->TheError());
      MPEG_SetError(self, MPEGerror_TheError(self->system->MPEGerror));
    }

    if ( self->audio && MPEGerror_WasError(self->audio->MPEGerror) ) {
//      SetError(audio->TheError());
      MPEG_SetError(self, MPEGerror_TheError(self->audio->MPEGerror));
    }

    if ( self->video && MPEGerror_WasError(self->video->MPEGerror) ) {
//      SetError(video->TheError());
      MPEG_SetError(self, MPEGerror_TheError(self->video->MPEGerror));
    }

//    if ( WasError() ) {
    if ( MPEGerror_WasError(self->MPEGerror) ) {
//      SetError(TheError());
      MPEG_SetError(self, MPEGerror_TheError(self->MPEGerror));
    }
}

void
MPEG_InitErrorState (MPEG *self)
{
    self->audio = NULL;
    self->video = NULL;
    self->system = NULL;
    self->MPEGerror->error = NULL;
    self->source = NULL;

    self->audiostream = self->videostream = NULL;
    self->audioaction = NULL;
    self->videoaction = NULL;
    self->audio = NULL;
    self->video = NULL;
    self->audioaction_enabled = self->videoaction_enabled = false;
    self->loop = false;
    self->pause = false;
}

void
MPEG_destroy (MPEG *self)
{
//  Stop(); /* XXX */
  MPEG_Stop(self);
  if (self->video) free(self->video);
  if (self->audio) free(self->audio);
  if (self->system) free(self->system);
  
  if (self->source) SDL_RWclose(self->source);
  if ( self->mpeg_mem )
    {
      free(self->mpeg_mem);
      self->mpeg_mem = NULL;
    }
}

bool
MPEG_AudioEnabled (MPEG *self)
{
  return (self->audioaction_enabled);
}

void
MPEG_EnableAudio (MPEG *self, bool enabled)
{
  if ( enabled && ! self->audioaction ) {
    enabled = false;
  }
  self->audioaction_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( self->audioaction && ! self->audioaction_enabled ) {
//    audioaction->Stop();  /* XXX */
    MPEGaudio_Stop(self->audioaction);
  } 
  /* Set the video time source */
  if ( self->videoaction ) {
    if ( self->audioaction_enabled ) {
//      videoaction->SetTimeSource(audioaction); /* XXX */
      MPEGvideo_SetTimeSource(self->videoaction, self->audioaction);
    } else {
//      videoaction->SetTimeSource(NULL); /* XXX */
      MPEGvideo_SetTimeSource(self->videoaction, NULL);
    }
  }
  if(self->audiostream)
//    audiostream->enable(enabled);  /* XXX */
    MPEGstream_enable(self->audiostream, enabled);
}

bool
MPEG_VideoEnabled (MPEG *self)
{
  return (self->videoaction_enabled);
}

void MPEG_EnableVideo (MPEG *self, bool enabled)
{
  if ( self->enabled && ! self->videoaction ) {
    self->enabled = false;
  }
  self->videoaction_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( self->videoaction && ! self->videoaction_enabled ) {
//    videoaction->Stop(); /* XXX: */
    MPEGvideo_Stop(self->videoaction);
  } 
  if(self->videostream)
//    videostream->enable(enabled); /* XXX */
    self->videostream->enable(self->videostream, enabled);
    MPEGstream_enable(self->videostream);
}

/* MPEG actions */
void
MPEG_Loop (MPEG *self, bool toggle)
{
  self->loop = toggle;
}

MPEG_Play (MPEG *self)
{
  if ( MPEG_AudioEnabled(self) ) {
//    audioaction->Play(); /* XXX */
    MPEGaudio_Play(self->audioaction);
  }
  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->Play(); /* XXX */
    MPEGvideo_Play(self->videoaction);
  }
}

void
MPEG_Stop (MPEG *self)
{
  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->Stop(); /* XXX */
    MPEGvideo_Stop(self->videoaction);
  }
  if ( MPEG_AudioEnabled(self) ) {
//    audioaction->Stop(); /* XXX */
    MPEGaudio_Stop(self->audioaction);
  }
}

void
MPEG_Rewind (MPEG *self)
{
  MPEG_seekIntoStream(self, 0);
}

void
MPEG_Pause (MPEG *self)
{
  self->pause = !self->pause;

  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->Pause(); /* XXX */
    MPEGvideo_Pause(self->videoaction);
  }
  if ( MPEG_AudioEnabled(self) ) {
//    audioaction->Pause(); /* XXX */
    MPEGaudio_Pause(self->audioaction);
  }
}

/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
MPEGstatus
MPEG_GetStatus (MPEG *self)
{
  MPEGstatus status;

  status = MPEG_STOPPED;
  if ( MPEG_VideoEnabled(self) ) {
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
//    switch (videoaction->GetStatus()) { /* XXX */
    switch (MPEGvideo_GetStatus(self->videoaction)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }
  if ( MPEG_AudioEnabled(self) ) {
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
//    switch (audioaction->GetStatus()) { /* XXX */
    switch (MPEGaudio_GetStatus(self->audioaction)) {
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
//    Rewind(); /* XXX */
    MPEG_Rewind(self);
//    Play(); /* XXX */
    MPEG_Play(self);

    if ( MPEG_VideoEnabled(self) ) {
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
//      switch (videoaction->GetStatus()) { /* XXX */
      switch (MPEGvideo_GetStatus(self->videoaction)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
	break;
        default:
        break;
      }
    }
    if ( MPEG_AudioEnabled(self) ) {
		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
//      switch (audioaction->GetStatus()) { /* XXX */
      switch (MPEGaudio_GetStatus(self->audioaction)) {
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
MPEG_GetAudioInfo (MPEG *self, MPEG_AudioInfo *info)
{
  if ( MPEG_AudioEnabled(self) ) {
//    return(audioaction->GetAudioInfo(info)); /* XXX */
    return(MPEGaudio_GetAudioInfo(self->audioaction, info));
  }
  return(false);
}

void
MPEG_Volume (MPEG *self, int vol)
{
  if ( MPEG_AudioEnabled(self) ) {
//    audioaction->Volume(vol); /* XXX */
    MPEGaudio_Volume(self->audioaction, vol);
  }
}

bool
MPEG_WantedSpec (MPEG *self, SDL_AudioSpec *wanted)
{
  if( self->audiostream ) {
//    return(GetAudio()->WantedSpec(wanted)); /* XXX */
    return (MPEGaduio_WantedSpec(MPEG_GetAudio(self), wanted));
  }
  return(false);
}

void
MPEG_ActualSpec (MEPG *self, const SDL_AudioSpec *actual)
{
  if( self->audiostream ) {
//    GetAudio()->ActualSpec(actual); /* XXX */
    return (MPEGaudio_ActualSpec(MPEG_GetAudio(self), actual));
  }
}

/* Simple accessor used in the C interface */
MPEGaudio *
MPEG_GetAudio (MPEG *self)
{
  return self->audio;
}

/* MPEG video actions */
bool
MPEG_GetVideoInfo (MPEG *self, MPEG_VideoInfo *info)
{
  if ( MPEG_VideoEnabled(self) ) {
//    return(videoaction->GetVideoInfo(info)); /* XXX */
    return (MPEGvideo_GetVideoInfo(self->videoaction, info));
  }
  return(false);
}

bool
MPEG_SetDisplay (MPEG *self, SDL_Surface *dst, SDL_mutex *lock, MPEG_DisplayCallback callback)
{
  if ( MPEG_VideoEnabled(self) ) {
//    return(videoaction->SetDisplay(dst, lock, callback)); /* XXX */
    return (MPEGvideo_SetDisplay(self->videoaction, dst, lock, callback));
  }
  return(false);
}

void
MPEG_MoveDisplay (MPEG *self, int x, int y)
{
  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->MoveDisplay(x, y); /* XXX */
    return (MPEGvideo_MoveDisplay(self->videoaction, x, y));
  }
}

void
MPEG_ScaleDisplay (MPEG *self, int w, int h)
{
  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->ScaleDisplayXY(w, h); /* XXX */
    return (MPEGvideo_ScaleDisplayXY(self->videoaction, w, h));
  }
}

void
MPEG_SetDisplayRegion (MPEG *self, int x, int y, int w, int h)
{
  if ( MPEG_VideoEnabled(self) ) {
//    videoaction->SetDisplayRegion(x, y, w, h); /* XXX */
    return (MPEGvideo_SetDisplayRegion(self->videoaction, x, y, w, h));
  }
}

void
MPEG_RenderFrame (MPEG *self, int frame)
{
    if ( MPEG_VideoEnabled(self) ) {
//        videoaction->RenderFrame(frame); /* XXX */
        MPEGvideo_RenderFrame(self->videoaction, frame);
    }
}

void
MPEG_RenderFinal (MPEG *self, SDL_Surface *dst, int x, int y)
{
//    Stop(); /* XXX */
    MPEG_Stop(self);
    if ( MPEG_VideoEnabled(self) ) {
//        videoaction->RenderFinal(dst, x, y); /* XXX */
        MPEGvideo_RenderFinal(self->videoaction, dst, x, y);
    }
//    Rewind(); /* XXX */
    MPEG_Rewind(self);
}


SMPEG_Filter *
MPEG_Filter (MPEG *self, SMPEG_Filter *filter)
{
  if ( MPEG_VideoEnabled(self) ) {
//    return(videoaction->Filter(filter)); /* XXX */
    return (MPEGvideo_Filter(self->videoaction, filter));
  }
  return 0;
}

void
MPEG_Seek (MPEG *self, int position)
{
  int was_playing = 0;

  /* Cannot seek past end of file */
//  if((Uint32)self->position > system->TotalSize()) return; /* XXX */
  if((Uint32)self->position > MPEGsystem_TotalSize(self->system)) return;
  
	/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
  /* get info whrether we need to restart playing at the end */
//  if( GetStatus() == MPEG_PLAYING ) /* XXX */
  if (MPEG_GetStatus(self) == MPEG_PLAYING)
    was_playing = 1;

  if(!MPEG_seekIntoStream(self, position)) return;

  /* If we were playing and not rewind then play again */
  if (was_playing)
//    Play(); /* XXX */
    MPEG_Play(self);

  if (MPEG_VideoEnabled(self) && !was_playing) 
//    videoaction->RenderFrame(0); /* XXX */
    MPEGvideo_RenderFrame(self->videoaction, 0);

  if ( self->pause && MPEG_VideoEnabled(self)() ) {
//    videoaction->Pause(); /* XXX */
    MPEGvideo_Pause(self->videoaction);
  }
  if ( self->pause && MPEG_AudioEnabled(self) ) {
//    audioaction->Pause(); /* XXX */
    MPEGaudio_Pause(self->audioaction);
  }
}

bool
MPEG_seekIntoStream (MPEG *self, int position)
{
  /* First we stop everything */
//  Stop(); /* XXX */
  MPEG_Stop(self);

  /* Go to the desired position into file */
//  if(!system->Seek(position)) return(false); /* XXX */
  if(!MPEGsystem_Seek(self->system, position)) return(false); /* XXX */

  /* Seek first aligned data */
  if(self->audiostream && self->audioaction_enabled)
//    while(audiostream->time() == -1) /* XXX */
    while (MPEGstream_time(self->audiostream) == -1)
//      if ( ! audiostream->next_packet() ) return false; /* XXX */
      if ( ! MPEGsystem_next_packet(self->audiostream) ) return false;
  if(self->videostream && self->videoaction_enabled)
//    while(videostream->time() == -1) /* XXX */
    while (MPEGstream_time(self->videostream) == -1) /* XXX */
//      if ( ! videostream->next_packet() ) return false; /* XXX */
      if ( ! MPEGsystem_next_packet(self->videostream) ) return false;

  /* Calculating current play time on audio only makes sense when there
     is no video */  
  if ( self->audioaction && !self->videoaction) {
//    audioaction->Rewind(); /* XXX */
    MPEGaudio_Rewind(self->audioaction);
//    audioaction->ResetSynchro(system->TimeElapsedAudio(position)); /* XXX */
    MPEGaudio_ResetSynchro(MPEGsystem_TimeElapsedAudio(self->system, position));
  }
  /* And forget what we previouly buffered */
  else if ( self->audioaction ) {
//    audioaction->Rewind(); /* XXX */
    MPEGaudio_Rewind(self->audioaction);
//    audioaction->ResetSynchro(audiostream->time()); /* XXX */
    MPEGaudio_ResetSynchro(MPEGstream_time(self->audiostream));
  }
  if ( self->videoaction ) {
//    videoaction->Rewind(); /* XXX */
    MPEGvideo_Rewind(self->videoaction);
//    videoaction->ResetSynchro(videostream->time()); /* XXX */
    MPEGvideo_ResetSynchro(MPEGstream_time(self->videostream));
  }

  return(true);
}

void
MPEG_Skip (MPEG *self, float seconds)
{
//  if(system->get_stream(SYSTEM_STREAMID)) /* XXX */
  if (MPEGsystem_get_stream(self->system, SYSTEM_STREAMID))
  {
//    system->Skip(seconds); /* XXX */
    MPEGsystem_Skip(self->system, seconds);
  }
  else
  {
    /* No system information in MPEG */
//    if( MPEG_VideoEnabled(self) ) videoaction->Skip(seconds); /* XXX */
    if ( MPEG_VideoEnabled(self) )
        MPEGvideo_Skip(self->videoaction, seconds);
//    if( MPEG_AudioEnabled(self) ) audioaction->Skip(seconds); /* XXX */
    if (MPEG_AudioEnabled(self))
        MPEGaudio_Skip(self->audioaction, seconds);
  }
}

void
MPEG_GetSystemInfo (MPEG *self, MPEG_SystemInfo *sinfo)
{
//  sinfo->total_size = system->TotalSize(); /* XXX */
  sinfo->total_size = MPEGsystem_TotalSize(self->system);
//  sinfo->current_offset = system->Tell(); /* XXX */
  sinfo->current_offset = MPEGsystem_Tell(self->system);
//  sinfo->total_time = system->TotalTime(); /* XXX */
  sinfo->total_time = MPEGsystem_TotalTime(self->system);

  /* Get current time from audio or video decoder */
  /* TODO: move timing reference in MPEGsystem    */
  sinfo->current_time = 0;
  if( self->videoaction ) 
//    sinfo->current_time = videoaction->Time(); /* XXX */
    sinfo->current_time = MPEGvideo_Time(self->videoaction);
  if( self->audioaction )
//    sinfo->current_time = audioaction->Time(); /* XXX */
    sinfo->current_time = MPEGaudio_Time(self->audioaction);
}

void
MPEG_parse_stream_list (MPEG *self)
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
//    stream_list = system->GetStreamList(); /* XXX */
    stream_list = MPEGsystem_GetStreamList(self->system);

    switch(stream_list[i]->streamid)
    {
      case SYSTEM_STREAMID:
      break;

      case AUDIO_STREAMID:
	self->audiostream = stream_list[i];
	self->audioaction_enabled = true;
//	self->audiostream->next_packet(); /* XXX */
	MPEGsystem_next_packet(self->audiostream);
//	self->audio = new MPEGaudio(audiostream, sdlaudio);  /* XXX */
	self->audio = MPEGaudio_new(audiostream, sdlaudio);
	self->audioaction = audio;
      break;

      case VIDEO_STREAMID:
	self->videostream = stream_list[i];
	self->videoaction_enabled = true;
//	self->videostream->next_packet(); /* XXX */
	MPEGsystem_next_packet(self->videostream);
//	self->video = new MPEGvideo(videostream); /* XXX */
	self->video = MPEGvideo_new(videostream);
	self->videoaction = video;
      break;
    }

    i++;
  }
  while(stream_list[i]);
}
