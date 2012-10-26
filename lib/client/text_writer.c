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
/*!\file text_writer.c
  \brief Implements a writer which stores results in a local file.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "oml2/oml_writer.h"
#include "log.h"
#include "oml_value.h"
#include "client.h"
#include "buffered_writer.h"

typedef struct {

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

  BufferedWriterHdl bufferedWriter;
  MBuffer* mbuf;        /* Pointer to active buffer from bufferedWriter */
  OmlOutStream* out_stream;
} OmlTextWriter;


static int meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);
static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int row_cols(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static int close(OmlWriter* writer);

/**
 * \brief Create a new +OmlWriter+
 * \param file_name the destination file
 * \return a new +OmlWriter+
 */
OmlWriter*
text_writer_new(OmlOutStream* out_stream)
{
  assert(out_stream != NULL);

  OmlTextWriter* self = (OmlTextWriter *)malloc(sizeof(OmlTextWriter));
  memset(self, 0, sizeof(OmlTextWriter));
  //pthread_mutex_init(&self->lock, NULL);

  self->bufferedWriter = bw_create(out_stream->write, out_stream, omlc_instance->max_queue, 0);
  self->out_stream = out_stream;

  self->meta = meta;
  self->header_done = header_done;
  self->row_start = row_start;
  self->row_end = row_end;
  self->out = row_cols;
  self->close = close;


  return (OmlWriter*)self;
}
/**
 * \brief Definition of the meta function of the oml net writer
 * \param writer the net writer that will send the data to the server
 * \param str the string to send
 * \return 1 if successful, 0 otherwise
 */
static int
meta(OmlWriter* writer, char* str)
{
  OmlTextWriter* self = (OmlTextWriter*)writer;
  if (self->bufferedWriter == NULL) return 0;

  MString *mstr = mstring_create();
  mstring_set (mstr, str);
  mstring_cat (mstr, "\n");
  bw_push_meta(self->bufferedWriter, (uint8_t*)mstring_buf (mstr), mstring_len (mstr));

  mstring_delete (mstr);
  return 1;
}

/**
 * \brief finish the writing of the first information
 * \param writer the writer that write this information
 * \return 1 if successful, 0 otherwise
 */
static int
header_done(OmlWriter* writer)
{
  return (meta(writer, "content: text") && meta(writer, ""));
}


/**
 * \brief write the result inside the file
 * \param writer pointer to writer instance
 * \param values type of sample
 * \param value_count size of above array
 * \return 1 if successful, 0 otherwise
 */
static int
row_cols(OmlWriter* writer, OmlValue* values, int value_count)
{
  OmlTextWriter* self = (OmlTextWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) return 0; /* previous use of mbuf failed */

  int i;
  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    int res;
    switch (oml_value_get_type(v)) {
    case OML_LONG_VALUE: {
      res = mbuf_print(mbuf, "\t%" PRId32, oml_value_clamp_long (v->value.longValue));
      break;
    }
    case OML_INT32_VALUE:  res = mbuf_print(mbuf, "\t%" PRId32,  v->value.int32Value);  break;
    case OML_UINT32_VALUE: res = mbuf_print(mbuf, "\t%" PRIu32,  v->value.uint32Value); break;
    case OML_INT64_VALUE:  res = mbuf_print(mbuf, "\t%" PRId64,  v->value.int64Value);  break;
    case OML_UINT64_VALUE: res = mbuf_print(mbuf, "\t%" PRIu64,  v->value.uint64Value); break;
    case OML_DOUBLE_VALUE: res = mbuf_print(mbuf, "\t%f",  v->value.doubleValue); break;
    case OML_STRING_VALUE: res = mbuf_print(mbuf, "\t%s",  omlc_get_string_ptr(*oml_value_get_value(v))); break;
    case OML_BLOB_VALUE: {
      const unsigned int max_bytes = 6;
      int bytes = v->value.blobValue.length < max_bytes ? v->value.blobValue.length : max_bytes;
      int i = 0;
      res = mbuf_print(mbuf, "blob ");
      for (i = 0; i < bytes; i++) {
        res = mbuf_print(mbuf, "%02x", ((uint8_t*)v->value.blobValue.ptr)[i]);
      }
      res = mbuf_print (mbuf, " ...");
      break;
    }
    default:
      res = -1;
      logerror( "Unsupported value type '%d'\n", oml_value_get_type(v));
      return 0;
    }
    if (res < 0) {
      mbuf_reset_write(mbuf);
      self->mbuf = NULL;
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
 * \return 1 if succesfull, 0 otherwise
 */
int
row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlTextWriter* self = (OmlTextWriter*)writer;
  assert(self->bufferedWriter != NULL);

  MBuffer* mbuf;
  if ((mbuf = self->mbuf = bw_get_write_buf(self->bufferedWriter, 1)) == NULL)
    return 0;

  mbuf_begin_write(mbuf);
  if (mbuf_print(mbuf, "%f\t%d\t%ld", now, ms->index, ms->seq_no)) {
    mbuf_reset_write(mbuf);
    self->mbuf = NULL;
    return 0;
  }
  return 1;
}

/**
 * \brief write the data after finalysing the data structure
 * \param writer the net writer that send the measurements
 * \param ms the stream of measurmenent
 * \return 1 if successful, 0 otherwise
 */
int
row_end(OmlWriter* writer, OmlMStream* ms)
{
  (void)ms;
  OmlTextWriter* self = (OmlTextWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) return 0; /* previous use of mbuf failed */

  int res;
  if ((res = mbuf_write(mbuf, (uint8_t*)"\n", 1)) != 0) {
    mbuf_reset_write(mbuf);
  } else {
    // success, lock in message
    mbuf_begin_write (mbuf);
  }
  self->mbuf = NULL;
  bw_unlock_buf(self->bufferedWriter);
  return res == 0;
}

/**
 * \brief Called to close the file
 * \param writer the file writer to close the socket in
 * \return 0
 */
static int
close(OmlWriter* writer)
{
  OmlTextWriter *self = (OmlTextWriter*) writer;

  // Blocks until the buffered writer drains
  bw_close (self->bufferedWriter);

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
