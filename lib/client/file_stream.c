/*
 * Copyright 2007-2014 National ICT Australia (NICTA)
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

static inline int file_stream_close(OmlOutStream* hdl);

/** Create a new out stream for writing into a local file.
 *
 * \param file destination file (oml_strndup()'d locally)
 * \return a new OmlOutStream instance
 *
 * \see oml_strndup
 */
OmlOutStream*
file_stream_new(const char *file)
{
  MString *dest;
  OmlFileOutStream* self = (OmlFileOutStream *)oml_malloc(sizeof(OmlFileOutStream));
  memset(self, 0, sizeof(OmlFileOutStream));

  loginfo ("File_stream: opening local storage file '%s'\n", file);

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
  self->dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  self->write = file_stream_write;
  self->close = file_stream_close;
  self->header_written = 0;
  return (OmlOutStream*)self;
}

/** Write data to a file without any sanity check
 * \param file_hdl FILE pointer
 * \param buffer pointer to the buffer containing the data to write
 * \param buffer length of the header information
 * \return amount of data written, or -1 on error
 */
static inline ssize_t
_file_stream_write(FILE* file_hdl, uint8_t* buffer, size_t length)
{
  return fwrite(buffer, 1, length, file_hdl); //(FILE*)((OmlFileOutStream*)hdl)->f);
}

/** Write data to a file
 * \param hdl pointer to the OmlOutStream
 * \param buffer pointer to the buffer containing the data to write
 * \param length length of the buffer to write
 * \param header pointer to an optional buffer containing headers to be sent after (re)connecting
 * \param header_length length of the header to write; must be 0 if header is NULL
 * \return amount of data written, or -1 on error
 * \see _file_stream_write
 */
ssize_t
file_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t length, uint8_t* header, size_t header_length)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  /* The header can be NULL, but header_length MUST be 0 in that case */
  assert(header || !header_length);

  if (!self) return -1;
  FILE* f = self->f;
  if (f == NULL) return -1;

  if (! self->header_written) {
    size_t count;
    if ((count = _file_stream_write(f, header, header_length)) < header_length) {
      // TODO: This is not completely right as we end up rewriting the same header
      // if we can only partially write it. At this stage we think this is too hard 
      // to deal with and we assume it doesn't happen.
      if (count > 0) {
        // PANIC
        logerror("Only wrote part of the header. May screw up further processing\n");
      }
      return 0;
    }
    self->header_written = 1;
  }
  size_t count = _file_stream_write(f, buffer, length);
  return count;
}

/** Write data to a file and flush it afterwards
 *
 * Use fflush(3) after each right.
 *
 * \param hdl pointer to the OmlOutStream
 * \param buffer pointer to the buffer containing the data to write
 * \param length length of the buffer to write
 * \param header pointer to an optional buffer containing headers to be sent after (re)connecting
 * \param header_length length of the header to write; must be 0 if header is NULL
 * \return amount of data written, or -1 on error
 */
ssize_t
file_stream_write_flush(OmlOutStream* hdl, uint8_t* buffer, size_t length, uint8_t* header, size_t header_length)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (!self) return -1;
  FILE* f = self->f;
  if (f == NULL) return -1;

  size_t count = file_stream_write(hdl, buffer, length, header, header_length);
  fflush(f);

  return count;
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

  if (self == NULL) return -1;

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

  if (self == NULL) return -1;

  return (hdl->write==file_stream_write);
}

/** Close an OmlFileOutStream's output file
 * \param hdl pointer to the OmlFileOutStream
 * \return amount of data written, or -1 on error
 */
static inline int
file_stream_close(OmlOutStream* hdl)
{
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;
  int ret = -1;

  logdebug("Destroying OmlFileOutStream to file %s at %p\n", self->dest, self);

  if (self->f != NULL) {
    ret = fclose(self->f);
    self->f = NULL;
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
 End:
 vim: sw=2:sts=2:expandtab
*/
