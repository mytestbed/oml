/*
 * Copyright 2007-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file bin_writer.c
 * \brief An implementation of the OmlWriter interface functions (see oml2/oml_writer.h) that writes messages using the OML binary protocol \ref omspbin.
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

/** An OmlWriter using the binary marshalling functions \ref omspbin */
typedef struct OmlBinWriter {

  /*
   * Fields from OmlWriter interface
   */

  /** \see OmlWriter::meta */
  oml_writer_meta meta;
  /** \see OmlWriter::header_done */
  oml_writer_header_done header_done;
  /** \see OmlWriter::row_start */
  oml_writer_row_start row_start;
  /** \see OmlWriter::row_end */
  oml_writer_row_end row_end;
  /** \see OmlWriter::out */
  oml_writer_out out;
  /** \see OmlWriter::close */
  oml_writer_close close;
  /** \see OmlWriter::next */
  OmlWriter* next;
  /** \see OmlWriter::bufferedWriter */
  BufferedWriterHdl bufferedWriter;

  /*
   * Fields specific to the OmlBinWriter
   */

  /** Currently active MBuffer of the bufferedWriter chain */
  MBuffer* mbuf;

  /** Output stream to write into, through teh bufferedWriter */
  OmlOutStream* out_stream;

  /** Set to 1 when the writer is ready to use */
  int is_enabled;

  /* Type of messages to generate */
  OmlBinMsgType msgtype;

} OmlBinWriter;

static int owb_meta(OmlWriter* writer, char* str);
static int owb_header_done(OmlWriter* writer);

static int owb_row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int owb_row_cols(OmlWriter* writer, OmlValue* values, int value_count);
static int owb_row_end(OmlWriter* writer, OmlMStream* ms);

static OmlWriter *owb_close(OmlWriter* writer);

/** Create a new OmlBinWriter
 * \param out_stream OmlOutStream into which the data should be written
 *
 * \return a pointer to the new OmlBinWriter cast as an OmlWriter
 *
 * \see BufferedWriter
 */
OmlWriter*
bin_writer_new(OmlOutStream* out_stream)
{
  assert(out_stream != NULL);

  OmlBinWriter* self = (OmlBinWriter *)oml_malloc(sizeof(OmlBinWriter));
  memset(self, 0, sizeof(OmlBinWriter));

  self->bufferedWriter = bw_create(out_stream,
      omlc_instance->max_queue, 0);
  self->out_stream = out_stream;

  self->meta = owb_meta;
  self->header_done = owb_header_done;
  self->row_start = owb_row_start;
  self->row_end = owb_row_end;
  self->out = owb_row_cols;
  self->close = owb_close;

  self->msgtype = OMB_DATA_P; // Short packets.

  return (OmlWriter*)self;
}

/** Function called whenever some header metadata needs to be added.
 * \see oml_writer_meta
 *
 * XXX: Code duplication with owt_meta in the OmlTextWriter (#1101)
 */
static int
owb_meta(OmlWriter* writer, char* str)
{
  OmlBinWriter* self = (OmlBinWriter*)writer;
  if (self->bufferedWriter == NULL) {
    return 0;
  }

  MString *mstr = mstring_create();
  mstring_set (mstr, str);
  mstring_cat (mstr, "\n");
  bw_push_meta(self->bufferedWriter, (uint8_t*)mstring_buf (mstr), mstring_len (mstr));
  mstring_delete (mstr);
  return 1;
}

/** Function called to finalise meta header
 * \see oml_writer_header_done
 */
static int
owb_header_done(OmlWriter* writer)
{
  return (owb_meta(writer, "content: binary") && owb_meta(writer, ""));
}

/** Function called for every result value in a measurement tuple (sample)
 * \see oml_writer_out
 * \see marshal_values
 */
static int
owb_row_cols(OmlWriter* writer, OmlValue* values, int value_count)
{
  OmlBinWriter* self = (OmlBinWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) {
    return 0; /* previous use of mbuf failed */
  }

  int cnt = marshal_values(mbuf, values, value_count);
  return cnt == value_count;
}

/** Function called after all items in a tuple have been sent
 * \see oml_writer_row_start
 *
 * Prepare a new marshalled message with sequence and time information in the
 * writer's Mbuffer.
 *
 * This acquires a lock on the BufferedWriter (bw_get_write_buf(...,
 * exclusive=1)).
 *
 * \see BufferedWriter, bw_get_write_buf, marshal_init, marshal_measurements
 * \see gettimeofday(3)
 */
static int
owb_row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlBinWriter* self = (OmlBinWriter*)writer;
  assert(self->bufferedWriter != NULL);

  MBuffer* mbuf;
  if ((mbuf = self->mbuf = bw_get_write_buf(self->bufferedWriter, 1)) == NULL) {
    return 0;
  }

  marshal_init (mbuf, self->msgtype);
  marshal_measurements(mbuf, ms->index, ms->seq_no, now);
  return 1;
}

/** Function called after all items in a tuple have been sent
 * \see oml_writer_row_end
 *
 * This releases the lock on the BufferedWriter.
 *
 * \see BufferedWriter, bw_unlock_buf, marshal_finalize
 */
static int
owb_row_end(OmlWriter* writer, OmlMStream* ms) {
  (void)ms;
  OmlBinWriter* self = (OmlBinWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) {
    return 0; /* previous use of mbuf failed */
  }

  marshal_finalize(self->mbuf);
  if (marshal_get_msgtype (self->mbuf) == OMB_LDATA_P) {
    self->msgtype = OMB_LDATA_P; // Generate long packets from now on.
  }

  if (0 == ms->index) {
    /* This is schema0, also push the data into the meta_buf
     * to be replayed after a disconnection.
     *
     * At the moment, the oml_outs_write_f takes header information as a
     * whole, but does not push more once it has sent the initial block. Its
     * two last parameters are only used to resend the entirety of the headers
     * when a disconnection does occur, nothing before.
     *
     * We therefore send the extra piece of data the normal way, but also
     * record it, separately, in the meta_buf
     *
     * XXX: This logic should be in higher layer levels, but given the current
     * implementation, with some of it already spread down into the
     * OmlOutStream (oml_outs_write_f), this require a much bigger refactoring.
     * It is also duplicated with the OmlTextWriter (see #1101).
     */
    _bw_push_meta(self->bufferedWriter,
        mbuf_message(self->mbuf), mbuf_message_length(self->mbuf));
  }

  mbuf_begin_write(mbuf);

  self->mbuf = NULL;
  bw_msgcount_add(self->bufferedWriter, 1);
  bw_unlock_buf(self->bufferedWriter);
  return 1;
}

/** Function called to close the writer and free its allocated objects.
 * \see oml_writer_close
 */
static OmlWriter*
owb_close(OmlWriter* writer)
{
  OmlWriter *next;

  if(!writer) {
    return NULL;
  }

  OmlBinWriter *self = (OmlBinWriter*) writer;

  next = self->next;

  // Blocks until the buffered writer drains
  bw_close (self->bufferedWriter);
  oml_free(self);

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
