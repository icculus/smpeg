#include "MPEGlist.h"

MPEGlist::MPEGlist()
{
  size = 0;
  data = 0;
  lock = 0;
  next = 0;
  prev = 0;
}

MPEGlist::~MPEGlist()
{
  if(next) next->prev = prev;
  if(prev) prev->next = next;
  if(data)
  {
    delete data;
    data = 0;
  }
}

/* Return the next free buffer or allocate a new one if none is empty */
MPEGlist * MPEGlist::Alloc(Uint32 Buffer_Size)
{
  MPEGlist * tmp;

  tmp = next;

  next = new MPEGlist;

  next->next = tmp;
  next->data = new Uint8[Buffer_Size];
  next->size = Buffer_Size;
  next->prev = this;

  if(!next->data)
  {
    fprintf(stderr, "Alloc : Not enough memory\n");
    exit(0);
  }

  return(next);
}

/* Lock current buffer */
void MPEGlist::Lock()
{
  lock++;
}

/* Unlock current buffer */
void MPEGlist::Unlock()
{
  if(lock != 0) lock--;
}
