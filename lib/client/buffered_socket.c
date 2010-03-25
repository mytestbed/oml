/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*!\file buffered_socket.c
  \brief Implements a non-blocking FIFO queue in-front of a socket
*/

#include <stdlib.h>
#include <pthread.h>
#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <errno.h>
#include <string.h>
#include <oml2/omlc.h>

#include "client.h"

#define MAX_BUFF_SIZE 1024

typedef struct _bufChain {

  struct _bufChain*  next;        /* Link to next buffer */


  void* writeP;  /* Points to beginning of empty buffer area */
  void* readP;      /* Points to first element not sent to writer */
  void* endP;      /* Points to first element not sent to writer */

  pthread_mutex_t lock;       /* Mutex to protect concurrent writes */
  //  pthread_cond_t lock;       /* Mutex to protect concurrent writes */

  long  bufLength;              /* Length of buffer */
  char  buf;               /* Points at the beginning of the buffer */
} BufferChain;

typedef struct {
  int  active;		  /* true if buffer is active, false kills thread */

  long chainsAvailable;         /* Number of chains which can still be allocated */
  long chainLength;             /* Length of buffer in each chain */

  BufferChain* writerChain;	/* Chain to write/push to */
  BufferChain* readerChain;	/* Chain to read/pop from */

  pthread_mutex_t lock;         /* Mutex to protect this struct */
  pthread_t  readerThread;	/* Thread for reading the queue and writing to socket */

  BufferChain* first;
} BufferedSocket;

typedef void* BufferedSocketHdl;

static BufferChain* getNextWriteChain(BufferedSocket* self);
static BufferChain* createBufferChain(BufferedSocket* self);
static void* threadStart(void* handle);

/**
 * \fn BufferedSocketHdl* buffSocket_create(long queueCapacity)
 * \brief Create a BufferedSocket instance
 * \param queueCapaity the max. size of the internal queue
 * \return an instance pointer if success, NULL if not.
 */
BufferedSocketHdl
buffSocket_create(
  Socket* socket,
  long  queueCapacity 		/* size of queue before dropping stuff */
) {
  BufferedSocket* self = (BufferedSocket*)malloc(sizeof(BufferedSocket));
  if (self ==NULL) return NULL;
  memset(self, 0, sizeof(BufferedSocket));

  long bufSize = queueCapacity / 2;
  if (bufSize > MAX_BUFF_SIZE) bufSize = MAX_BUFF_SIZE;
  self->chainLength = bufSize;
  self->chainsAvailable = queueCapacity / bufSize;
  self->writerChain = self->readerChain = createBufferChain(self);

  self->active = 1;
  pthread_create(&self->readerThread, NULL, threadStart, (void*)self);
  return (BufferedSocketHdl)self;
}

/**
 * \fn buffSocket_destroy(BufferedSocket* self)
 * \brief Destroy or free all allocated resources
 * \param instance Instance handle;
 */
void
buffSocket_destroy(
  BufferedSocketHdl instance
) {
  BufferedSocket* self = (BufferedSocket*)instance;
  self->active = 0;
}
 
/**
 * \fn buffSocket_push(BufferedSocketHdl* buffSocket, void* chunk, long chunkSize)
 * \brief Add a chunk to the end of the queue.
 * \param instance BufferedSocket handle
 * \param chunk Pointer to chunk to add
 * \param chunkSize size of chunk
 * \return 1 if success, 0 otherwise
 */
int
buffSocket_push(
  BufferedSocketHdl instance,
  void*  chunk,
  long chunkSize
) {
  BufferedSocket* self = (BufferedSocket*)instance;
  if (!self->active) return 0;
  if (oml_lock(&self->lock, "bufferedSocket")) return 0;

  BufferChain* buffer = self->writerChain;
  if (buffer == NULL) return 0;

  if (oml_lock(&buffer->lock, "bufferChain")) return 0;

  if ((buffer->endP - buffer->writeP) < chunkSize) {
    // Full, let's move on to the next chain
    BufferChain* nextBuffer = getNextWriteChain(self);
    oml_unlock(&buffer->lock, "bufferChain");
    buffer = nextBuffer;
    if (oml_lock(&buffer->lock, "bufferChain")) return 0;
  }
  memcpy(buffer->writeP, chunk, chunkSize);
  buffer->writeP += chunkSize;
  oml_unlock(&buffer->lock, "bufferChain");
  oml_unlock(&self->lock, "bufferedSocket");
  return 1;
}

// This function finds the next empty write chain, sets +self->writeChain+ to 
// it and returns.
//
// We only use the next one if it is empty. If not, we
// essentially just filled up the last chain and wrapped
// around to the socket reader. In that case, we either create a new chain 
// if the overall buffer can still grow, or we drop the data from the current one.
//
// This assumes that the current thread holds the +self->lock+ and the lock on
// the +self->writeChain+.
//
BufferChain*
getNextWriteChain(
  BufferedSocket* self
) {
  BufferChain* buffer = self->writerChain;
  BufferChain* nextBuffer = buffer->next;

  if (nextBuffer->writeP == &nextBuffer->buf) {
    // empty!!
    return self->writerChain = nextBuffer;
  }
  // OK, need to insert a new chain or clear the current one.
  if (self->chainsAvailable > 0) {
    BufferChain* newBuffer = createBufferChain(self);
    newBuffer->next = nextBuffer;
    return buffer->next = newBuffer;
  }
  // Filled up buffer, time to drop data
  buffer->writeP = &buffer->buf;
  return buffer;
}

BufferChain*
createBufferChain(
  BufferedSocket* self
) {
  BufferChain* buf = (BufferChain*)malloc(sizeof(BufferChain) + self->chainLength);
  if (buf == NULL) return NULL;
  memset(buf, 0, sizeof(BufferChain));

  // set state
  buf->writeP = buf->readP = &buf->buf;
  buf->endP = &buf->buf + (buf->bufLength = self->chainLength);

  self->chainsAvailable--;
  return buf;
}


/**
 * \fn static void* thread_start(void* handle)
 * \brief start the filter thread
 * \param handle the stream to use the filters on
 */
static void*
threadStart(
  void* handle
) {
  BufferedSocket* self = (BufferedSocket*)handle;

  while (self->active) {
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
