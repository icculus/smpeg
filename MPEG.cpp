#include "SDL.h"

#include "MPEG.h"

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>
#define O_BINARY 0
#endif
#include <fcntl.h>
#include <string.h>
#include <errno.h>

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

MPEG::~MPEG()
{
  if(audio) delete audio;
  if(video) delete video;

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
  if ( VideoEnabled() ) {
    videoaction->Play();
  }
  if ( AudioEnabled() ) {
    audioaction->Play();
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
  Stop();

  /* Go to the beginning of the file */
  system->Rewind();

  /* Skip the first empty buffer made when creating a mpegstream */
  /* which would otherwise be interpreted as end of file */
  if(audiostream) audiostream->next_packet();
  if(videostream) videostream->next_packet();

  if ( AudioEnabled() ) {
    audioaction->Rewind();
  }

  if ( VideoEnabled() ) {
    videoaction->Rewind();
  }
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

void MPEG::Skip(float seconds)
{
  if ( AudioEnabled() ) {
    audioaction->Skip(seconds);
  }

  if ( VideoEnabled() ) {
    videoaction->Skip(seconds);
  }
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
