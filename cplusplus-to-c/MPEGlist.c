/*
Doubly-linked list.
*/

#include <stdlib.h>
#include <string.h>

#include "MPEGlist.h"
#include "MPEGerror.h"


#undef _THIS
#define _THIS MPEGlist *self

MPEGlist *
MPEGlist_init (_THIS)
{
  MAKE_OBJECT(MPEGlist);
//  self = (MPEGlist*)malloc(sizeof(MPEGlist));
  self->size = 0;
  self->data = 0;
  self->lock = 0;
  self->next = 0;
  self->prev = 0;
  self->TimeStamp = -1;
  return self;
}

void
MPEGlist_destroy (_THIS)
{
  if (self->next) self->next->prev = self->prev;
  if (self->prev) self->prev->next = self->next;
  if (self->data)
    {
      free(self->data);
      self->data = 0;
    }
}

void
MPEGlist_delete (_THIS)
{
  MPEGlist_destroy (self);
  free(self);
}


/* Return the next free buffer or allocate a new one if none is empty */
MPEGlist *
MPEGlist_Alloc (_THIS, Uint32 bufsize)
{
  MPEGlist *tmp;

  tmp = self->next;
  self->next = MPEGlist_init(NULL);
  self->next->next = tmp;

  if (bufsize)
    {
      self->next->data = (Uint8*)malloc(bufsize);
      if (!self->next->data)
        {
          fprintf(stderr, "Alloc : Not enough memory\n");
          return 0;
        }
    }
  else
    {
      self->next->data = 0;
    }

  self->next->size = bufsize;
  self->next->prev = self;

  return self->next;
}

/* Lock current buffer */
void
MPEGlist_Lock (_THIS)
{
  self->lock++;
}

/* Unlock current buffer */
void
MPEGlist_Unlock (_THIS)
{
  if (self->lock != 0)
      self->lock--;
}

void*
MPEGlist_Buffer (_THIS)
{
  return self->data;
}

Uint32
MPEGlist_Size (_THIS)
{
  return self->size;
}

MPEGlist*
MPEGlist_Next (_THIS)
{
  return self->next;
}

MPEGlist*
MPEGlist_Prev (_THIS)
{
  return self->prev;
}

Uint32
MPEGlist_IsLocked (_THIS)
{
  return self->lock;
}

