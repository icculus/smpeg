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

/* A virtual class to provide basic MPEG playback actions */


#include "SDL.h"
#include "MPEGfilter.h"
#include "MPEGaction.h"


#undef _THIS
#define _THIS MPEGaction *self
#define METH(m) MPEGaction_##m


/* base methods. */


MPEGaction *METH(init) (_THIS)
{
  MAKE_OBJECT(MPEGaction);
  self->playing = false;
  self->paused = false;
  self->looping = false;
  self->play_time = 0.0;
  return self;
}

void METH(destroy) (_THIS)
{
  self->playing = false;
  self->paused = false;
  self->looping = false;
  self->play_time = 0.0;
}

void METH(ResetPause) (_THIS)
{
  self->paused = false;
}

/*virtual*/ void METH(Loop) (_THIS, bool toggle)
{
  self->looping = toggle;
}

/* Returns the time in seconds since start */
/*virtual*/ double METH(Time) (_THIS)
{
  return self->play_time;
}

/*virtual*/ double METH(Play) (_THIS)
{
  return 0;
}

/*virtual*/ double METH(Stop) (_THIS)
{
  return 0;
}

/*virtual*/ double METH(Rewind) (_THIS)
{
  return 0;
}

/*virtual*/ double METH(ResetSynchro) (_THIS, double time)
{
  return 0;
}

/*virtual*/ double METH(Skip) (_THIS, float seconds)
{
  return 0;
}

/* A toggle action */
/*virtual*/ void METH(Pause) (_THIS)
{
#if 0
        if ( paused ) {
            paused = false;
            Play();
        } else {
            Stop();
            paused = true;
        }
    }
#endif /* 0 */

};


