/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
/*!\file text_writer.c
  \brief Implements a writer which stores results in a local file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ocomm/o_log.h>
#include "oml2/omlc.h"
#include "client.h"
#include "oml2/oml_out_stream.h"


typedef struct _omlFileOutStream {

  oml_outs_write_f write;
  oml_outs_close_f close;

  //----------------------------
  FILE* f;                      /* File to write result to */

} OmlFileOutStream;

size_t write(OmlOutStream* hdl, uint8_t* buffer, size_t  length);


/**
 * \fn OmlOutStream* file_stream_new(char* fileName)
 * \brief Create a new +OmlWriter+
 * \param fileName the destination file
 * \return a new +OmlWriter+
 */
OmlOutStream*
file_stream_new(
  char* fileName
) {
  OmlFileOutStream* self = (OmlFileOutStream *)malloc(sizeof(OmlFileOutStream));
  memset(self, 0, sizeof(OmlFileOutStream));

  if (strcmp(fileName, "stdout") == 0 || strcmp(fileName, "-") == 0) {
    self->f = stdout;
  } else {
    if ((self->f = fopen(fileName, "a+")) == NULL) {
      o_log(O_LOG_ERROR, "Can't open local storage file '%s'\n", fileName);
      return 0;
    }
  }

  self->write = write;

  return (OmlWriter*)self;
}

size_t
write(
  OmlOutStream* hdl,
  uint8_t* buffer,
  size_t  length
) {
  OmlFileOutStream* self = (OmlFileOutStream*)hdl;

  FILE* f = self->f;
  if (f == NULL) return -1;

  size_t count = fwrite(buffer, 1, length, f);
  return count;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
