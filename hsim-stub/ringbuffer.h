/* Circular buffer example, keeps one slot open */

#include <stdio.h>
#include <stdlib.h>
#include "esim_type.h"

#define BUFFER_SIZE 100

/* Circular buffer object */
typedef struct
{
  volatile int start;			/* index of oldest element              */
  volatile int end;			/* index at which to write new element  */
  volatile packet elems[BUFFER_SIZE+1];		/* vector of elements                   */
} CircularBuffer;

void cbInit (CircularBuffer * cb);
void cbFree (CircularBuffer * cb);
int cbIsFull (CircularBuffer * cb);
int cbIsEmpty (CircularBuffer * cb);
void cbWrite (CircularBuffer * cb, packet * elem);
void cbRead (CircularBuffer * cb, packet * elem);

void cbInit (CircularBuffer * cb)
{
  cb->start = 0;
  cb->end = 0;
}

void cbFree (CircularBuffer * cb)
{
	//do nothing
}

int cbIsFull (CircularBuffer * cb)
{
  return (cb->end + 1) % BUFFER_SIZE == cb->start;
}

int cbIsEmpty (CircularBuffer * cb)
{
  return cb->end == cb->start;
}


/* Write an element, overwriting oldest element if buffer is full. App can
 *    choose to avoid the overwrite by checking cbIsFull(). */
void cbWrite (CircularBuffer * cb, packet * elem)
{
  cb->elems[cb->end] = *elem;
  cb->end = (cb->end + 1) % BUFFER_SIZE;
 // if (cb->end == cb->start)
   // cb->start = (cb->start + 1) % BUFFER_SIZE;	/* full, overwrite */
}

/* Read oldest element. App must ensure !cbIsEmpty() first. */
void cbRead (CircularBuffer * cb, packet * elem)
{
  *elem = cb->elems[cb->start];
  cb->start = (cb->start + 1) % BUFFER_SIZE;
}


