/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

/* A class used to parse and play MPEG streams */

#ifndef _MPEG_H_
#define _MPEG_H_

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <SDL/SDL.h>

#include "MPEGerror.h"
#include "MPEGstream.h"
#include "MPEGaction.h"
#include "MPEGaudio.h"
#include "MPEGvideo.h"

/* The main MPEG class - parses system streams and creates other streams
 A few design notes:
   Making this derived from MPEGstream allows us to do system stream
   parsing.  We create an additional MPEG object for each type of 
   stream in the MPEG file because each needs a separate pointer to
   the MPEG data.  The MPEG stream then creates an accessor object to
   do all the data parsing for that stream type.  It's a little odd,
   but seemed like the best way do implement stream parsing.
 */
class MPEG : public MPEGstream, public MPEGaudioaction,public MPEGvideoaction {
public:
    MPEG(Uint8 *Mpeg, Uint32 Size, Uint8 StreamID = 0, bool sdlaudio = true) :
                        MPEGstream(Mpeg, Size, StreamID) {
        audiostream = NULL; audio = NULL; audioaction = NULL;
        audioaction_enabled = false;
        videostream = NULL; video = NULL; videoaction = NULL;
        videoaction_enabled = false;
        if ( streamid == SYSTEM_STREAMID ) {
            /* Do special parsing to find out which MPEG streams to create */
            while ( next_packet(false) ) {
                data += 6;
                while ( data[0] & 0x80 ) {
#ifndef PROFILE_VIDEO
                    if ( (data[1] == AUDIO_STREAMID) && !audiostream ) {
                        audiostream = new MPEG(mpeg, size, data[1]);
                        if ( audiostream->WasError() ) {
                            SetError(audiostream->TheError());
                        }
                        audioaction = audiostream;
                        audioaction_enabled = true;
                    } else
#endif
                    if ( (data[1] == VIDEO_STREAMID) && !videostream ) {
                        videostream = new MPEG(mpeg, size, data[1]);
                        if ( videostream->WasError() ) {
                            SetError(videostream->TheError());
                        }
                        videoaction = videostream;
                        videoaction_enabled = true;
                    }
                    data += 3;
                }
            }
            reset_stream();
        }
        /* Determine if we are reading a system layer, and get first packet */
        if ( mpeg[3] == 0xba ) {
            next_packet();
        } else {
            packet = mpeg;
            packetlen = size;
            data = packet;
            stop = data + packetlen;
        }
        if ( streamid == AUDIO_STREAMID ) {
            audiostream = this;
            audio = new MPEGaudio(audiostream, sdlaudio);
            audioaction = audio;
            audioaction_enabled = true;
        } else
        if ( streamid == VIDEO_STREAMID ) {
            videostream = this;
            video = new MPEGvideo(videostream);
            videoaction = video;
            videoaction_enabled = true;
        }
        if ( ! audiostream && ! videostream ) {
            SetError("No audio/video stream found in MPEG");
        }
    }
    virtual ~MPEG() {
        if ( audiostream ) {
            if ( audiostream == this ) {
                delete audio;
            } else {
                delete audiostream;
            }
        }
        if ( videostream ) {
            if ( videostream == this ) {
                delete video;
            } else {
                delete videostream;
            }
        }
    }

    /* Enable/Disable audio and video */
    bool AudioEnabled(void) {
        return(audioaction_enabled);
    }
    void EnableAudio(bool enabled) {
        if ( enabled && ! audioaction ) {
            enabled = false;
        }
        audioaction_enabled = enabled;

        /* Stop currently playing stream, if necessary */
        if ( audioaction && ! audioaction_enabled ) {
            audioaction->Stop();
        } 
    }
    bool VideoEnabled(void) {
        return(videoaction_enabled);
    }
    void EnableVideo(bool enabled) {
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
    void Loop(bool toggle) {
        if ( VideoEnabled() ) {
            videoaction->Loop(toggle);
        }
        if ( AudioEnabled() ) {
            audioaction->Loop(toggle);
        }
    }
    void Play(void) {
        if ( VideoEnabled() ) {
            videoaction->Play();
        }
        if ( AudioEnabled() ) {
            audioaction->Play();
        }
    }
    void Stop(void) {
        if ( VideoEnabled() ) {
            videoaction->Stop();
        }
        if ( AudioEnabled() ) {
            audioaction->Stop();
        }
    }
    void Rewind(void) {
        if ( VideoEnabled() ) {
            videoaction->Rewind();
        }
        if ( AudioEnabled() ) {
            audioaction->Rewind();
        }
    }
    void Pause(void) {
        if ( VideoEnabled() ) {
            videoaction->Pause();
        }
        if ( AudioEnabled() ) {
            audioaction->Pause();
        }
    }
    MPEGstatus Status(void) {
        MPEGstatus status;

        status = MPEG_STOPPED;
        if ( VideoEnabled() ) {
            switch (videoaction->Status()) {
                case MPEG_PLAYING:
                    status = MPEG_PLAYING;
                    break;
            }
        }
        if ( AudioEnabled() ) {
            switch (audioaction->Status()) {
                case MPEG_PLAYING:
                    status = MPEG_PLAYING;
                    break;
            }
        }
        return(status);
    }

    /* MPEG audio actions */
    bool GetAudioInfo(MPEG_AudioInfo *info) {
        if ( AudioEnabled() ) {
            return(audioaction->GetAudioInfo(info));
        }
        return(false);
    }
    void Volume(int vol) {
        if ( AudioEnabled() ) {
            return(audioaction->Volume(vol));
        }
    }
	bool WantedSpec(SDL_AudioSpec *wanted) {
		if( AudioEnabled() && audio ) {
			return(audio->WantedSpec(wanted));
		}
		return(false);
	}
	void ActualSpec(const SDL_AudioSpec *actual) {
		if( AudioEnabled() && audio ) {
			audio->ActualSpec(actual);
		}
	}
	MPEGaudio *GetAudio(void) { // Simple accessor used in the C interface
		return audio;
	}

    /* MPEG video actions */
    bool GetVideoInfo(MPEG_VideoInfo *info) {
        if ( VideoEnabled() ) {
            return(videoaction->GetVideoInfo(info));
        }
        return(false);
    }
    bool SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
                                MPEG_DisplayCallback callback) {
        if ( VideoEnabled() ) {
            return(videoaction->SetDisplay(dst, lock, callback));
        }
        return(false);
    }
    void MoveDisplay(int x, int y) {
        if ( VideoEnabled() ) {
            videoaction->MoveDisplay(x, y);
        }
    }
    void DoubleDisplay(bool toggle) {
        if ( VideoEnabled() ) {
            videoaction->DoubleDisplay(toggle);
        }
    }
    void RenderFrame(int frame, SDL_Surface *dst, int x, int y) {
        if ( VideoEnabled() ) {
            videoaction->RenderFrame(frame, dst, x, y);
        }
    }
    void RenderFinal(SDL_Surface *dst, int x, int y) {
        if ( VideoEnabled() ) {
            videoaction->RenderFinal(dst, x, y);
        }
    }

protected:
    /* We need to have separate audio and video streams */
public:
    MPEG *audiostream;
    MPEG *videostream;

protected:
    MPEGaudio *audio;
    MPEGvideo *video;
    MPEGaudioaction *audioaction;
    bool audioaction_enabled;
    MPEGvideoaction *videoaction;
    bool videoaction_enabled;
};

/* This class is system dependent in the way it uses memory mapping */
#ifdef unix
#include <stdio.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>

class MPEGfile : public MPEGerror,
                 public MPEGaudioaction, public MPEGvideoaction {
public:
    MPEGfile(const char *file, bool sdlaudio = true) {
        FILE *newfp;
        newfp = fopen(file, "rb");
        Init(newfp, true, sdlaudio);
    }
    MPEGfile(FILE *MPEG_fp, bool autoclose = false) {
        Init(MPEG_fp, autoclose);
    }
    void Init(FILE *MPEG_fp, bool autoclose, bool sdlaudio = true) {
        /* Initialize everything to invalid values for cleanup */
        mpeg_fp = MPEG_fp;
        mpeg_area = (caddr_t)-1;
        mpeg = NULL;
        error = NULL;

        /* Memory map the file and create an MPEG object */
        if ( MPEG_fp ) {
            struct stat statb;

            mpeg_fp = MPEG_fp;
            if ( fstat(fileno(mpeg_fp), &statb) == 0 ) {
                mpeg_size = statb.st_size;
                mpeg_area = mmap(NULL, mpeg_size, PROT_READ, MAP_SHARED,
                                                     fileno(mpeg_fp), 0);
                if ( mpeg_area != (caddr_t)-1 ) {
                    mpeg = new MPEG((Uint8 *)mpeg_area, mpeg_size, 0, sdlaudio);
                    if ( mpeg->WasError() ) {
                        SetError(mpeg->TheError());
                        delete mpeg;
                        mpeg = NULL;
                    }
                } else {
                    SetError("Memory map of MPEG failed");
                }
            } else {
                SetError("Unable to stat() MPEG file");
            }
        } else {
            SetError("Unable to open MPEG file");
        }
        close_fp = autoclose;
    }
    virtual ~MPEGfile() {
        if ( mpeg ) {
            delete mpeg;
        }
        if ( mpeg_area != (caddr_t)-1 ) {
            munmap((caddr_t)mpeg_area, mpeg_size);
        }
        if ( close_fp && mpeg_fp ) {
            fclose(mpeg_fp);
        }
    }

    /* Enable/Disable audio and video */
    bool AudioEnabled(void) {
        if ( mpeg ) {
            return(mpeg->AudioEnabled());
        }
        return(false);
    }
    void EnableAudio(bool enabled) {
        if ( mpeg ) mpeg->EnableAudio(enabled);
    }
    bool VideoEnabled(void) {
        if ( mpeg ) {
            return(mpeg->VideoEnabled());
        }
        return(false);
    }
    void EnableVideo(bool enabled) {
        if ( mpeg ) mpeg->EnableVideo(enabled);
    }

    /* MPEG actions */
    void Loop(bool toggle) {
        if ( mpeg ) mpeg->Loop(toggle);
    }
    void Play(void) {
        if ( mpeg ) mpeg->Play();
    }
    void Stop(void) {
        if ( mpeg ) mpeg->Stop();
    }
    void Rewind(void) {
        if ( mpeg ) mpeg->Rewind();
    }
    void Pause(void) {
        if ( mpeg ) mpeg->Pause();
    }
    MPEGstatus Status(void) {
        MPEGstatus status;

        status = MPEG_ERROR;
        if ( mpeg ) {
            status = mpeg->Status();
        }
        return(status);
    }

    /* MPEG audio actions */
    bool GetAudioInfo(MPEG_AudioInfo *info) {
        if ( mpeg ) {
            return(mpeg->GetAudioInfo(info));
        }
        return(false);
    }
    void Volume(int vol) {
        if ( mpeg ) {
            return(mpeg->Volume(vol));
        }
    }
	bool WantedSpec(SDL_AudioSpec *wanted) {
		if( mpeg ) {
			return(mpeg->WantedSpec(wanted));
		}
	}
	void ActualSpec(const SDL_AudioSpec *actual) {
		if( mpeg ) {
			mpeg->ActualSpec(actual);
		}
	}
	MPEGaudio *GetAudio(void) {
		if( mpeg ) {
			return mpeg->GetAudio();
		}
	}

    /* MPEG video actions */
    bool GetVideoInfo(MPEG_VideoInfo *info) {
        if ( mpeg ) {
            return(mpeg->GetVideoInfo(info));
        }
        return(false);
    }
    bool SetDisplay(SDL_Surface *dst, SDL_mutex *lock,
                                MPEG_DisplayCallback callback) {
        if ( mpeg ) {
            return(mpeg->SetDisplay(dst, lock, callback));
        }
        return(false);
    }
    void MoveDisplay(int x, int y) {
        if ( mpeg ) {
            mpeg->MoveDisplay(x, y);
        }
    }
    void DoubleDisplay(bool toggle) {
        if ( mpeg ) {
            mpeg->DoubleDisplay(toggle);
        }
    }
    void RenderFrame(int frame, SDL_Surface *dst, int x, int y) {
        if ( mpeg ) {
            mpeg->RenderFrame(frame, dst, x, y);
        }
    }
    void RenderFinal(SDL_Surface *dst, int x, int y) {
        if ( mpeg ) {
            mpeg->RenderFinal(dst, x, y);
        }
    }

protected:
    FILE *mpeg_fp;
    bool close_fp;
    void  *mpeg_area;
    size_t mpeg_size;
public:
    MPEG *mpeg;
};
#else
#error Non-mmap implementation not completed
#endif /* unix */

#endif /* _MPEG_H_ */
