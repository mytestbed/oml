/*
 * Copyright 2010-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file text.c
 * \brief Text mode OMSP (\ref omsp \ref omsptext) serialisation of OML tuples.
 *
 * \page omsp
 * \section omsptext OMSP Text Protocol
 *
 * The text protocol is meant to simplify sourcing of measurement streams from
 * applications written in languages which are not supported by the OML library
 * or where the OML library is considered too heavy. It is primarily envisioned
 * for low-volume streams which do not require additional client side
 * filtering. There are native instrumentation (liboml2, <a
 * href="https://rubygems.org/gems/oml4r">OML4R</a>, <a
 * href="https://pypi.python.org/pypi/oml4py/">OML4Py</a>) but implementing the
 * protocol from scratch in any language of choice should be very straight
 * forward.
 *
 * The text protocol simply serialises metadata and values of a tuple as one
 * newline-terminated (`\n`), tab-separated (`\t`) line per sample.
 *
 * The textual representation of the types defined above is as follows:
 * - All numeric types are represented as *decimal* strings suitable for
 *   <a href="http://linux.die.net/man/3/strtod">strtod(3) and siblings</a>; using
 *   <a href="http://linux.die.net/man/3/snprintf">snprintf(3)</a>, with the relevant
 *   <a href="http://linux.die.net/man/0/inttypes.h">`PRIuN` format</a> if needed, should
 *   provide good functionality (at least V>=2; as of V<=3, there is no
 *   guarantee for the interpretation of non-decimal notations)
 * - Strings are represented directly (except for the nil-terminator) but some
 *   character values require special processing;
 *   - As the text protocol assigns special meaning to the tab and newline
 *     characters they would confused the parser if they appeared verbatim. To
 *     avoid this a simple backslash encoding is used: tab characters are
 *     represented by the *string* "`\t`", newlines by the *string* "`\n`" and
 *     backslash itself by the *string* "`\\`" (V>=4; no other backslash expansion
 *     is made *TODO* what if `\whatever` is input?);
 * - BLOBs are encoded using <a
 *   href="https://tools.ietf.org/html/rfc4648#section-4">BASE64 encoding</a>
 *   and the resulting string is sent. No line breaks are permitted within the
 *   BASE64-encoded string (V>=4);
 * - GUIDs are globally unique IDs used to link different measurements. These
 *   are treated as large numbers and thus represented as UINT64, unsigned
 *   decimal strings. (V>=4);
 * - booleans are encoded as any case-insensitive stem of FALSE or TRUE (e.g.,
 *   `fAL`, `trUe`, but generally `F` and `T` will suffice), being respectively
 *   False or True; any other value is considered True, *including '0'* (V>=4);
 * - vectors are encoded as a *space-separated* list in which the
 *   first element is the size of the vector followed by the vector
 *   elements themselves. Each vector entry is encoded according to
 *   its type as above. (V>=5).
 *
 * \subsection omsptextexample Example
 *
 * This example shows two streams, matching the \ref omspschemaexample "schema"
 * and \ref omspheadersexample "headers" examples.
 *
 *     0.903816 2 0 sample-1  1
 *     0.903904 1 0 sample-1  0.000000  0.000000
 *     1.903944 2 1 sample-2  2
 *     1.903961 1 1 sample-2  0.628319  0.587785
 *     2.460049 2 3 sample-3  3
 *     2.460557 1 3 sample-3  1.256637  0.951057
 *     3.461064 2 4 sample-4  4
 *     3.461103 1 4 sample-4  1.884956  0.951056
 *
 * *TODO*: Add example string and blob
 *
 */
#include "mbuf.h"
#include "marshal.h"
#include "oml_value.h"
#include "schema.h"
#include "message.h"

/**
 *  @brief Read an OmlValue value from +mbuf+.
 *
 *  The mbuf read pointer is assumed to be pointing to the start of a
 *  value.  The current line must be completely contained in the mbuf,
 *  including the final newline '\n'.  A value is read from the mbuf
 *  assuming that its type matches the one contained in the value
 *  object.
 *
 *  Values are assumed to be tab-delimited, and so either a tab or a
 *  newline terminates the next field in the buffer.  This function
 *  modifies the mbuf contents by null-terminating the field,
 *  replacing the tab or newline with '\0'.
 *
 *  After this function finishes, if it was successful, the mbuf read
 *  pointer (mbuf_rdptr()) will be modified to point to the first
 *  character following the field.
 *
 *  @param mbuf The buffer containing the next field value
 *  @param value An OmlValue to store the parse value in. The type of this
 *  value on input determines what type of value the function should
 *  attempt to read.
 *  @param line_length length of the current line in the buffer, measured
 *  from the buffer's current read pointer (mbuf_rdptr()).
 *
 *  @return -1 if an error occurred; otherwise, the length of the
 *  field in bytes.
 *
 */
static int
text_read_value (MBuffer *mbuf, OmlValue *value, size_t line_length)
{
  uint8_t *line = mbuf_rdptr (mbuf);
  size_t field_length = mbuf_find (mbuf, '\t');
  int len;
  int ret = 0;
  uint8_t save;

  /* No tab '\t' found on this line --> final field */
  if (field_length == (size_t)-1 || field_length > line_length)
    len = line_length;
  else
    len = field_length;

  save = line[len];
  line[len] = '\0';

  ret = oml_value_from_s (value, (char*)line);
  line[len] = save;

  len++; // Skip the separator
  if (ret == -1) {
    return ret;
  } else {
    mbuf_read_skip (mbuf, len);
    return len;
  }
}

int
text_read_msg_start (struct oml_message *msg, MBuffer *mbuf)
{
  int len = mbuf_find (mbuf, '\n'); // Find length of line
  OmlValue value;
  int bytes = 0;

  oml_value_init(&value);

  if (len == -1)
    return 0; // Haven't got a full line

  msg->length = (uint32_t)len + 1;

  /* Read the timestamp first */
  oml_value_set_type(&value, OML_DOUBLE_VALUE);
  bytes = text_read_value (mbuf, &value, len);
  if (bytes == -1)
    return -1;
  else
    len -= bytes;

  msg->timestamp = omlc_get_double(*oml_value_get_value(&value));

  /* Read the stream index */
  oml_value_set_type(&value, OML_UINT32_VALUE);
  bytes = text_read_value (mbuf, &value, len);
  if (bytes == -1)
    return -1;
  else
    len -= bytes;

  msg->stream = omlc_get_uint32(*oml_value_get_value(&value));

  /* Read the sequence number */
  oml_value_set_type(&value, OML_UINT32_VALUE);
  bytes = text_read_value (mbuf, &value, len);
  if (bytes == -1)
    return -1;
  else
    len -= bytes;

  msg->seqno = omlc_get_uint32(*oml_value_get_value(&value));

  oml_value_reset(&value);

  return msg->length;
}

/**
 *  @brief Read a vector of values matching schema from mbuf.
 *
 *  Reads as many values as required to match the schema from the
 *  mbuf, and stores them in the values vector.
 *
 */
int
text_read_msg_values (struct oml_message *msg, MBuffer *mbuf, struct schema *schema, OmlValue *values)
{
  int length = msg->length;
  int index = mbuf_message_index (mbuf);
  int msg_remaining = length - index;
  int i = 0;

  length = msg_remaining;

  for (i = 0; i < schema->nfields; i++) {
    int bytes;
    oml_value_set_type(&values[i], schema->fields[i].type);
    bytes = text_read_value (mbuf, &values[i], length);

    if (bytes == -1)
      return -1;
    else
      length -= bytes;
  }

  msg->count = schema->nfields;
  // rdptr now points to start of next line/message
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
