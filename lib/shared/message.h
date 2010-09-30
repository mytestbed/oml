
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
