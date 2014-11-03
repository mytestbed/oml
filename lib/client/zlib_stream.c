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

#include "oml2/omlc.h"
#include "mem.h"
#include "mstring.h"
#include "oml2/oml_out_stream.h"
#include "zlib_stream.h"

static ssize_t zlib_stream_write(OmlOutStream* stream, uint8_t* buffer, size_t  length);
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
  self->dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  logdebug("%s: Created OmlZlibOutStream\n", self->dest);

  self->write = zlib_stream_write;
  self->close = zlib_stream_close;
  self->os = out;

  /* Initialise Zlib stream*/
  self->chunk_size = 16384; /* XXX */
  self->zlevel = Z_DEFAULT_COMPRESSION;

  self->strm.zalloc = Z_NULL;
  self->strm.zfree = Z_NULL;
  self->strm.opaque = Z_NULL;
  if (deflateInit2(&self->strm, self->zlevel, Z_DEFLATED, OML_ZLIB_WINDOWBITS, 8, Z_DEFAULT_STRATEGY) != Z_OK) {
    logerror("%s: Cannot allocate Zlib structure", self->dest);
    oml_free(self);
    return NULL;
  }

  if(!(self->in = oml_malloc(self->chunk_size))) {
    logerror("%s: Cannot allocate Zlib input buffer", self->dest);
    oml_free(self);
  } else if (!(self->out = oml_malloc(self->chunk_size))) {
    logerror("%s: Cannot allocate Zlib output buffer", self->dest);
    oml_free(self->in);
    oml_free(self);
  }
  return (OmlOutStream*)self;
}

/** Drain the input buffer, compressing and writing the data out
 *
 * self->strm.avail_in and self->strm.next_in must have been set prior to
 * calling this function, except when flushing data.
 *
 * \param self OmlZlibOutStream being written
 * \param flush instruct Zlib to flush output or not, should be one of (Z_NO_FLUSH, Z_SYNC_FLUSH, Z_PARTIAL_FLUSH, Z_BLOCK, Z_FULL_FLUSH or Z_FINISH; \see deflate)
 * \param header pointer to the beginning of header data to write in case of disconnection
 * \param header_length length of header data to write in case of disconnection
 *
 * \return the amount of data consumed from the input buffer (i.e., NOT the amount of compressed data written out) on succes, -1 otherwise
 * \see oml_outs_write_f, zlib_stream_write
 */
static ssize_t
zlib_stream_deflate_write(OmlZlibOutStream* self, int flush)
{
  int ret = -1;
  int have, had = self->strm.avail_in, had_this_round;

  assert(self);
  assert(self->os);
  assert(self->os->write);
  assert(self->out);

  do {
    had_this_round = self->strm.avail_in;
    self->strm.avail_out = self->chunk_size;
    self->strm.next_out = self->out;

    switch(deflate(&self->strm, flush)) {
    case Z_STREAM_ERROR:
                        logerror("%s: Error deflating data\n", self->dest);
                        /* XXX: terminate stream */
                        return -1;
                        break;
    }

    have = self->chunk_size - self->strm.avail_out;
    logdebug3("%s: Deflated %dB to %dB and wrote out to %s\n",
        self->dest, had_this_round - self->strm.avail_in, have, self->os->dest);
    ret = self->os->write(self->os, self->out, have);

  } while(self->strm.avail_out == 0 /* While all the output buffer is used */
      || (Z_FINISH == flush && Z_OK == ret /* More compressed output coming */
       ));

  return had - self->strm.avail_in;
}

/** Compress input data and write into downstream OmlOutStream
 * \see oml_outs_write_f
 */
static ssize_t
zlib_stream_write(OmlOutStream* stream, uint8_t* buffer, size_t  length)
{
  int ret = -1;
  OmlZlibOutStream* self = (OmlZlibOutStream*)stream;

  if (!self) { return -1; }
  if (!self->os) { return -1; }
  if (!length) { return 0; }

  /* Header (re)writing management is done by the underlying OmlOutStream*/

  self->strm.avail_in = length;
  self->strm.next_in = buffer;

  ret = zlib_stream_deflate_write(self, Z_NO_FLUSH);

  return ret;
}

/** Called to close the socket
 * \see oml_outs_close_f
 */
  static int
zlib_stream_close(OmlOutStream* stream)
{
  int ret = -1;
  ssize_t len;
  OmlZlibOutStream* self = (OmlZlibOutStream*)stream;

  assert(self);
  assert(self->os);

  logdebug("Destroying OmlZlibOutStream to file %s at %p\n", self->dest, self);

  len =  self->strm.avail_out;
  zlib_stream_deflate_write(self, Z_FINISH);
  logdebug3("%s: Flushed the last %dB of the compressed stream\n", self->dest, len);
  deflateEnd(&self->strm);
  if(self->os) {
    self->os->close(self->os);
    self->os = NULL;
  }

  oml_free(self->dest);
  oml_free(self);

  return ret;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
