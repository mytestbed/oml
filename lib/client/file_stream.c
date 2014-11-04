/*
 * Copyright 2007-2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file file_stream.c
 * \brief An OmlOutStream implementation that writer that writes measurement tuples to a file on the local filesystem.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "oml2/omlc.h"
#include "oml2/oml_out_stream.h"
#include "ocomm/o_log.h"
#include "mem.h"
#include "mstring.h"
#include "client.h"
#include "file_stream.h"

static ssize_t file_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length);
static ssize_t file_stream_write_immediate(OmlOutStream* hdl, uint8_t* buffer, size_t  length);
static ssize_t file_stream_write_flush(OmlOutStream* hdl, uint8_t* buffer, size_t  length);
static int file_stream_close(OmlOutStream* hdl);

/** Create a new out stream for writing into a local file.
 *
 * *Don't forget to associate header data if you need it*
 *
 * \param file destination file (oml_strndup()'d locally)
 * \return a new OmlOutStream instance
 *
 * \see oml_strndup, out_stream_set_header_data
 */
OmlOutStream*
file_stream_new(const char *file)
{
  MString *dest;
  OmlFileOutStream* self = (OmlFileOutStream *)oml_malloc(sizeof(OmlFileOutStream));
  memset(self, 0, sizeof(OmlFileOutStream));

  if (strcmp(file, "stdout") == 0 || strcmp(file, "-") == 0) {
    self->f = stdout;
  } else {
    if ((self->f = fopen(file, "a+")) == NULL) {
      logerror ("Can't open local storage file '%s'\n", file);
      return 0;
    }
  }

  dest = mstring_create();
  mstring_sprintf(dest, "file:%s", file);
  self->os.dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  logdebug("%s: Created OmlFileOutStream\n", self->os.dest);

  self->os.write = file_stream_write;
  self->os.write_immediate = file_stream_write_immediate;
  self->os.close = file_stream_close;
  return (OmlOutStream*)self;
}

/** Write data to a file
 * \copy oml_outs_write_f
 * \see file_stream_write_immediate
 */
static ssize_t
file_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t length)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;
  size_t count;

  if (!self) { return -1; }
  if (!length) { return 0; }

  out_stream_write_header(hdl);

  count = file_stream_write_immediate(hdl, buffer, length);
  return count;
}

/** Write data to a file without any sanity check
 * \copydetails oml_outs_write_immediate_f
 */
static ssize_t
file_stream_write_immediate(OmlOutStream *outs, uint8_t* buffer, size_t length)
{
  OmlFileOutStream *self = (OmlFileOutStream*) outs;

  if (!self) { return -1; }
  assert(self->f);

  return fwrite(buffer, 1, length, self->f);
}

/** Write data to a file and flush it afterwards
 * \copy oml_outs_write_f
 * \see file_stream_write,file_stream_write_immediate
 */
ssize_t
file_stream_write_flush(OmlOutStream* hdl, uint8_t* buffer, size_t length)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (!self) { return -1; }
  assert(self->f);

  size_t count = file_stream_write(hdl, buffer, length);
  fflush(self->f);

  return count;
}

/** Close an OmlFileOutStream's output file
 * \copydetails oml_outs_write_f
 */
static int
file_stream_close(OmlOutStream* hdl)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;
  int ret = -1;

  if(!self) { return 0; }

  logdebug("Destroying OmlFileOutStream to file %s at %p\n", self->os.dest, self);

  if (self->f != NULL) {
    ret = fclose(self->f);
    self->f = NULL;
  }
  return ret;
}

/** * Set the buffering startegy of an OmlOutStream
 *
 * Tell whether fflush(3) should be used after each write.
 *
 * \param hdl the OmlOutStream
 * \param buffered if 0, unbuffered operation is used, otherwise buffered operation is
 * \return 0 on success, -1 on failure (+hdl+ was NULL)
 */
int
file_stream_set_buffered(OmlOutStream* hdl, int buffered)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (!self) { return -1; }

  if(buffered) {
    hdl->write=file_stream_write;
  } else {
    hdl->write=file_stream_write_flush;
  }

  return 0;
}

/** * Get the buffering startegy of an OmlOutStream
 *
 * Returns 0 if fflush(3) is used after each write, 1 otherwise.
 *
 * \param hdl the OmlOutStream
 * \return 0 if unbuffered, 1 if buffered, -1 on failure (hdl was NULL)
 */
int
file_stream_get_buffered(OmlOutStream* hdl)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (!self) { return -1; }

  return (hdl->write==file_stream_write);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
