/*
 * Copyright 2010-2011 National ICT Australia (NICTA), Australia
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
#ifndef MESSAGE_H__
#define MESSAGE_H__

#include <stdint.h>
#include <schema.h>
#include <oml2/omlc.h>
#include <mbuf.h>

enum MessageType {
  MSG_BINARY,
  MSG_TEXT
};

struct oml_message {
  int stream;       // The stream this message belongs to.
  uint32_t seqno;   // Sequence number of this message.
  double timestamp; // Relative time stamp of this message.
  enum MessageType type; // Type of message (text/binary)
  uint32_t length;  // Length in octets of this message/line
  int count;        // Expected/actual count of fields in the measurement
                    // (not including protocol metadata)
};

typedef int (*msg_start_fn) (struct oml_message *msg, MBuffer *mbuf);
typedef int (*msg_values_fn) (struct oml_message *msg, MBuffer *mbuf,
                              struct schema *schema, OmlValue *values);

#endif /* MESSAGE_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
