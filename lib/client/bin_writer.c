/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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
/*!\file net_writer.c
  \brief Implements a writer which sends results over the network
*/

#include <log.h>
#include <ocomm/o_socket.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include "oml2/omlc.h"
#include "client.h"
#include "oml2/oml_writer.h"
#include "oml2/oml_filter.h"
#include <marshal.h>
#include <mbuf.h>
#include "buffered_writer.h"
#include <assert.h>

#define DEF_PROTOCOL "tcp"
#define DEF_PORT 3003

/**
 * \struct OmlNetWriter
 * \brief a structure that send the data to the server
 */

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

  /**********************/

  BufferedWriterHdl bufferedWriter;
  MBuffer* mbuf;        /* Pointer to active buffer from bufferedWriter */

  OmlOutStream* out_stream;

  int        is_enabled;
} OmlBinProtoWriter;

static int write_meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);

static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int row_cols(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static int close(OmlWriter* writer);



/**
 * \brief Create a new +OmlWriter+
 * \param protocol the transport protocol
 * \param location the host and the port number of the server
 * \return a new +OmlWriter+
 */
OmlWriter*
bin_writer_new(
  OmlOutStream* out_stream
) {
  assert(out_stream != NULL);

  OmlBinProtoWriter* self = (OmlBinProtoWriter *)malloc(sizeof(OmlBinProtoWriter));
  memset(self, 0, sizeof(OmlBinProtoWriter));

  self->bufferedWriter = bw_create(out_stream->write, out_stream, omlc_instance->max_queue, 0);
  self->out_stream = out_stream;

  self->meta = write_meta;
  self->header_done = header_done;
  self->row_start = row_start;
  self->row_end = row_end;
  self->out = row_cols;

  self->close = close;

  return (OmlWriter*)self;
}

/**
 * \brief Definition of the write_meta function of the oml net writer
 * \param writer the net writer that will send the data to the server
 * \param str the string to send
 * \return 1 if the socket is not open, 0 if successful
 */
static int
write_meta(
  OmlWriter* writer,
  char* str
) {
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  if (self->bufferedWriter == NULL) return 0;

  MString *mstr = mstring_create();
  mstring_set (mstr, str);
  mstring_cat (mstr, "\n");
  bw_push(self->bufferedWriter, (uint8_t*)mstring_buf (mstr), mstring_len (mstr));
  mstring_delete (mstr);
  return 1;
}

/**
 * \brief finish the writing of the first information
 * \param writer the writer that write this information
 * \return
 */
static int
header_done(
  OmlWriter* writer
) {
  return (write_meta(writer, "content: binary") && write_meta(writer, ""));
}

/**
 * \fn static int row_cols(OmlWriter* writer, OmlValue*  values, int value_count)
 * \brief marshal and then transfer the values
 * \param writer pointer to writer instance
 * \param values type of sample
 * \param value_count size of above array
 * \return 0 if sucessful 1 otherwise
 */
static int
row_cols(
  OmlWriter* writer,
  OmlValue*  values,
  int        value_count
) {
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) return 0; /* previous use of mbuf failed */

  int cnt = marshal_values(mbuf, values, value_count);
  return cnt == value_count;
}

/**
 * \brief before sending datastore information about the time and the stream
 * \param writer the netwriter to send data
 * \param ms the stream to store the measruement from
 * \param now a timestamp that represensent the current time
 * \return 1
 */
int
row_start(
  OmlWriter* writer,
  OmlMStream* ms,
  double now
) {
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  assert(self->bufferedWriter != NULL);

  MBuffer* mbuf;
  if ((mbuf = self->mbuf = bw_get_write_buf(self->bufferedWriter, 1)) == NULL)
    return 0;

  mbuf_begin_write(mbuf);
  marshal_measurements(mbuf, ms->index, ms->seq_no, now);
  return 1;
}

/**
 * \brief send the data after finalysing the data structure
 * \param writer the net writer that send the measurements
 * \param ms the stream of measurmenent
 * \return 1
 */
int
row_end(
  OmlWriter* writer,
  OmlMStream* ms
) {
  (void)ms;
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) return 0; /* previous use of mbuf failed */

  marshal_finalize(self->mbuf);

  mbuf_begin_write(mbuf);

  self->mbuf = NULL;
  bw_unlock_buf(self->bufferedWriter);
  return 1;
}

static int
close(OmlWriter* writer)
{
  OmlBinProtoWriter *self = (OmlBinProtoWriter*) writer;

  // Blocks until the buffered writer drains
  bw_close (self->bufferedWriter);

  return 1;
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
