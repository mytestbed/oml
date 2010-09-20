#ifndef BINARY_H__
#define BINARY_H__

#include <oml2/omlc.h>
#include <mbuf.h>
#include "schema.h"
#include "message.h"

int bin_read_msg_start  (struct oml_message *msg, MBuffer *mbuf);
int bin_read_msg_values (struct oml_message *msg, MBuffer *mbuf,
                         struct schema *schema, OmlValue *values);

#endif /* BINARY_H__ */
