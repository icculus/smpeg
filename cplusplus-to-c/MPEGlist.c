#include "MPEGlist.h"
#include "MPEGerror.h"


/*
2LL
*/

#undef _THIS
#define _THIS MPEGlist *self
#undef METH
#define METH(m) MPEGlist_##m

MPEGlist *
METH(init) (_THIS)
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
METH(destroy) (_THIS)
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
METH(Alloc) (_THIS, Uint32 bufsize)
{
  MPEGlist *tmp;

  tmp = self->next;
  self->next = MPEGlist_init(NULL);
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
METH(Lock) (_THIS)
{
  self->lock++;
}

/* Unlock current buffer */
void
METH(Unlock) (_THIS)
{
  if (self->lock != 0)
      self->lock--;
}

void*
METH(Buffer) (_THIS)
{
  return self->data;
}

Uint32
METH(Size) (_THIS)
{
  return self->size;
}

MPEGlist*
METH(Next) (_THIS)
{
  return self->next;
}

MPEGlist*
METH(Prev) (_THIS)
{
  return self->prev;
}

Uint32
METH(IsLocked) (_THIS)
{
  return self->lock;
}

