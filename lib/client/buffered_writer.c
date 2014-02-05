/*
 * Copyright 2011-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file buffered_writer.c
 * \brief A non-blocking, self-draining FIFO queue using threads.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"

#include "client.h"
#include "buffered_writer.h"

/** Default target size in each MBuffer of the chunk */
#define DEF_CHAIN_BUFFER_SIZE 1024

/** A chunk of data to be put in a circular chain */
typedef struct BufferChunk {

  /** Link to the next buffer in the chunk */
  struct BufferChunk*  next;

  /** MBuffer used for storage */
  MBuffer* mbuf;
  /** Target maximal size of mbuf for that chunk */
  size_t   targetBufSize;

  /** Set to 1 when the reader is processing this chunk */
  int   reading;

  int nmessages; /**< Number of messages contained in this chunk */

} BufferChunk;

/** A writer reading from a chain of BufferChunks */
typedef struct BufferedWriter {
  /** Set to !0 if buffer is active; 0 kills the thread */
  int  active;

  /** Number of links which can still be allocated */
  long unallocatedBuffers;
  /** Target size of MBuffer in each chunk*/
  size_t bufSize;

  /** Opaque handler to  the output stream*/
  OmlOutStream*    outStream;

  /** Chunk where the data gets stored until it's pushed out */
  BufferChunk* writerChunk;
  /** Immutable entry into the chain */
  BufferChunk* firstChunk;

  /** Buffer holding protocol headers */
  MBuffer*     meta_buf;

  /** Mutex protecting the object */
  pthread_mutex_t lock;
  /** Semaphore for this object */
  pthread_cond_t semaphore;
  /** Thread in charge of reading the queue and writing the data out */
  pthread_t  readerThread;

  /** Time of the last failure, to backoff for REATTEMP_INTERVAL before retryying **/
  time_t last_failure_time;

  /** Backoff time, in seconds */
  uint8_t backoff;

  /** Return status from the thread */
  int retval;

  int nlost; /**< Number of lost messages since last query */

} BufferedWriter;
#define REATTEMP_INTERVAL 5    //! Seconds to open the stream again

static BufferChunk* getNextWriteChunk(BufferedWriter* self, BufferChunk* current);
static BufferChunk* createBufferChunk(BufferedWriter* self);
static int destroyBufferChain(BufferedWriter* self);
static void* threadStart(void* handle);
static int processChunk(BufferedWriter* self, BufferChunk* chunk);

/** Create a BufferedWriter instance
 *
 * \param outStream opaque OmlOutStream handler
 * \param queueCapacity maximal size [B] of the internal queue queueCapaity/chunkSize will be used (at least 2)
 * \param chunkSize size [B] of buffer space allocated at a time, set to 0 for default (DEF_CHAIN_BUFFER_SIZE)
 * \return an instance pointer if successful, NULL otherwise
 *
 * \see DEF_CHAIN_BUFFER_SIZE
 */
BufferedWriterHdl
bw_create(OmlOutStream* outStream, long  queueCapacity, long chunkSize)
{
  long nchunks;
  BufferedWriter* self = NULL;

  assert(outStream>=0);
  assert(queueCapacity>=0);
  assert(chunkSize>=0);

  if((self = (BufferedWriter*)oml_malloc(sizeof(BufferedWriter)))) {
    memset(self, 0, sizeof(BufferedWriter));

    self->outStream = outStream;
    /* This forces a 'connected' INFO message upon first connection */
    self->backoff = 1;

    self->bufSize = chunkSize > 0 ? chunkSize : DEF_CHAIN_BUFFER_SIZE;

    nchunks = queueCapacity / self->bufSize;
    self->unallocatedBuffers = (nchunks > 2) ? nchunks : 2; /* at least two chunks */

    logdebug ("%s: Buffer size %dB (%d chunks of %dB)\n",
        self->outStream->dest,
        self->unallocatedBuffers*self->bufSize,
        self->unallocatedBuffers, self->bufSize);

    if(NULL == (self->writerChunk = self->firstChunk = createBufferChunk(self))) {
      oml_free(self);
      self = NULL;

    } else if(NULL == (self->meta_buf = mbuf_create())) {
      destroyBufferChain(self);
      oml_free(self);
      self = NULL;

    } else {
      /* Initialize mutex and condition variable objects */
      pthread_cond_init(&self->semaphore, NULL);
      pthread_mutex_init(&self->lock, NULL);

      /* Initialize and set thread detached attribute */
      pthread_attr_t tattr;
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
      self->active = 1;
      pthread_create(&self->readerThread, &tattr, threadStart, (void*)self);
    }
  }

  return (BufferedWriterHdl)self;
}

/** Close an output stream and destroy the objects.
 *
 * \param instance handle (i.e., pointer) to a BufferedWriter
 */
void
bw_close(BufferedWriterHdl instance)
{
  int *retval;
  BufferedWriter *self = (BufferedWriter*)instance;

  if(!self) { return; }

  if (oml_lock (&self->lock, __FUNCTION__)) { return; }
  self->active = 0;

  loginfo ("%s: Waiting for buffered queue thread to drain...\n", self->outStream->dest);

  pthread_cond_signal (&self->semaphore);
  oml_unlock (&self->lock, __FUNCTION__);

  if(pthread_join (self->readerThread, (void**)&retval)) {
    logwarn ("%s: Cannot join buffered queue reader thread: %s\n",
        self->outStream->dest, strerror(errno));

  } else {
    if (1 == *retval) {
      logdebug ("%s: Buffered queue fully drained\n", self->outStream->dest);
    } else {
      logerror ("%s: Buffered queue did not fully drain\n",
          self->outStream->dest, *retval);
    }
  }

  self->outStream->close(self->outStream);
  destroyBufferChain(self);
  oml_free(self);
}

/** Add some data to the end of the queue.
 *
 * This function tries to acquire the lock on the BufferedWriter, and releases
 * it when done.
 *
 * \param instance BufferedWriter handle
 * \param data Pointer to data to add
 * \param size size of data
 * \return 1 if success, 0 otherwise
 *
 * \see _bw_push
 */
int
bw_push(BufferedWriterHdl instance, uint8_t *data, size_t size)
{
  int result = 0;
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, __FUNCTION__) == 0) {
    result =_bw_push(instance, data, size);
    oml_unlock(&self->lock, __FUNCTION__);
  }
  return result;
}

/** Add some data to the end of the queue (lock must be held).
 *
 * This function is the same as bw_push except it assumes that the
 * lock is already acquired.
 *
 * \copydetails bw_push
 *
 * \see bw_push
 */
int
_bw_push(BufferedWriterHdl instance, uint8_t* data, size_t size)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  if (!self->active) { return 0; }

  BufferChunk* chunk = self->writerChunk;
  if (chunk == NULL) { return 0; }

  if (mbuf_wr_remaining(chunk->mbuf) < size) {
    chunk = getNextWriteChunk(self, chunk);
  }

  if (mbuf_write(chunk->mbuf, data, size) < 0) {
    return 0;
  }

  pthread_cond_signal(&self->semaphore);

  return 1;
}

/** Add some data to the end of the header buffer.
 *
 * This function tries to acquire the lock on the BufferedWriter, and releases
 * it when done.
 *
 * \param instance BufferedWriter handle
 * \param data Pointer to data to add
 * \param size size of data
 * \return 1 if success, 0 otherwise
 *
 * \see _bw_push_meta
 */
int
bw_push_meta(BufferedWriterHdl instance, uint8_t* data, size_t size)
{
  int result = 0;
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->lock, __FUNCTION__) == 0) {
    result = _bw_push_meta(instance, data, size);
    oml_unlock(&self->lock, __FUNCTION__);
  }
  return result;
}

/** Add some data to the end of the header buffer (lock must be held).
 *
 * This function is the same as bw_push_meta except it assumes that the lock is
 * already acquired.
 *
 * \copydetails bw_push_meta
 *
 * \see bw_push_meta
 *
 */
int
_bw_push_meta(BufferedWriterHdl instance, uint8_t* data, size_t size)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  int result = 0;

  if (!self->active) { return 0; }

  if (mbuf_write(self->meta_buf, data, size) > 0) {
    result = 1;
    /* XXX: There is no point in signalling the semaphore as the
     * writer will not be able to do anything with the new data.
     *
     * Also, it puts everything in a deadlock otherwise */
    /* pthread_cond_signal(&self->semaphore); */
  }
  return result;
}

/** Count the addition (or deletion) of a full message in the current BufferChunk.
 *
 * \param instance BufferedWriter handle
 * \param nmessages number of messages to count (can be negative)
 *
 * \return the (new) current number of messages in the current writer BufferChunk
 *
 * \see bw_msgcount_reset, bw_nlost_reset
 */
int
bw_msgcount_add(BufferedWriterHdl instance, int nmessages) {
  BufferedWriter* self = (BufferedWriter*)instance;
  self->writerChunk->nmessages += 1;
  return self->writerChunk->nmessages;
}

/** Reset the message count in the current BufferChunk and return its previous value.
 *
 * \param instance BufferedWriter handle
 *
 * \return the number of messages in the current writer BufferChunk before resetting
 *
 * \see bw_msgcount_add, bw_nlost_reset
 */
int
bw_msgcount_reset(BufferedWriterHdl instance) {
  BufferedWriter* self = (BufferedWriter*)instance;
  int n = self->writerChunk->nmessages;
  self->writerChunk->nmessages = 0;
  return n;
}

/** Reset the number of messages lost by the BufferedWriter and return its previous value.
 *
 * \param instance BufferedWriter handle
 *
 * \return the number of lost messages in the BufferedWriter before resetting
 *
 * \see bw_msgcount_add, bw_msgcount_reset
 */
int
bw_nlost_reset(BufferedWriterHdl instance) {
  BufferedWriter* self = (BufferedWriter*)instance;
  int n = self->nlost;
  self->nlost = 0;
  return n;
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

  BufferChunk* chunk = self->writerChunk;
  if (chunk == NULL) { return 0; }

  MBuffer* mbuf = chunk->mbuf;
  if (mbuf_write_offset(mbuf) >= chunk->targetBufSize) {
    chunk = getNextWriteChunk(self, chunk);
    mbuf = chunk->mbuf;
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

/** Find the next empty write chunk, sets self->writeChunk to it and returns it.
 *
 * We only use the next one if it is empty. If not, we essentially just filled
 * up the last chunk and wrapped around to the socket reader. In that case, we
 * either create a new chunk if the overall buffer can still grow, or we drop
 * the data from the current one.
 *
 * This assumes that the current thread holds the self->lock and the lock on
 * the self->writeChunk.
 *
 * \param self BufferedWriter pointer
 * \param current BufferChunk to use or from which to find the next
 * \return a BufferChunk in which data can be stored
 */
BufferChunk*
getNextWriteChunk(BufferedWriter* self, BufferChunk* current) {
  int nlost;
  assert(current != NULL);
  BufferChunk* nextBuffer = current->next;
  assert(nextBuffer != NULL);

  if (mbuf_rd_remaining(nextBuffer->mbuf) == 0) {
    // It's empty (the reader has finished with it), we can use it
    mbuf_clear2(nextBuffer->mbuf, 0);
    self->writerChunk = nextBuffer;
    bw_msgcount_reset(self);

  } else if (self->unallocatedBuffers > 0) {
    // Insert a new chunk between current and next one.
    BufferChunk* newBuffer = createBufferChunk(self);
    assert(newBuffer);
    newBuffer->next = nextBuffer;
    current->next = newBuffer;
    self->writerChunk = newBuffer;

  } else {
    // The chain is full, time to drop data and reuse the next buffer
    current = nextBuffer;
    assert(nextBuffer->reading == 0); /* Ensure this is not the chunk currently being read */
    self->writerChunk = nextBuffer;

    nlost = bw_msgcount_reset(self);
    self->nlost += nlost;
    logwarn("Dropped %d samples (%dB)\n", nlost, mbuf_fill(nextBuffer->mbuf));
    mbuf_repack_message2(self->writerChunk->mbuf);
  }

  // Now we just need to copy the message from current to self->writerChunk
  int msgSize = mbuf_message_length(current->mbuf);
  if (msgSize > 0) {
    mbuf_write(self->writerChunk->mbuf, mbuf_message(current->mbuf), msgSize);
    mbuf_reset_write(current->mbuf);
    bw_msgcount_add(self, 1);
  }

  return self->writerChunk;
}

/** Initialise a BufferChunk for a BufferedWriter.
 * \param self BufferedWriter pointer
 * \return a pointer to the newly-created BufferChunk, or NULL on error
 */
BufferChunk*
createBufferChunk(BufferedWriter* self)
{
  size_t initsize = 0.1 * self->bufSize;
  MBuffer* buf = mbuf_create2(self->bufSize, initsize);
  if (buf == NULL) { return NULL; }

  BufferChunk* chunk = (BufferChunk*)oml_malloc(sizeof(BufferChunk));
  if (chunk == NULL) {
    mbuf_destroy(buf);
    return NULL;
  }
  memset(chunk, 0, sizeof(BufferChunk));

  // set state
  chunk->mbuf = buf;
  chunk->targetBufSize = self->bufSize;
  chunk->next = chunk;

  self->unallocatedBuffers--;
  logdebug("Allocated chunk of size %dB (up to %d), %d remaining\n",
        initsize, self->bufSize, self->unallocatedBuffers);
  return chunk;
}


/** Destroy the Buffer chain of a BufferedWriter
 *
 * \param self pointer to the BufferedWriter
 *
 * \return 0 on success, or a negative number otherwise
 */
int
destroyBufferChain(BufferedWriter* self) {
  BufferChunk *chunk, *start;

  if (!self) {
    return -1;
  }

  /* BufferChunk is a circular buffer */
  start = self->firstChunk;
  while( (chunk = self->firstChunk) && chunk!=start) {
    logdebug("Destroying BufferChunk at %p\n", chunk);
    self->firstChunk = chunk->next;

    mbuf_destroy(chunk->mbuf);
    oml_free(chunk);
  }

  pthread_cond_destroy(&self->semaphore);
  pthread_mutex_destroy(&self->lock);

  return 0;
}


/** Writing thread
 *
 * \param handle the stream to use the filters on
 * \return 1 if all the buffer chain has been processed, <1 otherwise
 */
static void*
threadStart(void* handle)
{
  int allsent = 1;
  BufferedWriter* self = (BufferedWriter*)handle;
  BufferChunk* chunk = self->firstChunk;

  while (self->active) {
    oml_lock(&self->lock, "bufferedWriter");
    pthread_cond_wait(&self->semaphore, &self->lock);
    // Process all chunks which have data in them
    do {
      if (mbuf_message(chunk->mbuf) > mbuf_rdptr(chunk->mbuf)) {
        allsent = processChunk(self, chunk);
      }
      // stop if we caught up to the writer

      if (chunk == self->writerChunk) break;

      if (allsent>0) {
        chunk = chunk->next;
      }
    } while(allsent > 0);
    oml_unlock(&self->lock, "bufferedWriter");
  }
  /* Drain this writer before terminating */
  /* XXX: “Backing-off for ...” messages might confuse the user as
   * we don't actually wait after a failure when draining at the end */
  while ((allsent=processChunk(self, chunk))>=-1) {
    if(allsent>0) {
      if (chunk == self->writerChunk) break;
      chunk = chunk->next;
    } else if (-1 == allsent) {
      sleep(self->backoff);
    }
  };
  self->retval = allsent;
  pthread_exit(&(self->retval));
}

/** Send data contained in one chunk.
 *
 * \param self BufferedWriter to process
 * \param chunk link of the chunk to process
 *
 * \return 1 if chunk has been fully sent, 0 if not, -1 on continuing back-off, -2 otherwise
 * \see oml_outs_write_f
 */
static int
processChunk(BufferedWriter* self, BufferChunk* chunk)
{
  assert(self);
  assert(self->meta_buf);
  assert(chunk);
  assert(chunk->mbuf);

  uint8_t* buf = mbuf_rdptr(chunk->mbuf);
  size_t size = mbuf_message_offset(chunk->mbuf) - mbuf_read_offset(chunk->mbuf);
  size_t sent = 0;

  MBuffer* meta = self->meta_buf;

  /* XXX: Should we use a timer instead? */
  time_t now;
  time(&now);
  if (difftime(now, self->last_failure_time) < self->backoff) {
    logdebug("%s: Still in back-off period (%ds)\n", self->outStream->dest, self->backoff);
    return -1;
  }

  chunk->reading = 1;

  while (size > sent) {
    long cnt = self->outStream->write(self->outStream, (void*)(buf + sent), size - sent,
                               meta->rdptr, meta->fill);
    if (cnt > 0) {
      sent += cnt;
      if (self->backoff) {
        self->backoff = 0;
        loginfo("%s: Connected\n", self->outStream->dest);
      }

    } else {
      self->last_failure_time = now;
      if (!self->backoff) {
        self->backoff = 1;
      } else if (self->backoff < UINT8_MAX) {
        self->backoff *= 2;
      }
      logwarn("%s: Error sending, backing off for %ds\n", self->outStream->dest, self->backoff);

      chunk->reading = 0;
      return -2;
    }
  }

  /* Check that we have sent everything, and reset the MBuffer
   * XXX: is this really needed? size>sent *should* be enough
   */
  mbuf_read_skip(chunk->mbuf, sent);
  chunk->reading = 0;
  /* XXX: Redundant with allsent */
  if (mbuf_write_offset(chunk->mbuf) == mbuf_read_offset(chunk->mbuf)) {
    mbuf_clear2(chunk->mbuf, 1);
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
