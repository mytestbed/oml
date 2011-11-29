/*
 * Copyright 2011 National ICT Australia (NICTA), Australia
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
  \brief Implements a non-blocking, self-draining FIFO queue.
*/

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <oml2/omlc.h>
#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <log.h>

#include "client.h"
#include "buffered_writer.h"

#define DEF_CHAIN_BUFFER_SIZE 1024

typedef struct _bufChain {

  struct _bufChain*  next;        //! Link to next buffer

  MBuffer* mbuf;         //! expandable storage
  size_t   targetBufSize;

  int   reading;                //! set to 1 when reader is processing this chain
} BufferChain;

typedef struct {
  int  active;      //! true if buffer is active, false kills thread

  long chainsAvailable;         //! Number of chains which can still be allocated
  size_t chainLength;           //! Length of buffer in each chain

  oml_outs_write_f  writeFunc;        //! Function to drain buffer - can block
  OmlOutStream*     writeFuncHdl;     //! Opaque handler to above function


  BufferChain* writerChain; //! Chain to write/push to
  BufferChain* firstChain;  //! Immutable entry into the chain

  pthread_mutex_t lock;         //! Mutex to protect this struct
  pthread_cond_t semaphore;
  pthread_t  readerThread;  //! Thread for reading the queue and writing to socket

} BufferedWriter;


static BufferChain* getNextWriteChain(BufferedWriter* self, BufferChain* current);
static BufferChain* createBufferChain(BufferedWriter* self);
static void* threadStart(void* handle);
static int processChain(BufferedWriter* self, BufferChain* chain);
static void oml_lock_persistent(BufferedWriter* self);


/**
 * \fn BufferedWriterHdl* buffSocket_create(long queueCapacity)
 * \brief Create a BufferedWriter instance
 * \param queueCapaity the max. size of the internal queue
 * \param chunkSize the size of buffer space allocated at a time, set to 0 for default
 * \return an instance pointer if success, NULL if not.
 */
BufferedWriterHdl
bw_create(
  oml_outs_write_f  writeFunc,        //! Function to drain buffer - can block
  OmlOutStream*     writeFuncHdl,     //! Opaque handler to above function
  long  queueCapacity,    /* size of queue before dropping stuff */
  long  chunkSize
) {
  BufferedWriter* self = (BufferedWriter*)malloc(sizeof(BufferedWriter));
  if (self ==NULL) return NULL;
  memset(self, 0, sizeof(BufferedWriter));

  self->writeFunc = writeFunc;
  self->writeFuncHdl = writeFuncHdl;

  long bufSize = chunkSize > 0 ? chunkSize : DEF_CHAIN_BUFFER_SIZE;
  self->chainLength = bufSize;

  long chunks = queueCapacity / bufSize;
  self->chainsAvailable = chunks > 2 ? chunks : 2; /* at least two chunks */

  /* Start out with two circular linked buffers */
  BufferChain* buf1 = self->firstChain = createBufferChain(self);
  buf1->next = buf1;
  //  BufferChain* buf2 = buf1->next = createBufferChain(self);
  //  buf2->next = buf1;

  self->writerChain = buf1;


  /* Initialize mutex and condition variable objects */
  pthread_cond_init(&self->semaphore, NULL);
  pthread_mutex_init(&self->lock, NULL);

  /* Initialize and set thread detached attribute */
  pthread_attr_t tattr;
  pthread_attr_init(&tattr);
  pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
  self->active = 1;
  pthread_create(&self->readerThread, &tattr, threadStart, (void*)self);


  return (BufferedWriterHdl)self;
}

/**
 * \fn buffSocket_destroy(BufferedWriter* self)
 * \brief Destroy or free all allocated resources
 * \param instance Instance handle;
 */
void
bw_close(
  BufferedWriterHdl instance
) {
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock (&self->lock, "bw_close")) return;
  self->active = 0;

  loginfo ("Waiting for buffered queue reader thread to drain...\n");

  pthread_cond_signal (&self->semaphore);
  oml_unlock (&self->lock, "bw_close");
  //  pthread_cond_destroy(&self->semaphore, NULL);
  int result = pthread_join (self->readerThread, NULL);
  if (result != 0) {
    switch (result) {
    case EINVAL:
      logerror ("Buffered queue reader thread is not joinable\n");
      break;
    case EDEADLK:
      logerror ("Buffered queue reader thread shutdown deadlock, or self-join\n");
      break;
    case ESRCH:
      logerror ("Buffered queue reader thread shutdown failed:  could not find the thread\n");
      break;
    }
  }
  loginfo ("Buffered queue reader thread finished OK...\n");
}

/**
 * \fn bw_push(BufferedWriterHdl* buffSocket, void* chunk, long chunkSize)
 * \brief Add a chunk to the end of the queue.
 * \param instance BufferedWriter handle
 * \param chunk Pointer to chunk to add
 * \param chunkSize size of chunk
 * \return 1 if success, 0 otherwise
 */
int
bw_push(
  BufferedWriterHdl instance,
  uint8_t*  chunk,
  size_t size
) {
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, "bw_push")) return 0;
  if (!self->active) return 0;

  BufferChain* chain = self->writerChain;
  if (chain == NULL) return 0;

  if (chain->mbuf->wr_remaining < size) {
    chain = self->writerChain = getNextWriteChain(self, chain);
  }

  if (mbuf_write(chain->mbuf, chunk, size) < 0)
    return 0;

  pthread_cond_signal(&self->semaphore);

//  oml_unlock(&chain->lock, "bw_push");
  oml_unlock(&self->lock, "bw_push");
  return 1;
}

/**
 * \brief Return an MBuffer with exclusive write access
 * \return MBuffr instance if success, NULL otherwise
 */
MBuffer*
bw_get_write_buf(
  BufferedWriterHdl instance,
  int exclusive
) {
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, "bw_get_write_buf")) return 0;
  if (!self->active) return 0;

  BufferChain* chain = self->writerChain;
  if (chain == NULL) return 0;

  MBuffer* mbuf = chain->mbuf;
  if (mbuf_write_offset(mbuf) >= chain->targetBufSize) {
    chain = self->writerChain = getNextWriteChain(self, chain);
    mbuf = chain->mbuf;
  }
  if (! exclusive) {
    oml_unlock(&self->lock, "bw_get_write_buf");
  }
  return mbuf;
}

/**
 * \brief Return and unlock MBuffer
 */
void
bw_unlock_buf(BufferedWriterHdl instance)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  pthread_cond_signal(&self->semaphore); /* assume we locked for a reason */
  oml_unlock(&self->lock, "bw_unlock_buf");
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
  BufferedWriter* self,
  BufferChain* current
) {
  assert(current != NULL);
  BufferChain* nextBuffer = current->next;
  assert(nextBuffer != NULL);

  BufferChain* resChain = NULL;
  if (mbuf_remaining(nextBuffer->mbuf) == 0) {
    // It's empty, we can use it
    mbuf_clear2(nextBuffer->mbuf, 0);
    resChain = nextBuffer;
  } else if (self->chainsAvailable > 0) {
    // insert new chain between current and next one.
    BufferChain* newBuffer = createBufferChain(self);
    newBuffer->next = nextBuffer;
    current->next = newBuffer;
    resChain = newBuffer;
  } else {
    // Filled up buffer, time to drop data and reuse current buffer
    // Current buffer holds most recent added data (we drop from the queue's tail
    //assert(current->reading == 0);
    assert(current->reading == 0);
    o_log (O_LOG_WARN, "Dropping %d bytes of measurement data\n", mbuf_fill(current->mbuf));
    mbuf_repack_message2(current->mbuf);
    return current;
  }
  // Now we just need to copy the +message+ from +current+ to +resChain+
  int msgSize = mbuf_message_length(current->mbuf);
  if (msgSize > 0) {
    mbuf_write(resChain->mbuf, mbuf_message(current->mbuf), msgSize);
    mbuf_reset_write(current->mbuf);
  }
  return resChain;
}

BufferChain*
createBufferChain(
  BufferedWriter* self
) {
  //  BufferChain* chain = (BufferChain*)malloc(sizeof(BufferChain) + self->chainLength);

  MBuffer* buf = mbuf_create2(self->chainLength, (size_t)(0.1 * self->chainLength));
  if (buf == NULL) return NULL;

  BufferChain* chain = (BufferChain*)malloc(sizeof(BufferChain));
  if (chain == NULL) return NULL;
  memset(chain, 0, sizeof(BufferChain));

  // set state
  chain->mbuf = buf;
  chain->targetBufSize = self->chainLength;

  /**
  buf->writeP = buf->readP = &buf->buf;
  buf->endP = &buf->buf + (buf->bufLength = self->chainLength);
  */

  self->chainsAvailable--;
  o_log (O_LOG_DEBUG, "Created new buffer chain of size %d with %d remaining.\n",
        self->chainLength, self->chainsAvailable);
  return chain;
}


/**
 * \fn static void* threadStart(void* handle)
 * \brief start the filter thread
 * \param handle the stream to use the filters on
 */
static void*
threadStart(void* handle)
{
  BufferedWriter* self = (BufferedWriter*)handle;
  BufferChain* chain = self->firstChain;

  while (self->active) {
    oml_lock(&self->lock, "bufferedWriter");
    pthread_cond_wait(&self->semaphore, &self->lock);
    // Process all chains which have data in them
    while(1) {
      if (mbuf_message(chain->mbuf) > mbuf_rdptr(chain->mbuf)) {
        // got something to read from this chain
        while (!processChain(self, chain));
      }
      // stop if we caught up to the writer

      if (chain == self->writerChain) break;

      chain = chain->next;
    }
    oml_unlock(&self->lock, "bufferedWriter");
  }
  return NULL;
}

// return 1 if chain has been fully sent, 0 otherwise
int
processChain(
  BufferedWriter* self,
  BufferChain* chain
) {
  uint8_t* buf = mbuf_rdptr(chain->mbuf);
  size_t size = mbuf_message_offset(chain->mbuf) - mbuf_read_offset(chain->mbuf);
  size_t sent = 0;
  chain->reading = 1;
  oml_unlock(&self->lock, "processChain"); /* don't keep lock while transmitting */

  while (size > sent) {
    long cnt = self->writeFunc(self->writeFuncHdl, (void*)(buf + sent), size - sent);
    if (cnt > 0) {
      sent += cnt;
    } else {
      /* ERROR: Sleep a bit and try again */
      sleep(1);
    }
  }
  // get lock back to see what happened while we were busy
  oml_lock_persistent(self);
  mbuf_read_skip(chain->mbuf, sent);
  if (mbuf_write_offset(chain->mbuf) == mbuf_read_offset(chain->mbuf)) {
    // seem to have sent everything so far, reset chain
    //    mbuf_clear2(chain->mbuf, 0);
    mbuf_clear2(chain->mbuf, 1);
    chain->reading = 0;
    return 1;
  }
  return 0;
}

// Get lock and keep on retrying if it fails
void
oml_lock_persistent(
  BufferedWriter* self
) {
  while (oml_lock(&self->lock, "bufferedWriter")) {
    o_log (O_LOG_WARN, "Having problems obtaining lock in buffered writer. Will try again soon.\n");
    sleep(1);
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
