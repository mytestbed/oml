#ifndef TEXT_H__
#define TEXT_H__

#include <oml2/omlc.h>
#include <mbuf.h>
#include "schema.h"
#include "message.h"

int text_read_msg_start  (struct oml_message *msg, MBuffer *mbuf);
int text_read_msg_values (struct oml_message *msg, MBuffer *mbuf,
                          struct schema *schema, OmlValue *values);

#endif /* TEXT_H__ */
