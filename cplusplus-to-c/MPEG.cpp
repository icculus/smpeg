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

bool MPEG_seekIntoStream(MPEG *self, int position);
void MPEG_parse_stream_list(MPEG *self);

MPEG *MPEG_create() {
		MPEG *ret = (MPEG *)malloc(sizeof(MPEG));

		if (ret) ret->err = MPEGerror_create();

		return ret;
}

MPEG *MPEG_create_file(const char * name, bool SDLaudio) {
		MPEG *ret = MPEG_create();

		mpeg_mem = 0;

		ret->source = SDL_RWFromFile(name, "rb");
		if (!ret->source) {
				MPEG_InitErrorState(ret);
				MPEGerror_SetError(ret->err, SDL_GetError());
				return;
		}
		MPEG_Init(ret, ret->source, SDLaudio);

		return ret;
}

MPEG *MPEG_create_fd(int Mpeg_FD, bool SDLaudio) {
		MPEG *ret = MPEG_create();

		mpeg_mem = 0;

		// *** FIXME we're leaking a bit of memory for the FILE *
		// best solution would be to have SDL_RWFromFD
		FILE *file = fdopen(Mpeg_FD, "rb");
		if (!file) {
				MPEG_InitErrorState(ret);
				MPEGerror_SetError(ret->err, strerror(errno));
				return;
		}

		ret->source = SDL_RWFromFP(file,false);
		if (!ret->source) {
				MPEG_InitErrorState(ret);
				MPEGerror_SetError(ret->err, SDL_GetError());
				return;
		}
		MPEG_Init(ret, ret->source, SDLaudio);

		return ret;
}

MPEG *MPEG_create_mem(void *data, int size, bool SDLaudio) {
		MPEG *ret = MPEG_create();

		// The semantics are that the data passed in should be copied
		// (?)
		ret->mpeg_mem = new char[size];
		memcpy(ret->mpeg_mem, data, size);

		ret->source = SDL_RWFromMem(ret->mpeg_mem, size);
		if (!ret->source) {
				MPEG_InitErrorState(ret);
				MPEGerror_SetError(ret->err, SDL_GetError());
				return;
		}
		MPEG_Init(ret, ret->source, SDLaudio);

		return ret;
}

MPEG *MPEG_create_rwops(SDL_RWops *mpeg_source, bool SDLaudio) {
		MPEG *ret = MPEG_create();

    ret->source = mpeg_source;
		ret->mpeg_mem = 0;
		MPEG_Init(ret, ret->source, SDLaudio);

		return ret;
}

void MPEG_Init(MPEG *self, SDL_RWops *mpeg_source, bool SDLaudio)
{
		/* Initialize everything to invalid values for cleanup */
    MPEG_InitErrorState(self);

    self->source = mpeg_source;
    self->sdlaudio = SDLaudio;

    /* Create the system that will parse the MPEG stream */
    self->system = MPEGsystem_create_rwops(source);

    MPEG_parse_stream_list(self);

    MPEG_EnableAudio(self, self->audioaction_enabled);
    MPEG_EnableVideo(self, self->videoaction_enabled);

    if ( ! self->audiostream && ! self->videostream ) {
      MPEGerror_SetError(self->err, "No audio/video stream found in MPEG");
    }

    if ( system && MPEGerror_WasError(system->err) ) {
      MPEGerror_SetError(self->err, MPEGerror_TheError(system->err));
    }

    if ( audio && MPEGerror_WasError(audio->err) ) {
      MPEGerror_SetError(self->err, MPEGerror_TheError(audio->err));
    }

    if ( video && MPEGerror_WasError(video->err) ) {
      MPEGerror_SetError(self->err, MPEGerror_TheError(video->err));
    }

    if ( MPEGerror_WasError(self->err) ) {
      MPEGerror_SetError(self->err, MPEGerror_TheError(self->err));
    }
}

void MPEG_InitErrorState(MPEG *self) {
    MPEGerror_ClearError(self->err);

		self->audio = NULL;
    self->video = NULL;
    self->system = NULL;
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

void MPEG_destroy(MPEG *self) {
		MPEG_Stop(self);
		if(self->video) delete self->video;
		if(self->audio) delete self->audio;
		if(self->system) MPEGsystem_destroy(self->system);

		if(self->source) SDL_RWclose(self->source);
		if ( self->mpeg_mem )
				delete[] self->mpeg_mem;

		free(self);
}

bool MPEG_AudioEnabled(MPEG *self) {
		return(self->audioaction_enabled);
}

void MPEG_EnableAudio(MPEG *self, bool enabled) {
		if ( enabled && ! self->audioaction ) {
				enabled = false;
		}
		self->audioaction_enabled = enabled;

		/* Stop currently playing stream, if necessary */
		if ( self->audioaction && ! self->audioaction_enabled ) {
				self->audioaction->Stop();
		}
		/* Set the video time source */
		if ( self->videoaction ) {
				if ( self->audioaction_enabled ) {
						self->videoaction->SetTimeSource(audioaction);
				} else {
						self->videoaction->SetTimeSource(NULL);
				}
		}
		if(self->audiostream)
				self->audiostream->enable(enabled);
}
bool MPEG_VideoEnabled(MPEG *self) {
		return(self->videoaction_enabled);
}
void MPEG_EnableVideo(MPEG *self, bool enabled) {
		if ( enabled && ! self->videoaction ) {
				enabled = false;
		}
		self->videoaction_enabled = enabled;

		/* Stop currently playing stream, if necessary */
		if ( self->videoaction && ! self->videoaction_enabled ) {
				self->videoaction->Stop();
		}
		if(self->videostream)
				self->videostream->enable(enabled);
}

/* MPEG actions */
void MPEG_Loop(MPEG *self, bool toggle) {
		self->loop = toggle;
}
void MPEG_Play(MPEG *self) {
		if ( MPEG_AudioEnabled(self) ) {
				self->audioaction->Play();
		}
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->Play();
		}
}
void MPEG_Stop(MPEG *self) {
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->Stop();
		}
		if ( MPEG_AudioEnabled(self) ) {
				self->audioaction->Stop();
		}
}

void MPEG_Rewind(MPEG *self) {
		self->seekIntoStream(0);
}

void MPEG_Pause(MPEG *self) {
		self->pause = !self->pause;

		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->Pause();
		}
		if ( MPEG_AudioEnabled(self) ) {
				self->audioaction->Pause();
		}
}

/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
MPEGstatus MPEG_GetStatus(MPEG *self) {
		MPEGstatus status;

		status = MPEG_STOPPED;
		if ( MPEG_VideoEnabled(self) ) {
				/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
				switch (self->videoaction->GetStatus()) {
				case MPEG_PLAYING:
						status = MPEG_PLAYING;
						break;
				default:
						break;
				}
		}
		if ( MPEG_AudioEnabled(self) ) {
				/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
				switch (self->audioaction->GetStatus()) {
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
				MPEG_Rewind(self);
				MPEG_Play(self);

				if ( MPEG_VideoEnabled(self) ) {
						/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
						switch (self->videoaction->GetStatus()) {
						case MPEG_PLAYING:
								status = MPEG_PLAYING;
								break;
						default:
								break;
						}
				}
				if ( MPEG_AudioEnabled(self) ) {
						/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
						switch (self->audioaction->GetStatus()) {
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
bool MPEG_GetAudioInfo(MPEG *self, MPEG_AudioInfo *info) {
		if ( MPEG_AudioEnabled(self) ) {
				return(self->audioaction->GetAudioInfo(info));
		}
		return(false);
}
void MPEG_Volume(MPEG *self, int vol) {
		if ( MPEG_AudioEnabled(self) ) {
				self->audioaction->Volume(vol);
		}
}
bool MPEG_WantedSpec(MPEG *self, SDL_AudioSpec *wanted) {
		if( self->audiostream ) {
				return(GetAudio(self)->WantedSpec(wanted));
		}
		return(false);
}
void MPEG_ActualSpec(MPEG *self, const SDL_AudioSpec *actual) {
		if( self->audiostream ) {
				GetAudio(self)->ActualSpec(actual);
		}
}
MPEGaudio *MPEG_GetAudio(MPEG *self) { // Simple accessor used in the C interface
		return self->audio;
}

/* MPEG video actions */
bool MPEG_GetVideoInfo(MPEG *self, MPEG_VideoInfo *info) {
		if ( MPEG_VideoEnabled(self) ) {
				return(self->videoaction->GetVideoInfo(info));
		}
		return(false);
}
bool MPEG_SetDisplay(MPEG *self, SDL_Surface *dst, SDL_Mutex *lock,
										 MPEG_DisplayCallback callback) {
		if ( MPEG_VideoEnabled(self) ) {
				return(self->videoaction->SetDisplay(dst, lock, callback));
		}
		return(false);
}
void MPEG_MoveDisplay(MPEG *self, int x, int y) {
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->MoveDisplay(x, y);
		}
}
void MPEG_ScaleDisplayXY(MPEG *self, int w, int h) {
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->ScaleDisplayXY(w, h);
		}
}
void MPEG_SetDisplayRegion(MPEG *self, int x, int y, int w, int h) {
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->SetDisplayRegion(x, y, w, h);
		}
}
void MPEG_RenderFrame(MPEG *self, int frame)
{
		if ( MPEG_VideoEnabled(self) ) {
				self->videoaction->RenderFrame(frame);
		}
}
void MPEG_RenderFinal(MPEG *self, SDL_Surface *dst, int x, int y)
{
    MPEG_Stop(self);
    if ( MPEG_VideoEnabled(self) ) {
        self->videoaction->RenderFinal(dst, x, y);
    }
    MPEG_Rewind(self);
}

SMPEG_Filter * MPEG_Filter(MPEG *self, SMPEG_Filter * filter)
{
		if ( MPEG_VideoEnabled(self) ) {
				return(self->videoaction->Filter(filter));
		}
		return 0;
}

void MPEG_Seek(MPEG *self, int position)
{
		int was_playing = 0;

		/* Cannot seek past end of file */
		if((Uint32)position > self->system->TotalSize()) return;

		/* Michel Darricau from eProcess <mdarricau@eprocess.fr> conflict name with popcorn */
		/* get info whrether we need to restart playing at the end */
		if( MPEG_GetStatus(self) == MPEG_PLAYING )
				was_playing = 1;

		if(!MPEG_seekIntoStream(self, position)) return;

		/* If we were playing and not rewind then play again */
		if (was_playing)
				MPEG_Play(self);

		if (MPEG_VideoEnabled(self) && !was_playing)
				self->videoaction->RenderFrame(0);

		if ( self->pause && MPEG_VideoEnabled(self) ) {
				self->videoaction->Pause();
		}
		if ( self->pause && MPEG_AudioEnabled(self) ) {
				self->audioaction->Pause();
		}
}

bool MPEG_seekIntoStream(MPEG *self, int position)
{
		/* First we stop everything */
		MPEG_Stop(self);

		/* Go to the desired position into file */
		if(!self->system->Seek(position)) return(false);

		/* Seek first aligned data */
		if(self->audiostream && self->audioaction_enabled)
				while(self->audiostream->time() == -1)
						if ( ! self->audiostream->next_packet() ) return false;
		if(self->videostream && self->videoaction_enabled)
				while(self->videostream->time() == -1)
						if ( ! self->videostream->next_packet() ) return false;

		/* Calculating current play time on audio only makes sense when there
		 is no video */
		if ( self->audioaction && !self->videoaction) {
				self->audioaction->Rewind();
				self->audioaction->ResetSynchro(system->TimeElapsedAudio(position));
		}
		/* And forget what we previouly buffered */
		else if ( self->audioaction ) {
				self->audioaction->Rewind();
				self->audioaction->ResetSynchro(audiostream->time());
		}
		if ( self->videoaction ) {
				self->videoaction->Rewind();
				self->videoaction->ResetSynchro(videostream->time());
		}

		return(true);
}

void MPEG_Skip(MPEG *self, float seconds)
{
		if(self->system->get_stream(SYSTEM_STREAMID))
		{
				self->system->Skip(seconds);
		}
		else
		{
				/* No system information in MPEG */
				if( MPEG_VideoEnabled(self) ) self->videoaction->Skip(seconds);
				if( MPEG_AudioEnabled(self) ) self->audioaction->Skip(seconds);
		}
}

void MPEG_GetSystemInfo(MPEG *self, MPEG_SystemInfo * sinfo)
{
		sinfo->total_size = self->system->TotalSize();
		sinfo->current_offset = self->system->Tell();
		sinfo->total_time = self->system->TotalTime();

		/* Get current time from audio or video decoder */
		/* TODO: move timing reference in MPEGsystem    */
		sinfo->current_time = 0;
		if( self->videoaction )
				sinfo->current_time = self->videoaction->Time();
		if( self->audioaction )
				sinfo->current_time = self->audioaction->Time();
}

void MPEG_parse_stream_list(MPEG *self)
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
				stream_list = self->system->GetStreamList();

				switch(stream_list[i]->streamid)
				{
				case SYSTEM_STREAMID:
						break;

				case AUDIO_STREAMID:
						self->audiostream = stream_list[i];
						self->audioaction_enabled = true;
						self->audiostream->next_packet();
						self->audio = new MPEGaudio(audiostream, sdlaudio);
						self->audioaction = audio;
						break;

				case VIDEO_STREAMID:
						self->videostream = stream_list[i];
						self->videoaction_enabled = true;
						self->videostream->next_packet();
						self->video = new MPEGvideo(videostream);
						self->videoaction = video;
						break;
				}

				i++;
		}
		while(stream_list[i]);
}
