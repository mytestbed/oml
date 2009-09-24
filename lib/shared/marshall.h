/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
#ifndef MARSHALL_H_
#define MARSHALL_H_

#include <oml2/omlc.h>
#include "mbuf.h"

typedef enum _omPktType {
  OMB_DATA_P = 0x1
} OmlMsgType;

struct _omBuffer;

typedef struct _omlBinaryHeader OmlBinaryHeader;

struct _omlBinaryHeader
{
	OmlMsgType type;
	int length;
	int values;
	int stream;
	int seqno;
	double timestamp;
};

int
marshall_measurements(
  MBuffer* mbuf,
  int         stream,
  int         seqno,
  double      now
);

MBuffer*
marshall_init(
  MBuffer* mbuf,
  OmlMsgType packet_type
);

int
marshall_values(
  MBuffer*    mbuffer,
  OmlValue*      values,       //! type of sample
  int            value_count  //! size of above array
);

inline int marshall_value(MBuffer* mbuf, OmlValueT val_type,  OmlValueU* val);

int
marshall_finalize(
  MBuffer*  mbuf
);

int
unmarshall_init(
  MBuffer*  mbuf,
  OmlBinaryHeader* header
);

int
unmarshall_measurements(
  MBuffer* mbuf,
  OmlBinaryHeader* header,
  OmlValue*   values,          //! measurement values
  int         max_value_count  //! max. length of above array
);

int
unmarshall_values(
    MBuffer*  mbuffer,
	OmlBinaryHeader* header,
    OmlValue*    values,       //! type of sample
    int          max_value_count  //! max. length of above array
);

int
unmarshall_value(
  MBuffer*  mbuffer,
  OmlValue*    value
);

int
unmarshal_typed_value (MBuffer* mbuf, const char* name, OmlValueT type, OmlValue* value);

unsigned char*
find_sync (unsigned char* buf, int len);

#endif /*MARSHALL_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
