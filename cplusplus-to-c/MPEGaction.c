/*
    SMPEG - SDL MPEG Player Library
    Copyright (C) 1999  Loki Entertainment Software
    
    - Modified by Michel Darricau from eProcess <mdarricau@eprocess.fr>  for popcorn -

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

#include "MPEGaction.h"

MPEGaction *MPEGaction_create(void *parent, void (*play)(void *), void (*stop)(void *)) {
    MPEGaction *ret;

    ret = (MPEGaction *)malloc(sizeof(MPEGaction));

    if (ret) {
	ret->parent = parent;
	ret->play = play;
        ret->stop = stop;
	ret->playing = false;
	ret->paused = false;
	ret->looping = false;
	ret->play_time = 0.0;
    }

    return ret;
}

void MPEGaction_Loop(MPEGaction *self, bool toggle) {
    self->looping = toggle;
}

double MPEGaction_Time(MPEGaction *self, bool toggle) {
    return self->play_time;
}

void MPEGaction_Pause(MPEGaction *self) {
    if (!self->play || !self->stop || !self->parent) return;

    if ( self->paused ) {
	self->paused = false;
	self->play(self->parent);
    } else {
	self->stop(self->parent);
	self->paused = true;
    }
}

void MPEGaction_ResetPause(MPEGaction *self) {
    self->paused = false;
}

MPEGaudioaction *MPEGaudioaction_create(void *parent, void (*play)(void *), void (*stop)(void *)) {
    MPEGaudioaction *ret;

    ret = (MPEGaudioaction *)malloc(sizeof(MPEGaudioaction));

    if (ret)
	ret->act = MPEGaction_create(parent, play, stop);

    return ret;
}

bool MPEGaudioaction_GetAudioInfo(MPEGvideoaction *self, MPEG_AudioInfo *info) {
    return true;
}

void MPEGvideoaction_SetTimeSource(MPEGvideoaction *self, MPEGaudioaction *source) {
    self->time_source = source;
}

bool MPEGvideoaction_GetVideoInfo(MPEGvideoaction *self, MPEG_VideoInfo *info) {
    return false;
}
