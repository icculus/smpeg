#include "MPEGlist.h"

MPEGlist *
MPEGlist_create ()
{
  MPEGlist *ret;

  ret = (MPEGlist*)malloc(sizeof(MPEGlist));
  ret->size = 0;
  ret->data = 0;
  ret->lock = 0;
  ret->next = 0;
  ret->prev = 0;
  ret->TimeStamp = -1;
  return ret;
}

void
MPEGlist_destroy (MPEGlist *self)
{
  if (self->next) self->next->prev = self->prev;
  if (self->prev) self->prev->next = self->next;
  if (self->data)
    {
      free(self->data);
      self->data = 0;
    }
}


/* Return the next free buffer or allocate a new one if none is empty */
MPEGlist *
MPEGlist_Alloc (MPEGlist *self, Uint32 bufsize)
{
  MPEGlist *tmp;

  tmp = self->next;
  self->next = MPEGlist_create();
  self->next->next = tmp;

  if (bufsize)
    {
      self->next->data = (Uint8*)malloc(bufsize);  /* XXX: of what type? */
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
MPEGlist_lock (MPEGlist *self)
{
  self->lock++;
}

/* Unlock current buffer */
void
MPEGlist_unlock (MPEGlist *self)
{
  if (self->lock != 0)
      self->lock--;
}

void*
MPEGlist_Buffer (MPEGlist *self)
{
  return self->data;
}

Uint32
MPEGlist_Size (MPEGlist *self)
{
  return self->size;
}

MPEGlist*
MPEGlist_Next (MPEGlist *self)
{
  return self->next;
}

MPEGlist*
MPEGlist_Prev (MPEGlist *self)
{
  return self->prev;
}

Uint32
MPEGlist_IsLocked (MPEGlist *self)
{
  return self->lock;
}

