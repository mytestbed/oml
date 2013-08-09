/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file text_writer.c
 * \brief An implementation of the OmlWriter interface functions (see oml2/oml_writer.h) that serializes measurement tuples using the OML text protocol \ref omsptext.
 */

#include <alloca.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "oml2/oml_writer.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "client.h"
#include "buffered_writer.h"
#include "string_utils.h"
#include "base64.h"

typedef struct OmlTextWriter {

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

  /*
   * Fields specific to the OmlTextWriter
   */

  /** Buffered writer into which the serialised data is written */
  BufferedWriterHdl bufferedWriter;
  /** Currently active MBuffer of the bufferedWriter chain */
  MBuffer* mbuf;

  /** Output stream to write into, through teh bufferedWriter */
  OmlOutStream* out_stream;

} OmlTextWriter;

static int owt_meta(OmlWriter* writer, char* str);
static int owt_header_done(OmlWriter* writer);

static int owt_row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int owt_row_cols(OmlWriter* writer, OmlValue* values, int value_count);
static int owt_row_end(OmlWriter* writer, OmlMStream* ms);

static OmlWriter* owt_close(OmlWriter* writer);

/** Create a new OmlTextWriter
 * \param out_stream OmlOutStream into which the data should be written
 *
 * \return a pointer to the new OmlTextWriter cast as an OmlWriter
 *
 * \see BufferedWriter
 */
OmlWriter*
text_writer_new(OmlOutStream* out_stream)
{
  assert(out_stream != NULL);

  OmlTextWriter* self = (OmlTextWriter *)xmalloc(sizeof(OmlTextWriter));
  memset(self, 0, sizeof(OmlTextWriter));

  self->bufferedWriter = bw_create(out_stream, omlc_instance->max_queue, 0);
  self->out_stream = out_stream;

  self->meta = owt_meta;
  self->header_done = owt_header_done;
  self->row_start = owt_row_start;
  self->row_end = owt_row_end;
  self->out = owt_row_cols;
  self->close = owt_close;


  return (OmlWriter*)self;
}

/** Function called whenever some header metadata needs to be added.
 * \see oml_writer_meta
 *
 * XXX: Code duplication with owb_meta in the OmlBinWriter (#1101)
 */
static int
owt_meta(OmlWriter* writer, char* str)
{
  OmlTextWriter* self = (OmlTextWriter*)writer;
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
owt_header_done(OmlWriter* writer)
{
  return (owt_meta(writer, "content: text") && owt_meta(writer, ""));
}


/** Function called for every result value in a measurement tuple (sample)
 * \see oml_writer_out
 */
static int
owt_row_cols(OmlWriter* writer, OmlValue* values, int value_count)
{
  char *enc;
  OmlTextWriter* self = (OmlTextWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) {
    return 0; /* previous use of mbuf failed */
  }

  int i;
  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    int res;
    switch (oml_value_get_type(v)) {
    case OML_LONG_VALUE:
      res = mbuf_print(mbuf, "\t%" PRId32, oml_value_clamp_long (omlc_get_long(*oml_value_get_value(v))));
      break;

    case OML_INT32_VALUE:
      res = mbuf_print(mbuf, "\t%" PRId32,
          omlc_get_int32(*oml_value_get_value(v)));
      break;
    case OML_UINT32_VALUE:
      res = mbuf_print(mbuf, "\t%" PRIu32,
          omlc_get_uint32(*oml_value_get_value(v)));
      break;
    case OML_INT64_VALUE:
      res = mbuf_print(mbuf, "\t%" PRId64,
          omlc_get_int64(*oml_value_get_value(v)));
      break;
    case OML_UINT64_VALUE:
      res = mbuf_print(mbuf, "\t%" PRIu64,
          omlc_get_uint64(*oml_value_get_value(v)));
      break;

    case OML_DOUBLE_VALUE:
      res = mbuf_print(mbuf, "\t%f",
          omlc_get_double(*oml_value_get_value(v)));
      break;

    case OML_STRING_VALUE:
      if(omlc_get_string_ptr(*oml_value_get_value(v)) &&
          0 < omlc_get_string_length(*oml_value_get_value(v))) {
        enc = xmalloc(backslash_encode_size(omlc_get_string_size(v->value)));
        backslash_encode(omlc_get_string_ptr(v->value), enc);
        res = mbuf_print(mbuf, "\t%s", enc);
        xfree(enc);

      } else {
        logdebug ("Attempting to send NULL or empty string; string of length 0 will be sent\n");
        res = mbuf_print(mbuf, "\t");
      }
      break;

    case OML_BLOB_VALUE: {
      if(omlc_get_blob_ptr(*oml_value_get_value(v)) &&
          0 < omlc_get_blob_length(*oml_value_get_value(v))) {
        enc = xmalloc(base64_size_string(omlc_get_blob_length(*oml_value_get_value(v))));
        base64_encode_blob(omlc_get_blob_length(*oml_value_get_value(v)), omlc_get_blob_ptr(*oml_value_get_value(v)), enc);
        res = mbuf_print(mbuf, "\t%s", enc);
        xfree(enc);

      } else {
        logdebug ("Attempting to send NULL or empty blob; blob of length 0 will be sent\n");
        res = mbuf_print(mbuf, "\t");
      }
      break;
    }

    case OML_GUID_VALUE:
      res = mbuf_print(mbuf, "\t%" PRIu64, omlc_get_guid(*oml_value_get_value(v)));
      break;

    case OML_BOOL_VALUE:
      res = mbuf_print(mbuf, "\t%c", (omlc_get_bool(*oml_value_get_value(v))!=OMLC_BOOL_FALSE)?'T':'F');
      break;

    default:
      res = -1;
      logerror("%s: Unsupported value type '%d'\n", __FUNCTION__, oml_value_get_type(v));
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

/** Function called after all items in a tuple have been sent
 * \see oml_writer_row_start
 */
static int
owt_row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlTextWriter* self = (OmlTextWriter*)writer;
  assert(self->bufferedWriter != NULL);

  MBuffer* mbuf;
  if ((mbuf = self->mbuf = bw_get_write_buf(self->bufferedWriter, 1)) == NULL) {
    return 0;
  }

  mbuf_begin_write(mbuf);
  if (mbuf_print(mbuf, "%f\t%d\t%ld", now, ms->index, ms->seq_no)) {
    mbuf_reset_write(mbuf);
    self->mbuf = NULL;
    return 0;
  }
  return 1;
}

/** Function called after all items in a tuple have been sent
 * \see oml_writer_row_end
 */
static int
owt_row_end(OmlWriter* writer, OmlMStream* ms)
{
  (void)ms;
  OmlTextWriter* self = (OmlTextWriter*)writer;
  MBuffer* mbuf;
  if ((mbuf = self->mbuf) == NULL) {
    return 0; /* previous use of mbuf failed */
  }

  int res;
  if ((res = mbuf_write(mbuf, (uint8_t*)"\n", 1)) != 0) {
    mbuf_reset_write(mbuf);

  } else {
    // success, lock in message
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
       * It is also duplicated with the OmlBinWriter (see #1101).
       */
      _bw_push_meta(self->bufferedWriter,
          mbuf_message(self->mbuf), mbuf_message_length(self->mbuf));
    }

    mbuf_begin_write (mbuf);
  }

  self->mbuf = NULL;
  bw_unlock_buf(self->bufferedWriter);
  return res == 0;
}

/** Function called to close the writer and free its allocated objects.
 * \see oml_writer_close
 */
static OmlWriter*
owt_close(OmlWriter* writer)
{
  OmlWriter *next;

  if(!writer) {
    return NULL;
  }

  OmlTextWriter *self = (OmlTextWriter*) writer;

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
