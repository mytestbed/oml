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
/*!\file net_writer.c
  \brief Implements a writer which sends results over the network
*/

#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include "oml2/omlc.h"
#include "oml2/oml_writer.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_log.h"
#include "client.h"
#include "marshal.h"
#include "mbuf.h"
#include "buffered_writer.h"
#include "assert.h"

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
  OmlBinMsgType msgtype; /* Type of messages to generate */
} OmlBinProtoWriter;

static int write_meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);

static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int row_cols(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static OmlWriter *close(OmlWriter* writer);



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

  OmlBinProtoWriter* self = (OmlBinProtoWriter *)xmalloc(sizeof(OmlBinProtoWriter));
  memset(self, 0, sizeof(OmlBinProtoWriter));

  self->bufferedWriter = bw_create(out_stream->write, out_stream, omlc_instance->max_queue, 0);
  self->out_stream = out_stream;

  self->meta = write_meta;
  self->header_done = header_done;
  self->row_start = row_start;
  self->row_end = row_end;
  self->out = row_cols;
  self->close = close;

  self->msgtype = OMB_DATA_P; // Short packets.

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
  bw_push_meta(self->bufferedWriter, (uint8_t*)mstring_buf (mstr), mstring_len (mstr));
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

/** Prepare a new marshalled message with sequence and time information in the writer's Mbuffer.
 *
 * This acquires a lock on the BufferedWriter (bw_get_write_buf(..., exclusive=1)).
 *
 * \param writer pointer to OmlWriter to write the data out on
 * \param ms pointer to OmlMStream outputting the data
 * \param now current timestamp
 * \return 1
 * \see BufferedWriter, bw_get_write_buf, marshal_init, marshal_measurements
 * \see gettimeofday(3)
 */
int row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  assert(self->bufferedWriter != NULL);

  MBuffer* mbuf;
  if ((mbuf = self->mbuf = bw_get_write_buf(self->bufferedWriter, 1)) == NULL)
    return 0;

  marshal_init (mbuf, self->msgtype);
  marshal_measurements(mbuf, ms->index, ms->seq_no, now);
  return 1;
}

/** Finalise the marshalled message and output it on the writer.
 *
 * This releases the lock on the BufferedWriter.
 *
 * \param writer pointer to OmlWriter to write the data out on
 * \param ms pointer to OmlMStream outputting the data
 * \return 1
 * \see BufferedWriter,bw_unlock_buf, marshal_finalize
 */
int row_end(OmlWriter* writer, OmlMStream* ms) {
  (void)ms;
  OmlBinProtoWriter* self = (OmlBinProtoWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) return 0; /* previous use of mbuf failed */

  marshal_finalize(self->mbuf);
  if (marshal_get_msgtype (self->mbuf) == OMB_LDATA_P)
    self->msgtype = OMB_LDATA_P; // Generate long packets from now on.

  mbuf_begin_write(mbuf);

  self->mbuf = NULL;
  bw_unlock_buf(self->bufferedWriter);
  return 1;
}

/** Close the binary writer and free data structures.
 *
 * This function is designed so it can be used in a while loop to clean up the
 * entire linked list:
 *
 *   while( (w=w->close(w)) );
 *
 * \param writer pointer to the writer to close
 * \return w->next (can be null)
 */
static OmlWriter *close(OmlWriter* writer)
{
  OmlWriter *next;

  if(!writer)
    return NULL;

  OmlBinProtoWriter *self = (OmlBinProtoWriter*) writer;

  next = self->next;

  // Blocks until the buffered writer drains
  bw_close (self->bufferedWriter);
  xfree(self);

  return next;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
