/*
 * Copyright 2010-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file message.h
 * \brief Defines a container for either a text or binary message (struct oml_message) and the associated interface functions.
 */
#ifndef MESSAGE_H__
#define MESSAGE_H__

#include <stdint.h>

#include "oml2/omlc.h"
#include "schema.h"
#include "mbuf.h"

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
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
