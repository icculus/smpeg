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

MPEG::MPEG(const char * name, bool Sdlaudio) :
  MPEGerror()
{
  int new_fd;

  new_fd = open(name, O_RDONLY|O_BINARY);
  close_fd = true;

  if(new_fd == -1)
  {
    audio = NULL;
    video = NULL;
    system = NULL;
    close_fd = false;
    error = NULL;

    audiostream = videostream = NULL;
    audioaction = NULL;
    videoaction = NULL;
    audio = NULL;
    video = NULL;
    audioaction_enabled = videoaction_enabled = false;
    loop = false;
    pause = false;

    SetError(strerror(errno));

    return;
  }

  Init(new_fd, Sdlaudio);
}

MPEG::MPEG(int Mpeg_FD, bool Sdlaudio) :
  MPEGerror()
{
  close_fd = false;
  Init(Mpeg_FD, Sdlaudio);
}

MPEG::MPEG(void *data, int size, bool Sdlaudio) :
  MPEGerror()
{
  close_fd = false;
  Init(data, size, Sdlaudio);
}

void MPEG::Init(int Mpeg_FD, bool Sdlaudio)
{
    mpeg_fd = Mpeg_FD;

    sdlaudio = Sdlaudio;

    /* Create the system that will parse the MPEG stream */
    system = new MPEGsystem(Mpeg_FD);

    /* Initialize everything to invalid values for cleanup */
    error = NULL;

    audiostream = videostream = NULL;
    audioaction = NULL;
    videoaction = NULL;
    audio = NULL;
    video = NULL;
    audioaction_enabled = videoaction_enabled = false;
    loop = false;
    pause = false;

    parse_stream_list();

    EnableAudio(audioaction_enabled);
    EnableVideo(videoaction_enabled);

    if ( ! audiostream && ! videostream ) {
      SetError("No audio/video stream found in MPEG");
    }

    if ( system && system->WasError() ) {
      SetError(system->TheError());
    }

    if ( audio && audio->WasError() ) {
      SetError(audio->TheError());
    }

    if ( video && video->WasError() ) {
      SetError(video->TheError());
    }

    if ( WasError() ) {
      SetError(TheError());
    }
}

void MPEG::Init(void *data, int size, bool Sdlaudio)
{
    sdlaudio = Sdlaudio;

    /* Create the system that will parse the MPEG stream */
    system = new MPEGsystem(data, size);

    /* Initialize everything to invalid values for cleanup */
    error = NULL;

    audiostream = videostream = NULL;
    audioaction = NULL;
    videoaction = NULL;
    audio = NULL;
    video = NULL;
    audioaction_enabled = videoaction_enabled = false;
    loop = false;
    pause = false;

    parse_stream_list();

    EnableAudio(audioaction_enabled);
    EnableVideo(videoaction_enabled);

    if ( ! audiostream && ! videostream ) {
      SetError("No audio/video stream found in MPEG");
    }

    if ( system && system->WasError() ) {
      SetError(system->TheError());
    }

    if ( audio && audio->WasError() ) {
      SetError(audio->TheError());
    }

    if ( video && video->WasError() ) {
      SetError(video->TheError());
    }

    if ( WasError() ) {
      SetError(TheError());
    }
}

MPEG::~MPEG()
{
  Stop();
  if(video) delete video;
  if(audio) delete audio;
  if(system) delete system;

  if ( close_fd && mpeg_fd ) {
    close(mpeg_fd);
  }
}

bool MPEG::AudioEnabled(void) {
  return(audioaction_enabled);
}
void MPEG::EnableAudio(bool enabled) {
  if ( enabled && ! audioaction ) {
    enabled = false;
  }
  audioaction_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( audioaction && ! audioaction_enabled ) {
    audioaction->Stop();
  } 
  /* Set the video time source */
  if ( videoaction ) {
    if ( audioaction_enabled ) {
      videoaction->SetTimeSource(audioaction);
    } else {
      videoaction->SetTimeSource(NULL);
    }
  }
/*
  if(audiostream)
    audiostream->enable(enabled);
*/
}
bool MPEG::VideoEnabled(void) {
  return(videoaction_enabled);
}
void MPEG::EnableVideo(bool enabled) {
  if ( enabled && ! videoaction ) {
    enabled = false;
  }
  videoaction_enabled = enabled;

  /* Stop currently playing stream, if necessary */
  if ( videoaction && ! videoaction_enabled ) {
    videoaction->Stop();
  } 
}

/* MPEG actions */
void MPEG::Loop(bool toggle) {
  loop = toggle;
  system->Loop(toggle);
}
void MPEG::Play(void) {
  if ( AudioEnabled() ) {
    audioaction->Play();
  }
  if ( VideoEnabled() ) {
    videoaction->Play();
  }
}
void MPEG::Stop(void) {
  if ( VideoEnabled() ) {
    videoaction->Stop();
  }
  if ( AudioEnabled() ) {
    audioaction->Stop();
  }
}

void MPEG::Rewind(void) {
  seekIntoStream(0);
}

void MPEG::Pause(void) {
  pause = !pause;

  if ( VideoEnabled() ) {
    videoaction->Pause();
  }
  if ( AudioEnabled() ) {
    audioaction->Pause();
  }
}

MPEGstatus MPEG::Status(void) {
  MPEGstatus status;

  status = MPEG_STOPPED;
  if ( VideoEnabled() ) {
    switch (videoaction->Status()) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }
  if ( AudioEnabled() ) {
    switch (audioaction->Status()) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
      break;
      default:
      break;
    }
  }

  if(status == MPEG_STOPPED && loop && !pause)
  {
    /* Here we go again */
    Rewind();
    Play();

    if ( VideoEnabled() ) {
      switch (videoaction->Status()) {
      case MPEG_PLAYING:
        status = MPEG_PLAYING;
	break;
        default:
        break;
      }
    }
    if ( AudioEnabled() ) {
      switch (audioaction->Status()) {
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
bool MPEG::GetAudioInfo(MPEG_AudioInfo *info) {
  if ( AudioEnabled() ) {
    return(audioaction->GetAudioInfo(info));
  }
  return(false);
}
void MPEG::Volume(int vol) {
  if ( AudioEnabled() ) {
    audioaction->Volume(vol);
  }
}
bool MPEG::WantedSpec(SDL_AudioSpec *wanted) {
  if( audiostream ) {
    return(GetAudio()->WantedSpec(wanted));
  }
  return(false);
}
void MPEG::ActualSpec(const SDL_AudioSpec *actual) {
  if( audiostream ) {
    GetAudio()->ActualSpec(actual);
  }
}
MPEGaudio *MPEG::GetAudio(void) { // Simple accessor used in the C interface
  return audio;
}

/* MPEG video actions */
bool MPEG::GetVideoInfo(MPEG_VideoInfo *info) {
  if ( VideoEnabled() ) {
    return(videoaction->GetVideoInfo(info));
  }
  return(false);
}
bool MPEG::SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
		MPEG_DisplayCallback callback) {
  if ( VideoEnabled() ) {
    return(videoaction->SetDisplay(dst, lock, callback));
  }
  return(false);
}
void MPEG::MoveDisplay(int x, int y) {
  if ( VideoEnabled() ) {
    videoaction->MoveDisplay(x, y);
  }
}
void MPEG::ScaleDisplayXY(int w, int h) {
  if ( VideoEnabled() ) {
    videoaction->ScaleDisplayXY(w, h);
  }
}
void MPEG::SetDisplayRegion(int x, int y, int w, int h) {
  if ( VideoEnabled() ) {
    videoaction->SetDisplayRegion(x, y, w, h);
  }
}
void MPEG::RenderFrame(int frame) {
  /* Prevent acces to the audio stream to avoid filling it */
  if( audiostream ) audiostream->enable(false);

  if ( VideoEnabled() ) {
    videoaction->RenderFrame(frame);
  }

  if( audiostream ) audiostream->enable(true);
}
void MPEG::RenderFinal(SDL_Surface *dst, int x, int y) {
  /* Prevent acces to the audio stream to avoid filling it */
  if( audiostream ) audiostream->enable(false);

  if ( VideoEnabled() ) {
    videoaction->RenderFinal(dst, x, y);
  }

  if( audiostream ) audiostream->enable(true);
}

SMPEG_Filter * MPEG::Filter(SMPEG_Filter * filter)
{
  if ( VideoEnabled() ) {
    return(videoaction->Filter(filter));
  }
}

void MPEG::Seek(int position)
{
  int was_playing = 0;

  /* Cannot seek past end of file */
  if(position > system->TotalSize()) return;
  
  /* get info whrether we need to restart playing at the end */
  if( Status() == MPEG_PLAYING )
    was_playing = 1;

  if(!seekIntoStream(position)) return;

  /* If we were playing and not rewind then play again */
  if (was_playing)
    Play();

  if (VideoEnabled() && !was_playing) 
    videoaction->RenderFrame(0);

  if ( pause && VideoEnabled() ) {
    videoaction->Pause();
  }
  if ( pause && AudioEnabled() ) {
    audioaction->Pause();
  }
}

bool MPEG::seekIntoStream(int position)
{
  double time;

  /* First we stop everything */
  Stop();

  /* Go to the desired position into file */
  if(!system->Seek(position)) return(false);

  /* Seek first aligned data */
  if(audiostream)
    while(audiostream->time() == -1)
      audiostream->next_packet();
  if(videostream)
    while(videostream->time() == -1)
      videostream->next_packet();

  /* And forget what we previouly buffered */
  if ( audioaction ) {
    audioaction->Rewind();
    audioaction->ResetSynchro(audiostream->time());
  }
  if ( videoaction ) {
    videoaction->Rewind();
    videoaction->ResetSynchro(videostream->time());
  }

  return(true);
}

void MPEG::Skip(float seconds)
{
  if(system->get_stream(SYSTEM_STREAMID))
  {
    system->Skip(seconds);
  }
  else
  {
    /* No system information in MPEG */
    if( VideoEnabled() ) videoaction->Skip(seconds);
    if( AudioEnabled() ) audioaction->Skip(seconds);
  }
}

void MPEG::GetSystemInfo(MPEG_SystemInfo * sinfo)
{
  sinfo->total_size = system->TotalSize();
  sinfo->current_offset = system->Tell();
  sinfo->total_time = system->TotalTime();

  /* Get current time from audio or video decoder */
  /* TODO: move timing reference in MPEGsystem    */
  sinfo->current_time = 0;
  if( videoaction ) 
    sinfo->current_time = videoaction->Time();
  if( audioaction )
    sinfo->current_time = audioaction->Time();
}

void MPEG::parse_stream_list()
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
    stream_list = system->GetStreamList();

    switch(stream_list[i]->streamid)
    {
      case SYSTEM_STREAMID:
      break;

      case AUDIO_STREAMID:
	audiostream = stream_list[i];
	audioaction_enabled = true;
	audiostream->next_packet();
	audio = new MPEGaudio(audiostream, sdlaudio);
	audioaction = audio;
      break;

      case VIDEO_STREAMID:
	videostream = stream_list[i];
	videoaction_enabled = true;
	videostream->next_packet();
	video = new MPEGvideo(videostream);
	videoaction = video;
      break;
    }

    i++;
  }
  while(stream_list[i]);
}
