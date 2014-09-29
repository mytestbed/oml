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

#include "oml2/omlc.h"
#include "mem.h"
#include "mstring.h"
#include "oml2/oml_out_stream.h"
#include "zlib_stream.h"

static ssize_t zlib_stream_write(OmlOutStream* stream, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length);
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
  mstring_sprintf(dest, "zlib+%s", out->dest);
  self->dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  logdebug("%s: Created OmlZlibOutStream\n", self->dest);
  logerror("%s: ZLib compression not implemented\n", self->dest);

  self->write = zlib_stream_write;
  self->close = zlib_stream_close;
  self->os = out;
  return (OmlOutStream*)self;
}

/** Compress input data and write into downstream OmlOutStream
 * \see oml_outs_write_f
 */
static ssize_t
zlib_stream_write(OmlOutStream* stream, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length)
{
  int ret = -1;
  OmlZlibOutStream* self = (OmlZlibOutStream*)stream;

  assert(self);
  assert(self->os);

  logerror("%s: ZLib compression not implemented\n", self->dest);
  return self->os->write(self->os, buffer, length, header, header_length);

  return ret;
}

/** Called to close the socket
 * \see oml_outs_close_f
 */
  static int
zlib_stream_close(OmlOutStream* stream)
{
  int ret = -1;
  OmlZlibOutStream* self = (OmlZlibOutStream*)stream;

  assert(self);
  assert(self->os);

  logerror("%s: ZLib compression not implemented\n", self->dest);
  logdebug("Destroying OmlZlibOutStream to file %s at %p\n", self->dest, self);

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
