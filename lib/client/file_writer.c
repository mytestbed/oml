/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
/*!\file file_writer.c
  \brief Implements a writer which stores results in a local file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>

#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include <oml2/oml_writer.h>
#include <oml_value.h>

typedef struct _omlFileWriter {

  oml_writer_meta meta;
  oml_writer_header_done header_done;


  //! Called before and after writing individual
  // filter results with 'out'
  oml_writer_row_start row_start;
  oml_writer_row_end row_end;

  //! Writing a result.
  oml_writer_out out;

  oml_writer_close close;

  OmlWriter* next;

  //----------------------------

  FILE* f;          /* File to write result to */
  int   first_row;

} OmlFileWriter;


static int meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);
static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int out(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static int close(OmlWriter* writer);
/**
 * \brief Create a new +OmlWriter+
 * \param file_name the destination file
 * \return a new +OmlWriter+
 */
/*@null@*/OmlWriter*
file_writer_new(char* file_name)
{
  OmlFileWriter* self = (OmlFileWriter *)malloc(sizeof(OmlFileWriter));
  if (self == NULL) return NULL;
  memset(self, 0, sizeof(OmlFileWriter));
  self->first_row = 1;

  if (strcmp(file_name, "stdout") == 0 || strcmp(file_name, "-") == 0) {
    self->f = stdout;
  } else {
    if ((self->f = fopen(file_name, "a+")) == NULL) {
      logerror ("Can't open local storage file '%s'\n", file_name);
      return 0;
    }
  }

  self->meta = meta;
  self->header_done = header_done;
  self->row_start = row_start;
  self->row_end = row_end;
  self->out = out;
  self->close = close;

  return (OmlWriter*)self;
}
/**
 * \brief Definition of the meta function of the oml net writer
 * \param writer the net writer that will send the data to the server
 * \param str the string to send
 * \return 1 if the socket is not open, 0 if successful
 */
static int
meta(OmlWriter* writer, char* str)
{
  OmlFileWriter* self = (OmlFileWriter*)writer;
  FILE* f = self->f;
  if (f == NULL) return 0;

  fprintf(f, "%s\n", str);
  return 0;
}

/**
 * \brief finish the writing of the first information
 * \param writer the writer that write this information
 * \return
 */
static int
header_done(OmlWriter* writer)
{
  meta(writer, "content: text");
  meta(writer, "");

  return 0;
}


/**
 * \brief write the result inside the file
 * \param writer pointer to writer instance
 * \param values type of sample
 * \param value_count size of above array
 * \return 0 if sucessful 1 otherwise
 */
static int
out(OmlWriter* writer, OmlValue* values, int value_count)
{
  OmlFileWriter* self = (OmlFileWriter*)writer;
  FILE* f = self->f;
  if (f == NULL) return 1;

  int i;
  OmlValue* v = values;

  for (i = 0; i < value_count; i++, v++) {
    switch (v->type) {
    case OML_LONG_VALUE: {
      fprintf(f, "\t%" PRId32, oml_value_clamp_long (v->value.longValue));
      break;
    }
    case OML_INT32_VALUE:  fprintf(f, "\t%" PRId32,  v->value.int32Value);  break;
    case OML_UINT32_VALUE: fprintf(f, "\t%" PRIu32,  v->value.uint32Value); break;
    case OML_INT64_VALUE:  fprintf(f, "\t%" PRId64,  v->value.int64Value);  break;
    case OML_UINT64_VALUE: fprintf(f, "\t%" PRIu64,  v->value.uint64Value); break;
    case OML_DOUBLE_VALUE: fprintf(f, "\t%f",  v->value.doubleValue); break;
    case OML_STRING_VALUE: fprintf(f, "\t%s",  v->value.stringValue.ptr); break;
    case OML_BLOB_VALUE: {
      const unsigned int max_bytes = 6;
      int bytes = v->value.blobValue.fill < max_bytes ? v->value.blobValue.fill : max_bytes;
      int i = 0;
      fprintf (f, "blob ");
      for (i = 0; i < bytes; i++) {
        fprintf(f, "%02x", ((uint8_t*)v->value.blobValue.data)[i]);
      }
      fprintf (f, " ...");
      break;
    }
    default:
      logerror ("Unsupported value type '%d'\n", v->type);
      return 0;
    }
  }
  return 1;
}

/**
 * \brief before sending datastore information about the time and the stream
 * \param writer the netwriter to send data
 * \param ms the stream to store the measruement from
 * \param now a timestamp that represensent the current time
 * \return 1
 */
int
row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlFileWriter* self = (OmlFileWriter*)writer;
  FILE* f = self->f;
  if (f == NULL) return 1;

  if (self->first_row) {
    // need to add eoln to separate from header
    fprintf(f, "\n");
    self->first_row = 0;
  }

  fprintf(f, "%f\t%d\t%ld", now, ms->index, ms->seq_no);
  return 1;
}

/**
 * \brief write the data after finalysing the data structure
 * \param writer the net writer that send the measurements
 * \param ms the stream of measurmenent
 * \return 1
 */
int
row_end(OmlWriter* writer, OmlMStream* ms)
{
  (void)ms;
  OmlFileWriter* self = (OmlFileWriter*)writer;
  FILE* f = self->f;
  if (f == NULL) return 1;

  fprintf(f, "\n");
  return 1;
}

/**
 * \brief Called to close the file
 * \param writer the file writer to close the socket in
 * \return 0
 */
static int
close(OmlWriter* writer)
{
  OmlFileWriter* self = (OmlFileWriter*)writer;

  if (self->f != 0) {
    fclose(self->f);
    self->f= NULL;
  }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
