/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
/*!\file file_stream.c
  \brief Implements a stream writer that writes its input to a file on the local filesystem.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_out_stream.h>
#include "client.h"


typedef struct _omlFileOutStream {

  oml_outs_write_f write;
  oml_outs_close_f close;

  //----------------------------
  FILE* f;                      /* File to write result to */
  int   header_written;         // True if header has been written to file

} OmlFileOutStream;

static size_t file_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length);


/**
 * \fn OmlOutStream* file_stream_new(char* fileName)
 * \brief Create a new +OmlWriter+
 * \param fileName the destination file
 * \return a new +OmlWriter+
 */
OmlOutStream*
file_stream_new(const char *file)
{
  OmlFileOutStream* self = (OmlFileOutStream *)malloc(sizeof(OmlFileOutStream));
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

  self->write = file_stream_write;
  self->header_written = 0;
  return (OmlOutStream*)self;
}

/**
 * \brief write data to a file without any sanity check
 * \param hdl pointer to the OmlOutStream
 * \param buffer pointer to the buffer containing the data to write
 * \param buffer length of the buffer to write
 * \param header pointer to header information
 * \param buffer length of the header information
 * \return amount of data written, or -1 on error
 */
static inline size_t
_file_stream_write(
  FILE*    file_hdl,
  uint8_t* buffer,
  size_t   length
) {
  return fwrite(buffer, 1, length, file_hdl); //(FILE*)((OmlFileOutStream*)hdl)->f);
}

/**
 * \brief write data to a file
 * \param hdl pointer to the OmlOutStream
 * \param buffer pointer to the buffer containing the data to write
 * \param buffer length of the buffer to write
 * \return amount of data written, or -1 on error
 */
size_t
file_stream_write(
  OmlOutStream* hdl,
  uint8_t* buffer,
  size_t   length,
  uint8_t* header,
  size_t   header_length
) {
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

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

/**
 * \brief write data to a file and flush it afterwards
 *
 * Use fflush(3) after each right.
 *
 * \param hdl pointer to the OmlOutStream
 * \param buffer pointer to the buffer containing the data to write
 * \param buffer length of the buffer to write
 * \return amount of data written, or -1 on error
 */
size_t
file_stream_write_flush(
  OmlOutStream* hdl,
  uint8_t* buffer,
  size_t  length,
  uint8_t* header,
  size_t   header_length
) {
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (!self) return -1;
  FILE* f = self->f;
  if (f == NULL) return -1;

  size_t count = file_stream_write(hdl, buffer, length, header, header_length);
  fflush(f);

  return count;
}

/**
 * Set the buffering startegy of an +OmlOutStream+
 *
 * Tell whether fflush(3) should be used after each write.
 *
 * \param hdl the +OmlOutStream+
 * \param buffered if 0, unbuffered operation is used, otherwise buffered operation is
 * \parame return 0 on success, -1 on failure (+hdl+ was NULL)
 */
int file_stream_set_buffered(
    OmlOutStream* hdl,
    int buffered
) {
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (self == NULL) return -1;

  if(buffered)
    hdl->write=file_stream_write;
  else
    hdl->write=file_stream_write_flush;

  return 0;
}

/**
 * Get the buffering startegy of an +OmlOutStream+
 *
 * Returns 0 if fflush(3) is used after each write, 1 otherwise.
 *
 * \param hdl the +OmlOutStream+
 * \parame return 0 if unbuffered, 1 if buffered, -1 on failure (+hdl+ was NULL)
 */
int file_stream_get_buffered(
    OmlOutStream* hdl
) {
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  if (self == NULL) return -1;

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
