/*
 * Copyright 2011-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file buffered_writer.c
 * \brief Implements a non-blocking, self-draining FIFO queue using threads.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"

#include "client.h"
#include "buffered_writer.h"

/** Default target size in each MBuffer of the chain */
#define DEF_CHAIN_BUFFER_SIZE 1024

/** A circular chain of buffers */
typedef struct BufferChain {

  /** Link to the next buffer in the chain */
  struct BufferChain*  next;

  /** MBuffer used for storage */
  MBuffer* mbuf;
  /** Target maximal size of mbuf for that link */
  size_t   targetBufSize;

  /** Set to 1 when the reader is processing this buffer */
  int   reading;
} BufferChain;

/** A writer reading from a BufferChain */
typedef struct BufferedWriter {
  /** Set to !0 if buffer is active; 0 kills the thread */
  int  active;

  /** Number of links which can still be allocated */
  long chainsAvailable;
  /** Target size of MBuffer in each link */
  size_t chainLength;

  /** Function called to write the buffer out, can be blocking */
  oml_outs_write_f  writeFunc;
  /** Opaque handler to writeFunc */
  OmlOutStream*     writeFuncHdl;

  /** Chain where the data gets stored until it's pushed out */
  BufferChain* writerChain;
  /** Immutable entry into the chain */
  BufferChain* firstChain;

  /** Buffer holding protocol headers */
  MBuffer*     meta_buf;

  /** Mutex protecting the object */
  pthread_mutex_t lock;
  /** Semaphore for this object */
  pthread_cond_t semaphore;
  /** Thread in charge of reading the queue and writing the data out */
  pthread_t  readerThread;

} BufferedWriter;

static BufferChain* getNextWriteChain(BufferedWriter* self, BufferChain* current);
static BufferChain* createBufferChain(BufferedWriter* self);
static int destroyBufferChain(BufferedWriter* self);
static void* threadStart(void* handle);
static int processChain(BufferedWriter* self, BufferChain* chain);

/** Create a BufferedWriter instance
 * \param writeFunc oml_outs_write_f function in charge of draining the buffer, can block
 * \param writeFuncHdl opaque OmlOutStream handler to writeFunc
 * \param queueCapaity maximal size of the internal queue
 * \param chunkSize size of buffer space allocated at a time, set to 0 for default
 *
 * \return an instance pointer if successful, NULL otherwise
 */
BufferedWriterHdl
bw_create(oml_outs_write_f writeFunc, OmlOutStream* writeFuncHdl, long  queueCapacity, long  chunkSize)
{
  if (writeFunc == NULL || writeFuncHdl == NULL ||
      queueCapacity < 0 || chunkSize < 0) {
    return NULL;
  }

  BufferedWriter* self = (BufferedWriter*)xmalloc(sizeof(BufferedWriter));
  if (self == NULL) { return NULL; }
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

  self->meta_buf = mbuf_create();

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

/** Close an output stream and destroy the objects.
 *
 * \param instance handle (i.e., pointer) to a BufferedWriter
 */
void
bw_close(BufferedWriterHdl instance)
{
  BufferedWriter *self = (BufferedWriter*)instance;

  if(!self) { return; }

  if (oml_lock (&self->lock, __FUNCTION__)) { return; }
  self->active = 0;

  loginfo ("Waiting for buffered queue reader thread to drain...\n");

  pthread_cond_signal (&self->semaphore);
  oml_unlock (&self->lock, __FUNCTION__);
  //  pthread_cond_destroy(&self->semaphore, NULL);
  switch (pthread_join (self->readerThread, NULL)) {
  case 0:
    loginfo ("Buffered queue reader thread finished OK...\n");
    break;
  case EINVAL:
    logerror ("Buffered queue reader thread is not joinable\n");
    break;
  case EDEADLK:
    logerror ("Buffered queue reader thread shutdown deadlock, or self-join\n");
    break;
  case ESRCH:
    logerror ("Buffered queue reader thread shutdown failed: could not find the thread\n");
    break;
  default:
    logerror ("Buffered queue reader thread shutdown failed with an unknown error\n");
    break;
  }

  destroyBufferChain(self);
  xfree(self);
}

/** Add a chunk to the end of the queue.
 * \param instance BufferedWriter handle
 * \param chunk Pointer to chunk to add
 * \param chunkSize size of chunk
 * \return 1 if success, 0 otherwise
 */
int
bw_push(BufferedWriterHdl instance, uint8_t* chunk, size_t size)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, __FUNCTION__)) { return 0; }
  if (!self->active) { return 0; }

  BufferChain* chain = self->writerChain;
  if (chain == NULL) { return 0; }

  if (chain->mbuf->wr_remaining < size) {
    chain = self->writerChain = getNextWriteChain(self, chain);
  }

  if (mbuf_write(chain->mbuf, chunk, size) < 0) {
    return 0;
  }

  pthread_cond_signal(&self->semaphore);

  oml_unlock(&self->lock, __FUNCTION__);
  return 1;
}

/** Add a chunk to the end of the header buffer.
 *
 * \param instance BufferedWriter handle
 * \param chunk pointer to chunk to add
 * \param chunkSize size of chunk
 *
 * \return 1 if success, 0 otherwise
 */
int
bw_push_meta(BufferedWriterHdl instance, uint8_t* chunk, size_t size)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  int result = 0;

  if (oml_lock(&self->lock, __FUNCTION__)) { return 0; }
  if (!self->active) { return 0; }

  if (mbuf_write(self->meta_buf, chunk, size) > 0) {
    result = 1;
    pthread_cond_signal(&self->semaphore);
  }
  oml_unlock(&self->lock, __FUNCTION__);
  return result;
}

/** Return an MBuffer with (optional) exclusive write access
 *
 * If exclusive access is required, the caller is in charge of releasing the
 * lock with bw_unlock_buf.
 *
 * \param instance BufferedWriter handle
 * \param exclusive indicate whether the entire BufferedWriter should be locked
 *
 * \return an MBuffer instance if success to write in, NULL otherwise
 * \see bw_unlock_buf
 */
MBuffer*
bw_get_write_buf(BufferedWriterHdl instance, int exclusive)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, __FUNCTION__)) { return 0; }
  if (!self->active) { return 0; }

  BufferChain* chain = self->writerChain;
  if (chain == NULL) { return 0; }

  MBuffer* mbuf = chain->mbuf;
  if (mbuf_write_offset(mbuf) >= chain->targetBufSize) {
    chain = self->writerChain = getNextWriteChain(self, chain);
    mbuf = chain->mbuf;
  }
  if (! exclusive) {
    oml_unlock(&self->lock, __FUNCTION__);
  }
  return mbuf;
}


/** Return and unlock MBuffer
 * \param instance BufferedWriter handle for which a buffer was previously obtained through bw_get_write_buf
 *
 * \see bw_get_write_buf
 */
void
bw_unlock_buf(BufferedWriterHdl instance)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  pthread_cond_signal(&self->semaphore); /* assume we locked for a reason */
  oml_unlock(&self->lock, __FUNCTION__);
}

/** Find the next empty write chain, sets self->writeChain to it and returns it.
 *
 * We only use the next one if it is empty. If not, we essentially just filled
 * up the last chain and wrapped around to the socket reader. In that case, we
 * either create a new chain if the overall buffer can still grow, or we drop
 * the data from the current one.
 *
 * This assumes that the current thread holds the self->lock and the lock on
 * the self->writeChain.
 *
 * \param self BufferedWriter pointer
 * \param current BufferChain to use or from which to find the next
 * \return a BufferChain in which data can be stored
 */
BufferChain*
getNextWriteChain(BufferedWriter* self, BufferChain* current) {
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

/** Initialise a BufferChain for a BufferedWriter.
 * \param self BufferedWriter pointer
 * \return a pointer to the newly-created BufferChain, or NULL on error
 */
BufferChain*
createBufferChain(BufferedWriter* self)
{
  //  BufferChain* chain = (BufferChain*)malloc(sizeof(BufferChain) + self->chainLength);

  MBuffer* buf = mbuf_create2(self->chainLength, (size_t)(0.1 * self->chainLength));
  if (buf == NULL) { return NULL; }

  BufferChain* chain = (BufferChain*)xmalloc(sizeof(BufferChain));
  if (chain == NULL) {
    mbuf_destroy(buf);
    return NULL;
  }
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


/** Destroy the Buffer chain of a BufferedWriter
 *
 * \param self pointer to the BufferedWriter
 * \return 0 on success, or a negative number otherwise
 */
int
destroyBufferChain(BufferedWriter* self) {
  BufferChain *chain, *start;

  if (!self) {
    return -1;
  }

  /* BufferChain is a circular buffer */
  start = self->firstChain;
  while( (chain = self->firstChain) && chain!=start) {
    logdebug("Destroying BufferChain at %p\n", chain);
    self->firstChain = chain->next;
    mbuf_destroy(chain->mbuf);
    xfree(chain);
  }

  return 0;
}


/** Writing thread
 * \param handle the stream to use the filters on
 * \return NULL on error; this function should not return
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

/** Process the chain and send data
 * \param selfBufferedWriter to process
 * \param chain link of the chain to process
 *
 * \return 1 if chain has been fully sent, 0 otherwise
 * \see oml_outs_write_f
 */
int
processChain(BufferedWriter* self, BufferChain* chain)
{
  uint8_t* buf = mbuf_rdptr(chain->mbuf);
  size_t size = mbuf_message_offset(chain->mbuf) - mbuf_read_offset(chain->mbuf);
  size_t sent = 0;
  chain->reading = 1;
  oml_unlock(&self->lock, __FUNCTION__); /* don't keep lock while transmitting */
  MBuffer* meta = self->meta_buf;

  while (size > sent) {
    long cnt = self->writeFunc(self->writeFuncHdl, (void*)(buf + sent), size - sent,
                               meta->rdptr, meta->fill);
    if (cnt > 0) {
      sent += cnt;
    } else {
      /* ERROR: Sleep a bit and try again */
      /* To be on the safe side, we rewind to the beginning of the
       * chain and try to resend everything - this is especially important
       * if the underlying stream needs to reopen and resync.
       */
      mbuf_reset_read(chain->mbuf);
      size = mbuf_message_offset(chain->mbuf) - mbuf_read_offset(chain->mbuf);
      sent = 0;
      sleep(1);
    }
  }
  // get lock back to see what happened while we were busy
  oml_lock_persistent(&self->lock, __FUNCTION__);
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

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
