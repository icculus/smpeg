
#include "MPEG.h"


MPEG::MPEG(Uint8 *Mpeg, Uint32 Size, Uint8 StreamID, bool sdlaudio) :
                                   MPEGstream(Mpeg, Size, StreamID)
{
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
                    audiostream = new MPEG(mpeg, size, data[1], sdlaudio);
                    if ( audiostream->WasError() ) {
                        SetError(audiostream->TheError());
                    }
                    audioaction = audiostream;
                    audioaction_enabled = true;
                } else
#endif
                if ( (data[1] == VIDEO_STREAMID) && !videostream ) {
                    videostream = new MPEG(mpeg, size, data[1], sdlaudio);
                    if ( videostream->WasError() ) {
                        SetError(videostream->TheError());
                    }
                    videoaction = videostream;
                    videoaction_enabled = true;
                }
                data += 3;
            }
            /* Hack to detect video streams that are not advertised */
            if ( ! videostream ) {
                if ( data[3] == 0xb3 ) {
                    videostream = new MPEG(mpeg,size, VIDEO_STREAMID, sdlaudio);
                    if ( videostream->WasError() ) {
                        SetError(videostream->TheError());
                    }
                    videoaction = videostream;
                    videoaction_enabled = true;
                }
            }
        }
        EnableAudio(audioaction_enabled);
        EnableVideo(videoaction_enabled);
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
        if ( audio->WasError() ) {
            SetError(audio->TheError());
        } else {
            audioaction = audio;
            audioaction_enabled = true;
        }
    } else
    if ( streamid == VIDEO_STREAMID ) {
        videostream = this;
        video = new MPEGvideo(videostream);
        if ( video->WasError() ) {
            SetError(video->TheError());
        } else {
            videoaction = video;
            videoaction_enabled = true;
        }
    }
    if ( ! audiostream && ! videostream ) {
        SetError("No audio/video stream found in MPEG");
    }
}

MPEG::~MPEG() {
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
