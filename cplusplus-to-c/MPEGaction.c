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

/* XXX: into a .h? */
//#define false 0
//#define true (!false)
//#define bool int

#undef _THIS
#define _THIS MPEGaction *self

void
MPEGaction_Loop (_THIS, bool toggle)
{
  self->looping = toggle;
}

double
MPEGaction_Time (_THIS)
{
  return self->play_time;
}

#define DEFMETH(name) MPEGaction_##name
void DEFMETH(Play) (_THIS) { }
void DEFMETH(Stop) (_THIS) { }
void DEFMETH(Rewind) (_THIS) { }
void DEFMETH(Skip) (_THIS, float seconds) { }

void
MPEGaction_Pause (_THIS)
{
  if (self->paused)
    {
      self->paused = false;
      self->Play(self);
    }
  else
    {
      self->Stop(self);
      self->paused = true;
    }
}

MPEGstatus
MPEGaction_GetStatus (_THIS)
{
  return MPEG_ERROR;
}

void
MPEGaction_ResetPause (_THIS)
{
  self->paused = false;
}

MPEGaction *
MPEGaction_init (_THIS)
{
  self->playing = false;
  self->paused = false;
  self->looping = false;
  self->play_time = 0.0;
  self->Loop = MPEGaction_Loop;
  self->Time = MPEGaction_Time;
  self->Play = MPEGaction_Play;
  self->Stop = MPEGaction_Stop;
  self->Rewind = MPEGaction_Rewind;
  self->Skip = MPEGaction_Skip;
  self->Pause = MPEGaction_Pause;
  self->GetStatus = MPEGaction_GetStatus;
  self->ResetPause = MPEGaction_ResetPause;
}

MPEGaction *
MPEGaction_new ()
{
  MPEGaction *self;

  self = (MPEGaction*)calloc(sizeof(MPEGaction));
  MPEGaction_init(self);
  return self;
}






bool
MPEGaudioaction_GetAudioInfo (_THIS, MPEG_AudioInfo *info)
{
  return (true);
}

void
MPEGaudioaction_Volume (_THIS, int vol)
{
}

void
MPEGaudioaction_init (_THIS)
{
  MPEGaction_init(self);
  self->GetAudioInfo = MPEGaudioaction_GetAudioInfo;
  self->Volume = MPEGaudioaction_Volume;
}

MPEGaction *
MPEGaudioaction_new ()
{
  MPEGaction *self;

  self = MPEGaction_new();
  MPEGaction_init(self);
  MPEGaudioaction_init(self);
  return self;
}

