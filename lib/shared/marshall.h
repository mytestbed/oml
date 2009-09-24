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

/*! Resize the uchar buffer held by +mbuffer+ to +new_size+. This function
 * will copy to content of the old buffer and return +curr_p+.
 *
 * Return mbuffer->curr_p or NULL if resize failed.
 */
typedef unsigned char* (*ob_resize)(struct _omBuffer* mbuffer, int new_size);

typedef struct _omBuffer {
  unsigned char* curr_p;        //! Ptr to first 'empty' character
  int            buffer_remaining; //! Bytes remaining for this message
  int            buffer_fill;   //! number of bytes 'valid' (used in unmarshall)
  unsigned char* message_start;  // Begin of message if there are more than one in buffer

  unsigned char* buffer;        //! buffer to marshall into
  int            buffer_length; //! size of allocated buffer

  ob_resize      resize;
} OmlMBuffer;

int
marshall_measurements(
  OmlMBufferEx* mbuf,
  OmlMStream* ms,
  double      now
);

OmlMBufferEx*
marshall_init(
  OmlMBufferEx* mbuf,
  OmlMsgType packet_type
);

int
marshall_values(
  OmlMBufferEx*    mbuffer,
  OmlValue*      values,       //! type of sample
  int            value_count  //! size of above array
);

int
marshall_finalize(
  OmlMBufferEx*  mbuf
);

int
unmarshall_init(
  OmlMBuffer*  mbuf,
  OmlMsgType* type_p
);

int
unmarshall_measurements(
  OmlMBuffer* mbuf,
  int*        table_index,
  int*        seq_no_p,
  double*     ts_p,
  OmlValue*   values,          //! measurement values
  int         max_value_count  //! max. length of above array
);

int
unmarshall_values(
    OmlMBuffer*  mbuffer,
    OmlValue*    values,       //! type of sample
    int          max_value_count  //! max. length of above array
);

int
unmarshall_value(
  OmlMBuffer*  mbuffer,
  OmlValue*    value
);

unsigned char*
marshall_resize(
  OmlMBuffer*  mbuf,
  int new_size
);

unsigned char*
marshall_check_resize(
  OmlMBuffer* mbuf,
  size_t bytes
);

unsigned char*
find_sync (unsigned char* buf, int len);

#endif /*MARSHALL_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
