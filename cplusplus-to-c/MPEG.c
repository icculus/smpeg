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
#undef METH
#define METH(m) MPEG_##m

MPEG*
METH(init) (_THIS)
{
  MAKE_OBJECT(MPEG);
  self->error = MPEGerror_init(NULL);
  return self;
}

MPEG*
METH(init_name) (_THIS, const char *name, bool SDLaudio)
{
  SDL_RWops *source;

  self = METH(init) (self);
  self->mpeg_mem = 0;

  source = SDL_RWFromFile(name, "rb");
  if (!source) {
    METH(InitErrorState) (self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return;
  }
  METH(Init) (self, source, SDLaudio);
  return self;
}

MPEG*
METH(init_descr)(_THIS, int Mpeg_FD, bool SDLaudio) //: MPEGerror()
{
  SDL_RWops *source;
  FILE *file;

  self = METH(init) (self);

  self->mpeg_mem = 0;

  // *** FIXME we're leaking a bit of memory for the FILE *
  // best solution would be to have SDL_RWFromFD
  file = fdopen(Mpeg_FD, "rb");
  if (!file) {
    METH(InitErrorState)(self);
    MPEGerror_SetError(self->error, strerror(errno));
    return;
  }

  source = SDL_RWFromFP(file,false);
  if (!source) {
    METH(InitErrorState)(self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return;
  }
  METH(Init) (self, source, SDLaudio);
  return self;
}

MPEG*
METH(init_data) (_THIS, void *data, int size, bool SDLaudio) //: MPEGerror()
{
  SDL_RWops *source;

  // The semantics are that the data passed in should be copied
  // (?)
//  self->mpeg_mem = new char[size];
  self = METH(init) (self);
  self->mpeg_mem = (char*)(malloc(size));
  memcpy(self->mpeg_mem, data, size);

  source = SDL_RWFromMem(self->mpeg_mem, size);
  if (!source) {
    METH(InitErrorState)(self);
    MPEGerror_SetError(self->error, SDL_GetError());
    return;
  }
  METH(Init) (self, source, SDLaudio);
}

MPEG*
METH(init_rwops) (_THIS, SDL_RWops *mpeg_source, bool SDLaudio) //: MPEGerror()
{
  self = METH(init)(self);
  self->mpeg_mem = 0;
  METH(Init) (self, mpeg_source, SDLaudio);
  return self;
}

void
METH(Init) (_THIS, SDL_RWops *mpeg_source, bool SDLaudio)
{
    self->source = mpeg_source;
    self->sdlaudio = SDLaudio;

    /* Create the system that will parse the MPEG stream */
//    self->system = new MPEGsystem(self->source);
    self->system = MPEGsystem_init_mpeg_source(NULL, self->source);

    /* Initialize everything to invalid values for cleanup */
    self->error->error = NULL;

    self->audiostream = self->videostream = NULL;
//    self->audioaction = NULL;
//    self->videoaction = NULL;
/*    self->audio = NULL; */
/*    self->video = NULL; */
//    self->audioaction_enabled = self->videoaction_enabled = false;
    self->audio_enabled = self->video_enabled = false;
    self->loop = false;
    self->pause = false;

    METH(parse_stream_list) (self);

//    MPEG_EnableAudio(self, self->audioaction_enabled);
//    MPEG_EnableVideo(self, self->videoaction_enabled);
    METH(EnableAudio) (self, self->audio_enabled);
    METH(EnableVideo) (self, self->video_enabled);

    if ( ! self->audiostream && ! self->videostream ) {
//      SetError("No audio/video stream found in MPEG");
      MPEGerror_SetError(self->error, "No audio/video stream found in MPEG");
    }

//    if ( self->system && system->WasError() ) {
    if ( self->system && MPEGerror_WasError(self->system->error) ) {
//      SetError(system->TheError());
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->system->error));
    }

    if ( self->audio && MPEGerror_WasError(self->audio->error) ) {
//      SetError(audio->TheError());
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->audio->error));
    }

    if ( self->video && MPEGerror_WasError(self->video->error) ) {
//      SetError(video->TheError());
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->video->error));
    }

//    if ( WasError() ) {
    if ( MPEGerror_WasError(self->error) ) {
//      SetError(TheError());
      MPEGerror_SetError(self->error, MPEGerror_TheError(self->error));
    }
}

void
METH(InitErrorState) (_THIS)
{
    self->audio = NULL;
    self->video = NULL;
    self->system = NULL;
    self->error->error = NULL;
    self->source = NULL;

    self->audiostream = self->videostream = NULL;
//    self->audioaction = NULL;
//    self->videoaction = NULL;
//    self->audioaction_enabled = self->videoaction_enabled = false;
    self->audio_enabled = self->video_enabled = false;
    self->loop = false;
    self->pause = false;
}

void
METH(destroy) (_THIS)
{
//  Stop(); /* XXX */
  METH(Stop) (self);
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
METH(AudioEnabled) (_THIS)
{
//  return (self->audioaction_enabled);
  return (self->audio_enabled);
}

void
METH(EnableAudio) (_THIS, bool enabled)
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
METH(VideoEnabled) (_THIS)
{
//  return (self->videoaction_enabled);
  return (self->video_enabled);
}

void
METH(EnableVideo) (_THIS, bool enabled)
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

/* dethreaded video. */
void
METH(run) (_THIS)
{
#ifndef THREADED_VIDEO
  MPEGvideo_run(self->video);
#endif /* THREADED_VIDEO */
}

/* MPEG actions */
void
METH(Loop) (_THIS, bool toggle)
{
  self->loop = toggle;
}

void
METH(Play) (_THIS)
{
  if ( METH(AudioEnabled) (self) ) {
//    audioaction->Play(); /* XXX */
//    MPEGaudio_Play(self->audioaction);
//    self->audioaction->Play(self->audioaction);
    MPEGaudio_Play(self->audio);
  }
  if ( METH(VideoEnabled) (self) ) {
//    videoaction->Play(); /* XXX */
//    MPEGvideo_Play(self->videoaction);
//    self->videoaction->Play(self->videoaction);
    MPEGvideo_Play(self->video);
  }
}

void
METH(Stop) (_THIS)
{
  if ( METH(VideoEnabled)(self) ) {
//    videoaction->Stop(); /* XXX */
//    MPEGvideo_Stop(self->videoaction);
//    self->videoaction->Stop(self->videoaction);
    MPEGvideo_Stop(self->video);
  }
  if ( METH(AudioEnabled) (self) ) {
//    audioaction->Stop(); /* XXX */
//    MPEGaudio_Stop(self->audioaction);
//    self->audioaction->Stop(self->audioaction);
    MPEGaudio_Stop(self->audio);
  }
}

void
METH(Rewind) (_THIS)
{
  METH(seekIntoStream) (self, 0);
}

void
METH(Pause) (_THIS)
{
  self->pause = !self->pause;

  if ( METH(VideoEnabled) (self) ) {
//    videoaction->Pause(); /* XXX */
//    MPEGvideo_Pause(self->videoaction);
    MPEGvideo_Pause(self->video);
  }
  if ( METH(AudioEnabled) (self) ) {
//    audioaction->Pause(); /* XXX */
//    MPEGaudio_Pause(self->audioaction);
    MPEGaudio_Pause(self->audio);
  }
}

MPEGstatus
METH(GetStatus) (_THIS)
{
  MPEGstatus status;

  status = MPEG_STOPPED;
  if ( METH(VideoEnabled)(self) ) {
//    switch (videoaction->GetStatus()) { /* XXX */
//    switch (MPEGvideo_GetStatus(self->videoaction)) {
//    switch (self->videoaction->GetStatus(self->videoaction)) {
//    switch (self->video->GetStatus(self->video)) {
    switch (MPEGvideo_GetStatus(self->video)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }
  if ( METH(AudioEnabled) (self) ) {
//    switch (audioaction->GetStatus()) { /* XXX */
//    switch (MPEGaudio_GetStatus(self->audioaction)) {
//    switch (self->audioaction->GetStatus(self->audioaction)) {
//    switch (self->audio->GetStatus(self->audio)) {
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
//    Rewind(); /* XXX */
    METH(Rewind) (self);
//    Play(); /* XXX */
    METH(Play) (self);

    if ( METH(VideoEnabled) (self) ) {
//      switch (videoaction->GetStatus()) { /* XXX */
//      switch (MPEGvideo_GetStatus(self->videoaction)) {
//      switch (self->videoaction->GetStatus(self->videoaction)) {
//      switch (self->video->GetStatus(self->video)) {
      switch (MPEGvideo_GetStatus(self->video)) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
	break;
        default:
        break;
      }
    }
    if ( METH(AudioEnabled) (self) ) {
//      switch (audioaction->GetStatus()) { /* XXX */
//      switch (MPEGaudio_GetStatus(self->audioaction)) {
//      switch (self->audioaction->GetStatus(self->audioaction)) {
//      switch (self->audio->GetStatus(self->audio)) {
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
METH(GetAudioInfo) (_THIS, MPEG_AudioInfo *info)
{
  if ( METH(AudioEnabled) (self) ) {
//    return(audioaction->GetAudioInfo(info)); /* XXX */
//    return(MPEGaudio_GetAudioInfo(self->audioaction, info));
//    return(self->audioaction->GetAudioInfo(self->audioaction, info));
    return (MPEGaudio_GetAudioInfo(self->audio, info));
  }
  return(false);
}

void
METH(Volume) (_THIS, int vol)
{
  if ( METH(AudioEnabled) (self) ) {
//    audioaction->Volume(vol); /* XXX */
//    MPEGaudio_Volume(self->audioaction, vol);
//    self->audioaction->Volume(self->audioaction, vol);
    MPEGaudio_Volume(self->audio, vol);
  }
}

bool
METH(WantedSpec) (_THIS, SDL_AudioSpec *wanted)
{
  if( self->audiostream ) {
//    return(GetAudio()->WantedSpec(wanted)); /* XXX */
    return (MPEGaudio_WantedSpec(METH(GetAudio) (self), wanted));
  }
  return(false);
}

void
METH(ActualSpec) (_THIS, const SDL_AudioSpec *actual)
{
  if( self->audiostream ) {
//    GetAudio()->ActualSpec(actual); /* XXX */
    return (MPEGaudio_ActualSpec(METH(GetAudio) (self), actual));
  }
}

/* Simple accessor used in the C interface */
MPEGaudio *
METH(GetAudio) (_THIS)
{
  return self->audio;
}

/* MPEG video actions */
bool
METH(GetVideoInfo) (_THIS, MPEG_VideoInfo *info)
{
  if ( METH(VideoEnabled) (self) ) {
//    return(videoaction->GetVideoInfo(info)); /* XXX */
//    return (MPEGvideo_GetVideoInfo(self->videoaction, info));
//    return (self->videoaction->GetVideoInfo(self->videoaction, info));
    return MPEGvideo_GetVideoInfo(self->video, info);
  }
  return(false);
}

bool
METH(SetDisplay) (_THIS, SDL_Surface *dst, SDL_mutex *lock, MPEG_DisplayCallback callback)
{
  if ( METH(VideoEnabled) (self) ) {
//    return(videoaction->SetDisplay(dst, lock, callback)); /* XXX */
//    return (MPEGvideo_SetDisplay(self->videoaction, dst, lock, callback));
//    return (self->videoaction->SetDisplay(self->videoaction, dst, lock, callback));
    return MPEGvideo_SetDisplay(self->video, dst, lock, callback);
  }
  return(false);
}

void
METH(MoveDisplay) (_THIS, int x, int y)
{
  if ( METH(VideoEnabled) (self) ) {
//    videoaction->MoveDisplay(x, y); /* XXX */
//    return (MPEGvideo_MoveDisplay(self->videoaction, x, y));
//    return (self->videoaction->MoveDisplay(self->videoaction, x, y));
//    MPEGvideoaction_MoveDisplay(self->videoaction, x, y);
    MPEGvideo_MoveDisplay(self->video, x, y);
  }
}

void
METH(ScaleDisplayXY) (_THIS, int w, int h)
{
  if ( METH(VideoEnabled) (self) ) {
//    videoaction->ScaleDisplayXY(w, h); /* XXX */
//    return (MPEGvideo_ScaleDisplayXY(self->videoaction, w, h));
//    return (self->videoaction->ScaleDisplayXY(self->videoaction, w, h));
    return (MPEGvideo_ScaleDisplayXY(self->video, w, h));
  }
}

void
METH(SetDisplayRegion) (_THIS, int x, int y, int w, int h)
{
  if ( METH(VideoEnabled) (self) ) {
//    videoaction->SetDisplayRegion(x, y, w, h); /* XXX */
//    return (MPEGvideo_SetDisplayRegion(self->videoaction, x, y, w, h));
//    return (self->videoaction->SetDisplayRegion(self->videoaction, x, y, w, h));
    return MPEGvideo_SetDisplayRegion(self->video, x, y, w, h);
  }
}

void
METH(RenderFrame) (_THIS, int frame)
{
    if ( METH(VideoEnabled) (self) ) {
//        videoaction->RenderFrame(frame); /* XXX */
//        MPEGvideo_RenderFrame(self->videoaction, frame);
//        self->videoaction->RenderFrame(self->videoaction, frame);
      MPEGvideo_RenderFrame(self->video, frame);
    }
}

void
METH(RenderFinal) (_THIS, SDL_Surface *dst, int x, int y)
{
//    Stop(); /* XXX */
    METH(Stop) (self);
    if ( METH(VideoEnabled) (self) ) {
//        videoaction->RenderFinal(dst, x, y); /* XXX */
//        MPEGvideo_RenderFinal(self->videoaction, dst, x, y);
//        self->videoaction->RenderFinal(self->videoaction, dst, x, y);
        MPEGvideo_RenderFinal(self->video, dst, x, y);
    }
//    Rewind(); /* XXX */
    METH(Rewind) (self);
//    self->Rewind(self);
}


SMPEG_Filter *
METH(Filter) (_THIS, SMPEG_Filter *filter)
{
  if ( METH(VideoEnabled) (self) ) {
//    return(videoaction->Filter(filter)); /* XXX */
    return (MPEGvideo_Filter(self->video, filter));
//    return (self->videoaction->Filter(self->videoaction, filter));
  }
  return 0;
}

void
METH(Seek) (_THIS, int position)
{
  int was_playing = 0;

  /* Cannot seek past end of file */
//  if((Uint32)self->position > system->TotalSize()) return; /* XXX */
  if((Uint32)position > MPEGsystem_TotalSize(self->system)) return;
  
  /* get info whrether we need to restart playing at the end */
//  if( GetStatus() == MPEG_PLAYING ) /* XXX */
  if (METH(GetStatus) (self) == MPEG_PLAYING)
    was_playing = 1;

  if(!METH(seekIntoStream) (self, position)) return;

  /* If we were playing and not rewind then play again */
  if (was_playing)
//    Play(); /* XXX */
    METH(Play) (self);

  if (METH(VideoEnabled) (self) && !was_playing) 
//    videoaction->RenderFrame(0); /* XXX */
    MPEGvideo_RenderFrame(self->video, 0);

  if ( self->pause && METH(VideoEnabled) (self) ) {
//    videoaction->Pause(); /* XXX */
    MPEGvideo_Pause(self->video);
  }
  if ( self->pause && METH(AudioEnabled) (self) ) {
//    audioaction->Pause(); /* XXX */
    MPEGaudio_Pause(self->audio);
  }
}

bool
METH(seekIntoStream) (_THIS, int position)
{
  /* First we stop everything */
//  Stop(); /* XXX */
  METH(Stop) (self);

  /* Go to the desired position into file */
//  if(!system->Seek(position)) return(false); /* XXX */
  if(!MPEGsystem_Seek(self->system, position)) return(false); /* XXX */

  /* Seek first aligned data */
//  if(self->audiostream && self->audioaction_enabled)
  if(self->audiostream && self->audio_enabled)
//    while(audiostream->time() == -1) /* XXX */
    while (MPEGstream_time(self->audiostream) == -1)
//      if ( ! audiostream->next_packet() ) return false; /* XXX */
      if ( ! MPEGstream_next_packet(self->audiostream, true, true) ) return false;
//  if(self->videostream && self->videoaction_enabled)
  if(self->videostream && self->video_enabled)
//    while(videostream->time() == -1) /* XXX */
    while (MPEGstream_time(self->videostream) == -1) /* XXX */
//      if ( ! videostream->next_packet() ) return false; /* XXX */
      if ( ! MPEGstream_next_packet(self->videostream, true, true) ) return false;

  /* Calculating current play time on audio only makes sense when there
     is no video */  
//  if ( self->audioaction && !self->videoaction) {
  if ( self->audio && !self->video) {
//    audioaction->Rewind(); /* XXX */
    MPEGaudio_Rewind(self->audio);
//    audioaction->ResetSynchro(system->TimeElapsedAudio(position)); /* XXX */
    MPEGaudio_ResetSynchro(self->audio, MPEGsystem_TimeElapsedAudio(self->system, position));
  }
  /* And forget what we previouly buffered */
//  else if ( self->audioaction ) {
  else if ( self->audio ) {
//    audioaction->Rewind(); /* XXX */
    MPEGaudio_Rewind(self->audio);
//    audioaction->ResetSynchro(audiostream->time()); /* XXX */
    MPEGaudio_ResetSynchro(self->audio, MPEGstream_time(self->audiostream));
  }
//  if ( self->videoaction ) {
  if ( self->video ) {
//    videoaction->Rewind(); /* XXX */
    MPEGvideo_Rewind(self->video);
//    videoaction->ResetSynchro(videostream->time()); /* XXX */
    MPEGvideo_ResetSynchro(self->video, MPEGstream_time(self->videostream));
  }

  return(true);
}

void
METH(Skip) (_THIS, float seconds)
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
    if ( METH(VideoEnabled)(self) )
        MPEGvideo_Skip(self->video, seconds);
//    if( MPEG_AudioEnabled(self) ) audioaction->Skip(seconds); /* XXX */
    if (METH(AudioEnabled)(self))
        MPEGaudio_Skip(self->audio, seconds);
  }
}

void
METH(GetSystemInfo) (_THIS, MPEG_SystemInfo *sinfo)
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
//  if( self->videoaction ) 
  if( self->video ) 
//    sinfo->current_time = videoaction->Time(); /* XXX */
    sinfo->current_time = MPEGvideo_Time(self->video);
//  if( self->audioaction )
  if( self->audio )
//    sinfo->current_time = audioaction->Time(); /* XXX */
    sinfo->current_time = MPEGaudio_Time(self->audio);
}

void
METH(parse_stream_list) (_THIS)
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
//	self->audioaction_enabled = true;
	self->audio_enabled = true;
//	self->audiostream->next_packet(); /* XXX */
	MPEGstream_next_packet(self->audiostream, true, true);
//	self->audio = new MPEGaudio(audiostream, sdlaudio);  /* XXX */
	self->audio = MPEGaudio_init(self->audio, self->audiostream, self->sdlaudio);
//	self->audioaction = self->audio;
      break;

      case VIDEO_STREAMID:
	self->videostream = stream_list[i];
//	self->videoaction_enabled = true;
	self->video_enabled = true;
//	self->videostream->next_packet(); /* XXX */
	MPEGstream_next_packet(self->videostream, true, true);
//	self->video = new MPEGvideo(videostream); /* XXX */
	self->video = MPEGvideo_init(self->video, self->videostream);
//	self->videoaction = self->video;
      break;
    }

    i++;
  }
  while(stream_list[i]);
}
