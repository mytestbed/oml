/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
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
#include <arpa/inet.h>

#include "oml2/omlc.h"
#include "oml_value.h"
#include "mbuf.h"
#include "marshal.h"
#include "schema.h"
#include "message.h"

#ifndef SYNC_BYTE
#define SYNC_BYTE 0xAA
#endif

#ifndef OMB_DATA_P
#define OMB_DATA_P 0x1
#endif

#ifndef OMB_LDATA_P
#define OMB_LDATA_P 0x2
#endif

static int
bin_read_value (MBuffer *mbuf, OmlValue *value)
{
  return unmarshal_value (mbuf, value);
}

/** Find the next sync bytes in an MBuffer, and advance read pointer accordingly if found.
 *
 * The previous message is also consumed.
 *
 * \param mbuf MBuffer to look into
 * \return (>=0) offset if found, -1 otherwise (including if there is not enough data in the buffer)
 * \see find_sync, mbuf_read_skip, mbuf_consume_message
 */
int bin_find_sync (MBuffer *mbuf)
{
  int len, offset = -1;
  uint8_t *buf, *sync_pos;

  len = mbuf_remaining (mbuf);
  if (len < 2) {
    /* Optimisation: get out early */
    return -1;
  }

  buf = mbuf_rdptr (mbuf);
  sync_pos = find_sync(buf, len);
  if (sync_pos) {
    offset = sync_pos - buf;
    mbuf_read_skip (mbuf, offset);
    mbuf_consume_message(mbuf);
  }

  return offset;
}

size_t
bin_value_size (OmlValue *value)
{
  switch (oml_value_get_type(value)) {
  case OML_LONG_VALUE:
  case OML_INT32_VALUE:
  case OML_UINT32_VALUE:
    return sizeof (uint32_t) + 1;
  case OML_INT64_VALUE:
  case OML_UINT64_VALUE:
    return sizeof (uint64_t) + 1;
  case OML_DOUBLE_VALUE:
    return 6;
  case OML_STRING_VALUE:
    return 2 + omlc_get_string_length(*oml_value_get_value(value));
  case OML_BLOB_VALUE:
    return 1 + sizeof (uint32_t) + value->value.blobValue.length;
  default:
    return 0;
  }
}

/*
 * Read the start of the new message; detect which stream it belongs
 * to, what the length of the message is, the sequence number, and the
 * timestamp.  Fill in the msg struct with this information.
 *
 * Return value == 0 means "wait for more data"
 * Return value == -1 means "error in protocol" or memory alloc error
 * Return value > 0 means "following message length is the many bytes"
 */
int
bin_read_msg_start (struct oml_message *msg, MBuffer *mbuf)
{
  OmlValue value;

  oml_value_init(&value);

  if (msg == NULL || mbuf == NULL)
    return -1;

  /* First, find the sync position */
  int sync_pos = bin_find_sync (mbuf);

  if (sync_pos == -1)
    return -1;

  mbuf_begin_read (mbuf);

  if (mbuf_remaining (mbuf) < 3) {
    return 0; // Not enough data to determine packet type
  }

  mbuf_read_skip (mbuf, 2); // Skip the sync bytes

  uint8_t packet_type;
  uint16_t msglen16;
  uint32_t length;
  uint32_t header_length;
  mbuf_read (mbuf, &packet_type, 1);

  switch (packet_type) {
  case OMB_DATA_P:
    // FIXME:  Return type (maybe not enough bytes)
    mbuf_read (mbuf, (uint8_t*)&msglen16, 2);
    msglen16 = ntohs (msglen16);
    length = (uint32_t)msglen16;
    header_length = 5;
    break;
  case OMB_LDATA_P:
    mbuf_read (mbuf, (uint8_t*)&length, 4);
    length = ntohl (length);
    header_length = 7;
    break;
  default:
    return -1; // Unknown packet type
  }

  if (mbuf_remaining (mbuf) < length)
    return 0; /* Not enough bytes for the full message */

  // Now get the count and index
  uint8_t count;
  uint8_t stream;

  mbuf_read (mbuf, &count, 1);
  mbuf_read (mbuf, &stream, 1);

  msg->type = MSG_BINARY;
  msg->stream = stream;
  msg->length = length + header_length;
  msg->count = count;


  oml_value_set_type(&value, OML_INT32_VALUE);
  // FIXME: check for error (e.g. type mismatch)
  bin_read_value (mbuf, &value);
  msg->seqno = omlc_get_int32(*oml_value_get_value(&value));

  oml_value_set_type(&value, OML_DOUBLE_VALUE);
  // FIXME: check for error (e.g. type mismatch)
  bin_read_value (mbuf, &value);
  msg->timestamp = omlc_get_double(*oml_value_get_value(&value));

  oml_value_reset(&value);

  return msg->length;
}

int
bin_read_msg_values (struct oml_message *msg, MBuffer *mbuf, struct schema *schema, OmlValue *values)
{
  int i = 0;

  if (msg->count != schema->nfields)
    return -1;

  for (i = 0; i < schema->nfields; i++) {
    int bytes;

    bytes = bin_read_value (mbuf, &values[i]);

    if (bytes == -1)
      return -1;
  }

  mbuf_consume_message (mbuf);
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
