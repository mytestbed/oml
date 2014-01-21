/*
 * Copyright 2007-2014 National ICT Australia (NICTA), Australia
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
#ifndef MARSHAL_H_
#define MARSHAL_H_

#include <stdint.h>

#include "oml2/omlc.h"
#include "mbuf.h"

/** Represent whether a marshalled packet is short or long */
typedef enum {
  /** Short packet of size \ref PACKET_HEADER_SIZE bytes */
  OMB_DATA_P = 0x1,
  /** Long packet of size \ref PACKET_HEADER_SIZE + \ref STREAM_HEADER_SIZE bytes */
  OMB_LDATA_P = 0x2,
} OmlBinMsgType;


typedef struct {
    OmlBinMsgType type;
    size_t length;
    int values;
    int stream;
    int seqno;
    double timestamp;
} OmlBinaryHeader;

int marshal_measurements(MBuffer* mbuf, int stream, int seqno, double now);
int marshal_init(MBuffer* mbuf, OmlBinMsgType msgtype);
int marshal_values(MBuffer* mbuffer, OmlValue* values, int value_count);
int marshal_value(MBuffer* mbuf, OmlValueT val_type,  OmlValueU* val);
int marshal_finalize(MBuffer*  mbuf);
OmlBinMsgType marshal_get_msgtype (MBuffer *mbuf);


int unmarshal_init(MBuffer*  mbuf, OmlBinaryHeader* header);
int unmarshal_measurements(MBuffer* mbuf, OmlBinaryHeader* header,
                            OmlValue*  values, int max_value_count);
int unmarshal_values(MBuffer*  mbuffer, OmlBinaryHeader* header,
                      OmlValue* values, int max_value_count);
int unmarshal_value(MBuffer* mbuffer, OmlValue* value);
int unmarshal_typed_value (MBuffer* mbuf, const char* name, OmlValueT type, OmlValue* value);

uint8_t* find_sync (const uint8_t* buf, int len);

#endif /*MARSHAL_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
