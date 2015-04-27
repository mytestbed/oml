/*
 * Copyright 2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file zlib_stream.h
 * \brief An OmlOutStream implemenation using Zlib to compress data before writing it into another OmlOutStream.
 * \see OmlOutStream
 */
#include <assert.h>
#include <string.h>
#include <zlib.h>
#include <sys/time.h>

#include "oml2/omlc.h"
#include "mem.h"
#include "mstring.h"
#include "mbuf.h"
#include "oml2/oml_out_stream.h"
#include "zlib_stream.h"

static int zlib_stream_init(OmlZlibOutStream *self);
static ssize_t zlib_stream_write(OmlOutStream* stream, uint8_t* buffer, size_t  length);
static ssize_t zlib_stream_write_immediate(OmlOutStream* stream, uint8_t* buffer, size_t  length);
static inline ssize_t _zlib_stream_write(OmlZlibOutStream* self, uint8_t* buffer, size_t  length, int flush);
static int zlib_stream_close(OmlOutStream* stream);

/** Create a new OmlOutStream writing compressed data into another OmlOutStream
 * \param out pointer to the downstream OmlOutStream into which the compressed data will be written
 * \return a new OmlOutStream instance
 */
OmlOutStream*
zlib_stream_new(OmlOutStream *out)
{
  MString *dest;
  assert(out != NULL);
  OmlZlibOutStream* self = (OmlZlibOutStream *)oml_malloc(sizeof(OmlZlibOutStream));
  memset(self, 0, sizeof(OmlZlibOutStream));

  dest = mstring_create();
  mstring_sprintf(dest, "gzip+%s", out->dest);
  self->os.dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  logdebug("%s: Created OmlZlibOutStream\n", self->os.dest);

  self->os.write = zlib_stream_write;
  self->os.write_immediate = zlib_stream_write_immediate;
  self->os.close = zlib_stream_close;
  self->outs = out;

  if(!(self->encapheader = mbuf_create())) {
    logerror("%s: Cannot allocate encapsulation header buffer", self->os.dest);
    goto fail_free_self;
  }

  if (zlib_stream_init(self)) {
    logerror("%s: Cannot allocate Zlib structure", self->os.dest);
    goto fail_free_mbuf;
  }

  mbuf_write(self->encapheader, (uint8_t*)(ZENCAPHEADER), sizeof(ZENCAPHEADER)-1); /* Don't want the terminating nul byte */
  out_stream_set_header_data(self->outs, self->encapheader);

  if(!(self->in = oml_malloc(self->chunk_size))) {
    logerror("%s: Cannot allocate Zlib input buffer", self->os.dest);
    goto fail_free_mbuf;
  } else if (!(self->out = oml_malloc(self->chunk_size))) {
    logerror("%s: Cannot allocate Zlib output buffer", self->os.dest);
    goto fail_free_input;
  }
  return (OmlOutStream*)self;

fail_free_input:
  oml_free(self->in);

fail_free_mbuf:
  mbuf_destroy(self->encapheader);

fail_free_self:
  oml_free(self);

  return NULL;
}

/** (Re)initialise the Zlib stream.
 * \param self OmlZlibOutStream which Zlib stream needs (re)initialisation
 *
 * \return 0 on success, -1 otherwise
 * \see deflateInit2
 */
static int
zlib_stream_init(OmlZlibOutStream *self)
{
  int ret = -1;

  /* New stream; start by sending headers */
  self->os.header_written = 0;

  /* Initialise Zlib stream*/
  self->chunk_size = OML_ZLIB_CHUNKSIZE;
  self->zlevel = OML_ZLIB_ZLEVEL;

  ret = oml_zlib_init(&self->strm, OML_ZLIB_DEFLATE, self->zlevel);

  return ret;
}

/** Drain the input buffer, compressing and writing the data out
 *
 * self->strm.avail_in and self->strm.next_in must have been set prior to
 * calling this function, except when flushing data.
 *
 * \param self OmlZlibOutStream being written
 * \param flush instruct Zlib to flush output or not, should be one of (Z_NO_FLUSH, Z_SYNC_FLUSH, Z_PARTIAL_FLUSH, Z_BLOCK, Z_FULL_FLUSH or Z_FINISH; \see deflate)
 *
 * \return the amount of data consumed from the input buffer (i.e., NOT the amount of compressed data written out) on succes, -1 otherwise
 * \see oml_outs_write_f, zlib_stream_write
 */
static ssize_t
zlib_stream_deflate_write(OmlZlibOutStream* self, int flush)
{
  int ret = -1, deflate_ret = -1, more_data = 0;
  int have, had = self->strm.avail_in, had_this_round;

  assert(self);
  assert(self->outs);
  assert(self->outs->write);
  assert(self->out);

  do {
    had_this_round = self->strm.avail_in;
    self->strm.avail_out = self->chunk_size;
    self->strm.next_out = self->out;

    switch(deflate_ret=deflate(&self->strm, flush)) {
    case Z_STREAM_ERROR:
                        logerror("%s: Error deflating data\n", self->os.dest);
                        /* XXX: terminate stream */
                        ret = -1;
                        break;
    case Z_OK: /* do something if flush == Z_finish */
                        if(Z_FINISH == flush) {
                          more_data = 1;
                        }
                        /* Continue to the next block */
    case Z_STREAM_END:
                        have = self->chunk_size - self->strm.avail_out;
                        logdebug3("%s: Deflated %dB to %dB and wrote out to %s\n",
                            self->os.dest, had_this_round - self->strm.avail_in, have, self->outs->dest);
                        ret = out_stream_write(self->outs, self->out, have);
                        break;

    default:
                        logerror("%s: Unknow return from deflate: %d\n", self->os.dest, deflate_ret);
                        break;
    }

  } while(ret > 0 &&               /* Writing was successful, and */
      (self->strm.avail_out == 0 || /* While all the output buffer was used or */
       more_data)                   /* More compressed output should be coming before the end */
      );


  if (ret) {
    if(Z_NO_FLUSH == flush) {
      self->nwrites++;

    } else {
      time(&self->last_flush);
      self->nwrites = 0;
    }

  } else if(0 >= ret) {
    /* XXX No data was written, but we don't know why.
     * adopt the view of the underlying stream about the headers in case they need to be resent, and
     * reinitialise the ZLib stream if headers need to be re-written */
    self->os.header_written = self->outs->header_written;
    if(!self->os.header_written) {
      /* Get ready to send again */
      zlib_stream_init(self);
    }
  }

  return (ret>-1)?(had - self->strm.avail_in):-1;
}

/** Compress input data and write into downstream OmlOutStream
 * \copydetails oml_outs_write_f
 * \see oml_outs_write_f
 */
static ssize_t
zlib_stream_write(OmlOutStream* outs, uint8_t* buffer, size_t  length)
{
  time_t t;
  int flush = OML_ZLIB_FLUSH;
  OmlZlibOutStream* self = (OmlZlibOutStream*)outs;

  self->os.header_written = self->outs->header_written;
  out_stream_write_header(outs);

  time(&t);
  if (self->nwrites>=10 ||
      (t-self->last_flush) > 1) {
    flush = Z_FULL_FLUSH;
  }

  return _zlib_stream_write(self, buffer, length, flush);
}

/** Immediately write data into stream
 * \copydetails oml_outs_write_immediate_f
 * \see oml_outs_write_immediate_f
 */
static ssize_t
zlib_stream_write_immediate(OmlOutStream* outs, uint8_t* buffer, size_t  length)
{
  OmlZlibOutStream* self = (OmlZlibOutStream*)outs;

  return _zlib_stream_write(self, buffer, length, Z_FULL_FLUSH);
}

/** Helper in charge of actually passing the data to deflate(), with the selected flush mode.
 * \param self OmlZlibOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 * \param flush instruct Zlib to flush output or not, should be one of (Z_NO_FLUSH, Z_SYNC_FLUSH, Z_PARTIAL_FLUSH, Z_BLOCK, Z_FULL_FLUSH or Z_FINISH; \see deflate)
 *
 * \return the number of sent bytes on success, -1 otherwise
 *
 * \see oml_outs_write_f, zlib_stream_write, zlib_stream_write_immediate, zlib_stream_deflate_write, deflate
 */
static inline ssize_t
_zlib_stream_write(OmlZlibOutStream* self, uint8_t* buffer, size_t  length, int flush)
{
  if (!self) { return -1; }
  if (!self->outs) { return -1; }
  if (!length) { return 0; }

  self->strm.avail_in = length;
  self->strm.next_in = buffer;

  return zlib_stream_deflate_write(self, flush);
}

/** Called to close the socket
 * \copydetails oml_outs_close_f
 * \see oml_outs_close_f
 */
static int
zlib_stream_close(OmlOutStream* stream)
{
  int ret = -1;
  ssize_t len;
  OmlZlibOutStream* self = (OmlZlibOutStream*)stream;

  if (!self) { return -1; }
  assert(self);

  logdebug("Destroying OmlZlibOutStream to file %s at %p\n", self->os.dest, self);

  len =  self->strm.avail_out;
  zlib_stream_deflate_write(self, Z_FINISH);
  logdebug3("%s: Flushed the last %dB of the compressed stream\n", self->os.dest, len);
  deflateEnd(&self->strm);
  out_stream_close(self->outs);
  mbuf_destroy(self->encapheader);
  oml_free(self->in);
  oml_free(self->out);

  return ret;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
