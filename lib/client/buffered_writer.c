/*
 * Copyright 2011-2015 National ICT Australia (NICTA)
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
#include "oml2/oml_out_stream.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"

#include "client.h"
#include "buffered_writer.h"

/** Default target size in each MBuffer of the chunk */
#define DEF_CHAIN_BUFFER_SIZE 1024

/** A chunk of data to be put in a circular chain */
typedef struct BufferChunk {

  struct BufferChunk*  next;	/**< Link to the next buffer in the chunk */

  MBuffer* mbuf;		/**< MBuffer used for storage */
  size_t   targetBufSize;	/**< Target maximal size of mbuf for that chunk */

  pthread_mutex_t lock;		/**< Mutex preventing access to chunk's MBuffer by both threads at once */

  int nmessages;		/**< Number of messages contained in this chunk */

} BufferChunk;

/** A writer reading from a chain of BufferChunks */
struct BufferedWriter {
  int  active;			/**< Set to !0 if buffer is active; 0 kills the thread */

  long unallocatedBuffers;	/**< Number of links which can still be allocated */
  size_t bufSize;		/**< Target size of MBuffer in each chunk*/

  OmlOutStream*    outStream;	/**< Opaque handler to  the output stream*/

  BufferChunk* writerChunk;	/**< Chunk where the data gets stored until it's pushed out */
  BufferChunk* nextReaderChunk;	/**< Chunk where to read the data next */
  BufferChunk* firstChunk;	/**< Immutable entry into the chain */

  MBuffer*     meta_buf;	/**< Buffer holding protocol headers */
  MBuffer*     read_buf;	/**< Read buffer used to double-buffer reading, \see processChunk */

  pthread_mutex_t lock;		/**< Mutex protecting the chain structure */
  pthread_mutex_t meta_lock;	/**< Mutex protecting the headers buffer */
  pthread_cond_t semaphore;	/**< Semaphore indicating that more data has been written */
  pthread_t  readerThread;	/**< Thread in charge of reading the queue and writing the data out */

  time_t last_failure_time;	/**< Time of the last failure, to backoff for REATTEMP_INTERVAL before retryying, \see processChunk*/
  uint8_t backoff;		/**< Backoff time, in seconds, \see processChunk */

  int retval;			/**< Return status from the thread */

  int nlost;			/**< Number of lost messages since last query */

};
#define REATTEMP_INTERVAL 5    //! Seconds to open the stream again

static BufferChunk* getNextWriteChunk(BufferedWriter* self, BufferChunk* current);
static BufferChunk* getNextReadChunk(BufferedWriter* self);
static BufferChunk* createBufferChunk(BufferedWriter* self);
static int destroyBufferChain(BufferedWriter* self);
static void* bufferedWriterThread(void* handle);
static int processChunk(BufferedWriter* self, BufferChunk* chunk);

static int _bw_push_meta(BufferedWriter* instance, uint8_t* data, size_t size);

/** Create a BufferedWriter instance
 *
 * \param outStream opaque OmlOutStream handler
 * \param queueCapacity maximal size [B] of the internal queue queueCapaity/chunkSize will be used (at least 2)
 * \param chunkSize size [B] of buffer space allocated at a time, set to 0 for default (DEF_CHAIN_BUFFER_SIZE)
 * \return an instance pointer if successful, NULL otherwise
 *
 * \see DEF_CHAIN_BUFFER_SIZE
 */
BufferedWriter*
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

    if(NULL == (self->writerChunk = self->nextReaderChunk = self->firstChunk = createBufferChunk(self))) {
      oml_free(self);
      self = NULL;

    } else if(NULL == (self->meta_buf = mbuf_create())) {
      destroyBufferChain(self);
      oml_free(self);
      self = NULL;

    } else if(NULL == (self->read_buf = mbuf_create())) {
      destroyBufferChain(self);
      oml_free(self);
      self = NULL;

    } else {
      /* Initialize mutex and condition variable objects */
      pthread_cond_init(&self->semaphore, NULL);
      pthread_mutex_init(&self->lock, NULL);
      logdebug3("%s: initialised mutex %p\n", self->outStream->dest, &self->lock);
      pthread_mutex_init(&self->meta_lock, NULL);
      logdebug3("%s: initialised mutex %p\n", self->outStream->dest, &self->meta_lock);

      /* Initialize and set thread detached attribute */
      pthread_attr_t tattr;
      pthread_attr_init(&tattr);
      pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_JOINABLE);
      self->active = 1;
      pthread_create(&self->readerThread, &tattr, bufferedWriterThread, (void*)self);
    }

    out_stream_set_header_data(outStream, self->meta_buf);
  }

  return (BufferedWriter*)self;
}

/** Close an output stream and destroy the objects.
 *
 * \param instance handle (i.e., pointer) to a BufferedWriter
 */
void
bw_close(BufferedWriter* instance)
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

  out_stream_close(self->outStream);
  destroyBufferChain(self);
  oml_free(self);
}

/** Add some data to the end of the header buffer.
 *
 * \warning This function tries to acquire the lock on the header data, and
 * releases it when done.
 *
 * \param instance BufferedWriter handle
 * \param data Pointer to data to add
 * \param size size of data
 * \return 1 if success, 0 otherwise
 *
 * \see _bw_push_meta
 */
int
bw_push_meta(BufferedWriter* instance, uint8_t* data, size_t size)
{
  int result = 0;
  BufferedWriter* self = (BufferedWriter*)instance;
  if (oml_lock(&self->meta_lock, __FUNCTION__) == 0) {
    result = _bw_push_meta(instance, data, size);
    oml_unlock(&self->meta_lock, __FUNCTION__);
  }
  return result;
}

/** Add some data to the end of the header buffer (lock must be held).
 *
 * \warning This function is the same as bw_push_meta except it assumes that
 * the lock on the header data is already acquired.
 *
 * \copydetails bw_push_meta
 *
 * \see bw_push_meta
 *
 */
static int
_bw_push_meta(BufferedWriter* instance, uint8_t* data, size_t size)
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
bw_msgcount_add(BufferedWriter* instance, int nmessages) {
  instance->writerChunk->nmessages += nmessages;
  return instance->writerChunk->nmessages;
}

/** Reset the message count in a BufferChunk and return its previous value.
 *
 * \param instance BufferChunk handle
 *
 * \return the number of messages in the current writer BufferChunk before resetting
 *
 * \see bw_msgcount_add, bw_nlost_reset
 */
int
bw_msgcount_reset(BufferChunk* chunk) {
  int n = chunk->nmessages;
  chunk->nmessages = 0;
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
bw_nlost_reset(BufferedWriter* instance) {
  int n = instance->nlost;
  instance->nlost = 0;
  return n;
}
/** Return an MBuffer with exclusive access
 *
 * \param instance BufferedWriter handle
 *
 * \return an MBuffer instance if success to write in, NULL otherwise
 * \see bw_release_write_buf
 */
MBuffer*
bw_get_write_buf(BufferedWriter* instance)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  if (!self->active) { return 0; }

  BufferChunk* chunk = self->writerChunk;
  if (chunk == NULL) { return 0; }
  oml_lock(&chunk->lock, __FUNCTION__);

  MBuffer* mbuf = chunk->mbuf;
  if (mbuf_write_offset(mbuf) >= chunk->targetBufSize) {
    chunk = getNextWriteChunk(self, chunk);
    mbuf = chunk->mbuf;
  }
  return mbuf;
}


/** Return and unlock MBuffer
 * \param instance BufferedWriter handle for which a buffer was previously obtained through bw_get_write_buf
 *
 * \see bw_get_write_buf
 */
void
bw_release_write_buf(BufferedWriter* instance)
{
  BufferedWriter* self = (BufferedWriter*)instance;
  pthread_cond_signal(&self->semaphore); /* assume we locked for a reason */
  oml_unlock(&self->writerChunk->lock, __FUNCTION__);
}

/** Find the next empty write chunk, sets self->writerChunk to it and returns * it.
 *
 * We only use the next one if it is empty. If not, we essentially just filled
 * up the last chunk and wrapped around to the socket reader. In that case, we
 * either create a new chunk if the overall buffer can still grow, or we drop
 * the data from the current one.
 *
 * \warning A lock on the current writer chunk should be held prior to calling
 * this function. It will be released, and the returned writerChunk will be
 * similarly locked.
 *
 * \param self BufferedWriter pointer
 * \param current locked BufferChunk to use or from which to find the next
 * \return a locked BufferChunk in which data can be stored
 */
BufferChunk*
getNextWriteChunk(BufferedWriter* self, BufferChunk* current) {
  int nlost;
  BufferChunk* nextBuffer;

  assert(current != NULL);
  nextBuffer = current->next;
  oml_unlock(&current->lock, __FUNCTION__);
  assert(nextBuffer != NULL);

  oml_lock(&self->lock, __FUNCTION__);
  if (nextBuffer == self->nextReaderChunk) {
    if (self->unallocatedBuffers > 0) {
      /* The next buffer is the next to be read, but we can allocate more,
       * allocate a new buffer, insert it after the current writer, and use it */
      nextBuffer = createBufferChunk(self);
      assert(nextBuffer); /** \todo Use existing buffer if allocation fails */
      oml_unlock(&self->lock, __FUNCTION__);

      oml_lock(&current->lock, __FUNCTION__);
      nextBuffer->next = current->next;
      current->next = nextBuffer; /* we have a lock on this one */
      oml_unlock(&current->lock, __FUNCTION__);

      oml_lock(&self->lock, __FUNCTION__);

    } else {
      /* The next buffer is the next to be read, and we cannot allocate more,
       * use it, dropping unread data, and advance the read pointer */
      self->nextReaderChunk = nextBuffer->next;
    }
  }

  self->writerChunk = nextBuffer;
  nlost = bw_msgcount_reset(self->writerChunk);
  self->nlost += nlost;
  oml_unlock(&self->lock, __FUNCTION__);
  oml_lock(&nextBuffer->lock, __FUNCTION__);
  if (nlost) {
    logwarn("%s: Dropping %d samples (%dB)\n", self->outStream->dest, nlost, mbuf_fill(nextBuffer->mbuf));
  }
  mbuf_clear2(nextBuffer->mbuf, 0);

  // Now we just need to copy the message from current to self->writerChunk
  int msgSize = mbuf_message_length(current->mbuf);
  if (msgSize > 0) {
    mbuf_write(nextBuffer->mbuf, mbuf_message(current->mbuf), msgSize);
    mbuf_reset_write(current->mbuf);
  }

  return nextBuffer;
}

/** Get the next available reader chunck (even if there is nothing to read)
 *
 * \param self BufferedWriter pointer
 * \return a BufferChunk frome which data can next be read
 */
static BufferChunk*
getNextReadChunk(BufferedWriter* self) {
  BufferChunk* chunk;
  chunk = self->nextReaderChunk;
  self->nextReaderChunk = self->nextReaderChunk->next;
  return chunk;
}

/** Initialise a BufferChunk for a BufferedWriter.
 *
 * \warning A lock on the BufferedWriter should be held if the readerThread
 * has already been started.
 *
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
  pthread_mutex_init(&chunk->lock, NULL);
  logdebug3("%s: initialised chunk mutex %p\n", self->outStream->dest, &chunk->lock);

  self->unallocatedBuffers--;
  logdebug("%s: Allocated chunk of size %dB (up to %d), %d remaining\n",
        self->outStream->dest, initsize, self->bufSize, self->unallocatedBuffers);
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
    pthread_mutex_destroy(&chunk->lock);
    oml_free(chunk);
  }

  mbuf_destroy(self->meta_buf);
  mbuf_destroy(self->read_buf);

  pthread_cond_destroy(&self->semaphore);
  pthread_mutex_destroy(&self->meta_lock);
  pthread_mutex_destroy(&self->lock);

  return 0;
}


/** Writing thread.
 *
 * \param handle the stream to use the filters on
 * \return 1 if all the buffer chain has been processed, <1 otherwise
 */
static void*
bufferedWriterThread(void* handle)
{
  int allsent = 1;
  BufferedWriter* self = (BufferedWriter*)handle;
  BufferChunk* chunk = self->firstChunk;

  while (self->active) {
    oml_lock(&self->lock, __FUNCTION__);
    pthread_cond_wait(&self->semaphore, &self->lock);
    // Process all chunks which have data in them
    do {
      oml_unlock(&self->lock, __FUNCTION__);
      allsent = processChunk(self, chunk);

      oml_lock(&self->lock, __FUNCTION__);
      /* Stop if we caught up to the writer... */
      if (chunk == self->writerChunk) { break; }
      /* ...otherwise, move on to the next chunk */
      if (allsent>0) {
        bw_msgcount_reset(chunk);
        chunk = getNextReadChunk(self);
      }
    } while(allsent > 0);
    oml_unlock(&self->lock, __FUNCTION__);
  }
  /* Drain this writer before terminating */
  /* XXX: “Backing-off for ...” messages might confuse the user as
   * we don't actually wait after a failure when draining at the end */
  while ((allsent=processChunk(self, chunk))>=-1) {
    if(allsent>0) {
      if (chunk == self->writerChunk) { break; };

      oml_lock(&self->lock, __FUNCTION__);
      bw_msgcount_reset(chunk);
      chunk = getNextReadChunk(self);
      oml_unlock(&self->lock, __FUNCTION__);
    }
  };
  self->retval = allsent;
  pthread_exit(&(self->retval));
}

/** Send data contained in one chunk.
 *
 * \warning This implementation needs to access self->last_failure_time,
 * self->backoff and self->read_buf, amongst others. It is currently the only
 * one that touches these variable. Therefore, the lock is *NOT* acquired on the
 * BufferedWriter when only these fields are accessed.
 *
 * \warning This function acquires the lock on the chunk being processed for
 * the time it takes to check it and swap the double buffer.
 *
 * \warning This function acquires the lock on the meta buffer.
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
  time_t now;
  int ret = -2;
  ssize_t cnt = 0;
  MBuffer *read_buf = NULL;
  assert(self);
  assert(self->read_buf);
  assert(chunk);
  assert(chunk->mbuf);

  time(&now);
  if (difftime(now, self->last_failure_time) < self->backoff) {
    logdebug("%s: Still in back-off period (%ds)\n", self->outStream->dest, self->backoff);
    ret = -1;
    goto processChunk_cleanup;
  }

  if (mbuf_message(self->read_buf) > mbuf_rdptr(self->read_buf)) {
    /* There is unread data in the double buffer */
    ret = 0;
  }

  oml_lock(&chunk->lock, __FUNCTION__);
  if (ret < 0 && (mbuf_message(chunk->mbuf) >= mbuf_rdptr(chunk->mbuf))) {
    /* The double buffer is empty, but there is unread data in the read buffer, swap MBuffers */
    read_buf = chunk->mbuf;

    chunk->mbuf = self->read_buf;
    bc_msgcount_reset(chunk);

    self->read_buf = read_buf;

    ret = 1; /* XXX: We are processing the chunk, but we should remember to change this status on error */
  }
  oml_unlock(&chunk->lock, __FUNCTION__);

  if (ret < 0) {
    /* We didn't find any data to read, we're done */
    ret = 1;
    goto processChunk_cleanup;
  }

  while (mbuf_write_offset(self->read_buf) > mbuf_read_offset(self->read_buf)) {
    oml_lock(&self->meta_lock, __FUNCTION__); /* If there was a disconnection, out_stream_write() will
                                                 read and transmit the contents of the meta buffer first,
                                                 so we need to logk it, just in case... */
    cnt = out_stream_write(self->outStream, mbuf_rdptr(self->read_buf),
        (mbuf_message_offset(self->read_buf) - mbuf_read_offset(self->read_buf)));
    oml_unlock(&self->meta_lock, __FUNCTION__);
    if (cnt > 0) {
      mbuf_read_skip(self->read_buf, cnt);
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
      ret = -1;
      goto processChunk_cleanup;
    }
  }

processChunk_cleanup:
  return ret;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
