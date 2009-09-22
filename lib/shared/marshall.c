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
/*!\file marshall.c
  \brief Implements marhsalling and unmarshalling of basic types for transmission across the network.

*/

#include <ocomm/o_log.h>
#include <math.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "marshall.h"


#define BIG_S 15
#define BIG_L 30

#define LONG_T   0x1
#define DOUBLE_T 0x2
#define DOUBLE_NAN 0x3
#define STRING_T 0x4

#define SYNC_BYTE 0xAA

#define MIN_LENGTH  64  // Min. length of string buffer allocation


inline int marshall_value(OmlMBuffer* mbuf, OmlValueT val_type,  OmlValueU* val);

unsigned char*
find_sync (unsigned char* buf, int len)
{
  int i;

  for (i = 1; i < len; i++)
	{
	  if (buf[i] == SYNC_BYTE && buf[i-1] == SYNC_BYTE)
		return &buf[i-1];
	}
  return NULL;
}

/**
 * \fn OmlMBuffer* marshall_init(OmlMBuffer*  mbuf, OmlMsgType  packet_type)
 * \brief
 * \param mbuf
 * \param packet_type
 * \return
 */
OmlMBuffer*
marshall_init(
  OmlMBuffer*  mbuf,
  OmlMsgType  packet_type
) {

  if (mbuf->buffer == NULL || mbuf->buffer_length < MIN_LENGTH) {
    if (!marshall_resize(mbuf, MIN_LENGTH)) {
      return NULL;
    }
  }
  unsigned char* p = mbuf->buffer;

  *(p++) = SYNC_BYTE;
  *(p++) = SYNC_BYTE;
  *(p) = packet_type;
  mbuf->curr_p = mbuf->buffer + 5;  // Leave two more for size
  mbuf->buffer_remaining = mbuf->buffer_length -
                            (mbuf->curr_p - mbuf->buffer);

  return mbuf;
}

/**
 * \fn int marshall_measurements(OmlMBuffer* mbuf, OmlMStream* ms, double      now)
 * \brief
 * \param mbuf
 * \param ms
 * \param now
 * \return 1 if successful
 */
int
marshall_measurements(
  OmlMBuffer* mbuf,
  OmlMStream* ms,
  double      now
) {
  OmlValueU v;

  mbuf = marshall_init(mbuf, OMB_DATA_P);

  // Make room for 'value_count'
  if (mbuf->buffer_remaining < 1) {
    marshall_resize(mbuf, 2 * mbuf->buffer_length);
  }
  *(mbuf->curr_p++) = (unsigned char)0;
  mbuf->buffer_remaining--;

  // index to database table
  *(mbuf->curr_p++) = (unsigned char)ms->index;
  mbuf->buffer_remaining--;
//  v.stringPtrValue = table_name;
//  marshall_value(mbuf, OML_STRING_PTR_VALUE, &v);

  v.longValue = ms->seq_no;
  marshall_value(mbuf, OML_LONG_VALUE, &v);

  v.doubleValue = now;
  marshall_value(mbuf, OML_DOUBLE_VALUE, &v);

  return marshall_finalize(mbuf);
}

/**
 * \fn int marshall_values(OmlMBuffer* mbuf, OmlValue* values, int value_count)
 * \brief Marshall the array of +values+ into +buffer+ and return the size of the buffer used. If the returned number is negative marshalling didn't finish as the provided buffer was short of the number of bytes returned (when multiplied by -1).
 * \param mbuf
 * \param values
 * \param value_count
 * \return 1 when finished
 */
int
marshall_values(
  OmlMBuffer*    mbuf,
  OmlValue*      values,       //! type of sample
  int            value_count   //! size of above array
) {
  OmlValue* val = values;
  int i;

  for (i = 0; i < value_count; i++, val++) {
    marshall_value(mbuf, val->type, &val->value);
  }
  *(mbuf->buffer + 5) += value_count; // store the number of values added
  return 1;
}

/**
 * \fn inline int marshall_value(OmlMBuffer* mbuf, OmlValueT val_type, OmlValueU* val)
 * \brief
 * \param mbuf
 * \param val_type
 * \param val
 * \return 1 if successful, 0 otherwise
 */
inline int
marshall_value(
  OmlMBuffer*    mbuf,
  OmlValueT      val_type,
  OmlValueU*     val
) {
  unsigned char* p = mbuf->curr_p;

  switch (val_type) {
    case OML_LONG_VALUE: {
      long v = val->longValue;
      //uint32_t uv = v >= 0 ? ((uint32_t)v + BIG_L) : (-1 * v);
      uint32_t uv = (uint32_t)v;
      uint32_t nv = htonl(uv);

	  p = marshall_check_resize (mbuf, 5);
	  mbuf->buffer_remaining -= 5;

      *(p++) = LONG_T;
      memcpy(p, &nv, 4);
      p += 4;
      break;
    }
    case OML_DOUBLE_VALUE: {
      char type = DOUBLE_T;
      double v = val->doubleValue;
      int exp;
      double mant = frexp(v, &exp);
      signed char nexp = (signed char)exp;
      if (nexp != exp) {
        o_log(O_LOG_ERROR, "Double number '%lf' is out of bounds\n", v);
        type = DOUBLE_NAN;
        nexp = 0;
      }
      int32_t imant = (int32_t)(mant * (1 << BIG_L));
      uint32_t nmant = htonl(imant);

	  p = marshall_check_resize (mbuf, 6);
	  mbuf->buffer_remaining -= 6;

      *(p++) = type;
      memcpy(p, &nmant, 4);
      p += 4;
      *(p++) = nexp;
      break;
    }
    case OML_STRING_VALUE: {
      char* str = val->stringValue.ptr;

	  if (str == NULL)
		{
		  str = "";
		  o_log (O_LOG_WARN, "Attempting to send a NULL string; sending empty string instead\n");
		}

      int len = strlen(str);
      if (len > 254) {
        o_log(O_LOG_ERROR, "Truncated string '%s'\n", str);
        len = 254;
      }

	  p = marshall_check_resize (mbuf, (len+2));
	  mbuf->buffer_remaining -= (len + 2);

      *(p++) = STRING_T;
      *(p++) = (char)(len & 0xff);
      for (; len > 0; len--, str++) *(p++) = *str;
      break;
    }
    default:
    o_log(O_LOG_ERROR, "Unsupported value type '%d'\n", val_type);
    return 0;
  }
  mbuf->curr_p = p;
  return 1;
}

/**
 * \fn int marshall_finalize(OmlMBuffer*  mbuf)
 * \brief
 * \param mbuf
 * \return 1 when finished
 */
int
marshall_finalize(
  OmlMBuffer*  mbuf
) {
  unsigned char* p = mbuf->buffer + 3;  // beginning of length block
  int len = mbuf->curr_p - mbuf->buffer - 5;  // 5 is the header
  //o_log(O_LOG_DEBUG2, "Message content  size '%d'\n", len);

  uint16_t nlen = htons(len);  // pure data length
  memcpy(p,&nlen,2);
  p += 2;
  return 1;
}

/**
 * \fn unsigned char* marshall_resize( OmlMBuffer*  mbuf, int new_size)
 * \brief
 * \param mbuf
 * \param new_size
 * \return the current ointer to the buffer
 */
unsigned char*
marshall_resize(
  OmlMBuffer*  mbuf,
  int new_size
) {
  if (mbuf->resize != NULL) {
    return mbuf->resize(mbuf, new_size);
  }
  if (new_size < MIN_LENGTH) new_size = MIN_LENGTH;
  unsigned char* newBuf = (unsigned char*)malloc(new_size);
  if (mbuf->buffer != NULL) {
    int offset = mbuf->curr_p - mbuf->buffer;
	int msg_offset = mbuf->message_start - mbuf->buffer;
    int len = offset + mbuf->buffer_fill;
    int i;
    unsigned char* from = mbuf->buffer;
    unsigned char* to = newBuf;
    mbuf->buffer_remaining = new_size - len;
    for (i = 0; i < len; len--) *(to++) = *(from++);
    mbuf->curr_p = newBuf + offset;
	mbuf->message_start = newBuf + msg_offset;
    free(mbuf->buffer);
  } else {
    mbuf->curr_p = newBuf;
	mbuf->message_start = newBuf;
    mbuf->buffer_remaining = new_size;
  }
  mbuf->buffer = newBuf;
  mbuf->buffer_length = new_size;

  return mbuf->curr_p;
}

/**
 *  @brief Check that the buffer has space for the next piece of data,
 *  and resize it if necessary.
 *
 *  Returns the current buffer insertion point.
 */
unsigned char*
marshall_check_resize (OmlMBuffer* mbuf, size_t bytes)
{
  if (mbuf->buffer_remaining <= 0 || (size_t)mbuf->buffer_remaining < bytes)
	{
	  return marshall_resize (mbuf, 2 * mbuf->buffer_length);
	}
  return mbuf->curr_p;
}

/**
 * \fn
 * \brief Reads the header information contained in the +buffer+ contained in +mbuf+. It stores the message type in +type_p+.
 * \param mbuf
 * \param type_p
 * \return It returns 1 if everything is fine. If the buffer is too short it returns the size of the missing section as a negative number.
 */
int
unmarshall_init(
  OmlMBuffer*  mbuf,
  OmlMsgType* type_p
) {
  unsigned char* p = mbuf->message_start = mbuf->curr_p;
  int remaining = mbuf->buffer_fill - (p - mbuf->buffer);

  if (remaining < 6) {
    return remaining - 6;
  }
  if (! (*(p++) == SYNC_BYTE && *(p++) == SYNC_BYTE)) {
    o_log(O_LOG_ERROR, "Out of sync. Don't know how to get back\n");
    return 0;
  }
  *type_p = (OmlMsgType)*(p++);
  uint16_t nv = 0;
  memcpy(&nv,p,2);
  p += 2;
  uint16_t hv = ntohs(nv);
  int len = (int)(hv);

  mbuf->curr_p = p;
  mbuf->buffer_remaining = len;

  // check if the full message is in the buffer
  int extra = mbuf->buffer_fill - ((p - mbuf->buffer) + len);
  if (extra < 0) {
	int start_pos = mbuf->message_start - mbuf->buffer;
	o_log (O_LOG_DEBUG, "Didn't get a full message, so unwinding the message buffer\n");
	o_log (O_LOG_DEBUG, "(Message starts at %d; message length %d; fill - start =  %d; %d bytes short)\n",
		   start_pos,
		   len,
		   mbuf->buffer_fill - (mbuf->message_start - mbuf->buffer),
		   -extra);
    mbuf->curr_p = mbuf->message_start;  // rewind
	return extra;  // need more input data
  }
  return 1;
}

/**
 * \fn int unmarshall_measurements(OmlMBuffer* mbuf, int* table_index, int* seq_no_p,  double* ts_p, OmlValue*   values, int         max_value_count)
 * \brief
 * \param mbuf
 * \param table_index
 * \param seq_no_p
 * \param ts_p
 * \param values measurement values
 * \param max_value_count max. length of above array
 * \return 1 if successful, 0 otherwise
 */
int
unmarshall_measurements(
  OmlMBuffer* mbuf,
  int*        table_index,
  int*        seq_no_p,
  double*     ts_p,
  OmlValue*   values,
  int         max_value_count
) {
  mbuf->curr_p++;           // That's where we keep the number of values
  mbuf->buffer_remaining--;

  *table_index = (int)*(mbuf->curr_p++);
  mbuf->buffer_remaining--;

  OmlValue v;
  if (unmarshall_value(mbuf, &v) != 1) {
    o_log(O_LOG_DEBUG, "Can't find seq_no in incoming measurement\n");
    return 0;
  }
  if (v.type != OML_LONG_VALUE) {
    o_log(O_LOG_DEBUG, "Expected an integer for seq_no, but got type '%d'\n", v.type);
    return 0;
  }
  *seq_no_p = (int)v.value.longValue;

  if (unmarshall_value(mbuf, &v) != 1) {
    o_log(O_LOG_DEBUG, "Can't find time_stamp in incoming measurement\n");
    return 0;
  }
  if (v.type != OML_DOUBLE_VALUE) {
    o_log(O_LOG_DEBUG, "Expected a double for time_stamp, but got type '%d'\n", v.type);
    return 0;
  }
  *ts_p = v.value.doubleValue;

  return unmarshall_values(mbuf, values, max_value_count);
}

/**
 * \fn int unmarshall_values(OmlMBuffer*  mbuffer, OmlValue*    values, int max_value_count)
 * \brief Unmarshalls the content of +buffer+ into an array of +values+ whose max. allocated size is +value_count+.
 * \param mbuffer
 * \param values type of sample
 * \param max_value_count max. length of above array
 * \return the number of values found. If the returned number is negative, there were more values in the buffer. The number (when multiplied by -1) indicates  by how much the +values+ array should be extended. If the number is less then -100, it indicates an error.
 */
int
unmarshall_values(
  OmlMBuffer*  mbuf,
  OmlValue*    values,
  int          max_value_count
) {
  int value_count = (int)*(mbuf->message_start + 5);

  if (value_count > max_value_count) {
	o_log (O_LOG_WARN, "Measurement packet contained %d too many values for internal storage (max %d, actual %d); skipping packet\n",
		   (value_count - max_value_count),
		   max_value_count,
		   value_count);
	uint16_t nv = 0;
	memcpy (&nv, mbuf->message_start + 3, 2);
	uint16_t hv = ntohs (nv);
	int msg_length = (int)hv;
	o_log (O_LOG_WARN, "Message length appears to be %d + 5\n", msg_length);

	mbuf->message_start += msg_length + 5;
	mbuf->curr_p = mbuf->message_start;

	if ((mbuf->curr_p - mbuf->buffer < mbuf->buffer_fill - 1) &&
		(mbuf->curr_p[0] != SYNC_BYTE || mbuf->curr_p[1] != SYNC_BYTE))
	  {
		o_log (O_LOG_WARN, "Tried to skip a packet but lost sync in the process.  Entering a parallel universe...\n");
	  }
    return max_value_count - value_count;  // value array is too small
  }

  int i;
  OmlValue* val = values;
   //o_log(O_LOG_DEBUG, "value to analyse'%d'\n", value_count);
  for (i = 0; i < value_count; i++, val++) {
    if (!unmarshall_value(mbuf, val)) {
	  o_log (O_LOG_WARN, "Some kind of ERROR in unmarshall_value() call\n");
      return -1;
    }
  }
  return value_count;
}

/**
 * \fn int unmarshall_value(OmlMBuffer*  mbuffer, OmlValue*    value)
 * \brief
 * \param mbuffer
 * \param value
 * \return 1 if successful, 0 otherwise
 */
int
unmarshall_value(
  OmlMBuffer*  mbuf,
  OmlValue*    value
) {
  unsigned char* p = mbuf->curr_p;
  int buf_remaining = mbuf->buffer_remaining;

  if ((buf_remaining --) < 0) {
    o_log(O_LOG_ERROR, "Buffer is too short for TYPE.\n");
    return 0;
  }
  char type = *p++;
  switch (type) {
    case LONG_T: {
      if ((buf_remaining -= 4) < 0) {
        o_log(O_LOG_ERROR, "Buffer is too short for LONG.\n");
        return 0;
      }
      uint32_t nv = 0;
      memcpy(&nv,p,4);
      p += 4;
      uint32_t hv = ntohl(nv);
      //long v = (long)(hv - BIG_L);
      long v = (long)(hv);
      value->value.longValue = v;
      value->type = OML_LONG_VALUE;
      //o_log(O_LOG_DEBUG, "Unmarshalling long %ld.\n", v);
      break;
    }
    case DOUBLE_T: {
      if ((buf_remaining -= 5) < 0) {
        o_log(O_LOG_ERROR, "Buffer is too short for DOUBLE.\n");
        return -102;
      }
      uint32_t nmant = 0;
      memcpy(&nmant,p,4);
      p += 4;
      int hmant = (int)ntohl(nmant);
      double mant = hmant * 1.0 / (1 << BIG_L);
      int exp = (signed char)*p++;
      double v = ldexp(mant, exp);
      value->value.doubleValue = v;
      value->type = OML_DOUBLE_VALUE;
      //o_log(O_LOG_DEBUG, "Unmarshalling float %lf.\n", v);
      break;
    }
    case STRING_T: {
      if ((buf_remaining -= 1) < 0) {
        o_log(O_LOG_ERROR, "Buffer is too short for STRING (len).\n");
        return 0;
      }
      int len = (int)(*p++);
      if (value->type != OML_STRING_VALUE) {
        value->value.stringValue.size = 0;
        value->type = OML_STRING_VALUE;
      }
      if ((buf_remaining -= len) < 0) {
        o_log(O_LOG_ERROR, "Buffer is too short for STRING (text).\n");
        return 0;
      }
      if (len >= value->value.stringValue.size) {
        if (value->value.stringValue.size > 0) {
          free(value->value.stringValue.ptr);
        }
        int mlen = (len < MIN_LENGTH) ? MIN_LENGTH : len + 1;

        value->value.stringValue.ptr = (char*)malloc(mlen);
        value->value.stringValue.size = mlen - 1;
        value->value.stringValue.length = mlen;

      }
      strncpy(value->value.stringValue.ptr, (char*)p, len);
      *(value->value.stringValue.ptr + len) = '\0';
      p += len;
      break;
    }
    default:
      o_log(O_LOG_ERROR, "Unsupported value type '%d'\n", type);
      return 0;
  }
  mbuf->curr_p = p;
  mbuf->buffer_remaining = buf_remaining;

  return 1;
}



///**
// * \fn int oml_marshall_test()
// * \brief Test function of the marshalling library
// * \return 1 when finished
// */
//int
//oml_marshall_test()

//{
//  OmlValue in_values[5];
//
//  OmlValue* v1 = &in_values[0];
//  v1->type = OML_LONG_VALUE;
//  v1->value.longValue = -123456789;
//
//  OmlValue* v2 = &in_values[1];
//  v2->type = OML_DOUBLE_VALUE;
//  v2->value.doubleValue = -12.34567890e23;
//
//  OmlValue* v3 = &in_values[2];
//  v3->type = OML_STRING_PTR_VALUE;
//  v3->value.stringPtrValue = "hello world";
//
//  OmlMBuffer buf;
//  OmlValue in;
//  OmlValue out;
//
//  in.type = OML_LONG_VALUE;
//  in.value.longValue = -123456789;
//  memset((void*)&buf, 0, sizeof(OmlMBuffer));
//  buf.curr_p = buf.buffer;
//  marshall_value(&buf, in.type, &in.value);
//  memset((void*)&out, 0, sizeof(OmlValue));
//  buf.curr_p = buf.buffer;
//  int res = unmarshall_value(&buf, &out);
//  if (out.type != in.type || in.value.longValue != out.value.longValue) {
//    printf("Failed marshall long value %d\n", in.value.longValue);
//  }
//  in.value.longValue = 123456789;
//  memset((void*)&buf, 0, sizeof(OmlMBuffer));
//  buf.curr_p = buf.buffer;
//  marshall_value(&buf, in.type, &in.value);
//  memset((void*)&out, 0, sizeof(OmlValue));
//  buf.curr_p = buf.buffer;
//  res = unmarshall_value(&buf, &out);
//  if (out.type != in.type || in.value.longValue != out.value.longValue) {
//    printf("Failed marshall long value %d\n", in.value.longValue);
//  }
//
//  // DOUBLE
//  in.type = OML_DOUBLE_VALUE;
//  in.value.doubleValue = -0.12345e12;
//  memset((void*)&buf, 0, sizeof(OmlMBuffer));
//  buf.curr_p = buf.buffer;
//  marshall_value(&buf, in.type, &in.value);
//  memset((void*)&out, 0, sizeof(OmlValue));
//  buf.curr_p = buf.buffer;
//  res = unmarshall_value(&buf, &out);
//  double err = (in.value.doubleValue - out.value.doubleValue) /  out.value.doubleValue;
//  if (out.type != in.type || !(err < 0.0001 && err > -0.0001)) {
//    printf("Failed marshall double value %f\n", in.value.doubleValue);
//  }
//
//  in.value.doubleValue = 1.0e-34;
//  memset((void*)&buf, 0, sizeof(OmlMBuffer));
//  buf.curr_p = buf.buffer;
//  marshall_value(&buf, in.type, &in.value);
//  memset((void*)&out, 0, sizeof(OmlValue));
//  buf.curr_p = buf.buffer;
//  res = unmarshall_value(&buf, &out);
//  err = (in.value.doubleValue - out.value.doubleValue) /  out.value.doubleValue;
//  if (out.type != in.type || !(err < 0.0001 && err > -0.0001)) {
//    printf("Failed marshall double value %f\n", in.value.doubleValue);
//  }
//
//  in.value.doubleValue = 1.2345;
//  memset((void*)&buf, 0, sizeof(OmlMBuffer));
//  buf.curr_p = buf.buffer;
//  marshall_value(&buf, in.type, &in.value);
//  memset((void*)&out, 0, sizeof(OmlValue));
//  buf.curr_p = buf.buffer;
//  res = unmarshall_value(&buf, &out);
//  err = (in.value.doubleValue - out.value.doubleValue) /  out.value.doubleValue;
//  if (out.type != in.type || !(err < 0.0001 && err > -0.0001)) {
//    printf("Failed marshall double value %f\n", in.value.doubleValue);
//  }
//
//  //  int res1 = marshall_values(&buf, in_values, 3);
////  o_log(O_LOG_INFO, "Result: %d\n", res1);
////
////  OmlValue out_values[5];
////  buf.curr_p = buf.message_start = buf.buffer; buf.buffer_remaining = buf.buffer_length;
////  int res2 = unmarshall_values(&buf, out_values, 5);
////  o_log(O_LOG_INFO, "Result: %d\n", res2);
//
//  buf.curr_p = buf.buffer; buf.buffer_remaining = buf.buffer_length;
//  OmlMStream ms;
//  OmlMP mp;
//  ms.mp = &mp;
//  ms.index = 1; ms.seq_no = 99; mp.param_count = 3;
//  marshall_measurements(&buf, &ms, 2.3);
//  int res3 = marshall_values(&buf, in_values, 3);
//  marshall_finalize(&buf);
//  o_log(O_LOG_INFO, "Result: %d\n", res3);
//
//  buf.buffer_fill = (buf.curr_p - buf.buffer);
//  buf.curr_p = buf.buffer; buf.buffer_remaining = buf.buffer_length;
//
//  OmlMsgType type;
//  unmarshall_init(&buf, &type);
//
//  OmlValue out_values2[5];
//  int      index;
//  int      seq_no;
//  double   ts;
//  int res4 = unmarshall_measurements(&buf, &index, &seq_no, &ts, out_values2, 5);
//  o_log(O_LOG_INFO, "Result: %d\n", res4);
//
//
//
//  return 1;
//}
//
///**
// * \fn
// * \brief
// * \param
// * \return
// */
//#ifdef MARSHALL_TEST
//void
//main(
//  char** argv,
//  int argc
//) {
//  oml_marshall_test();
//}
//#endif
//  OmlValue out_values[5];
//  buf.curr_p = buf.message_start = buf.buffer; buf.buffer_remaining = buf.buffer_length;
//  int res2 = unmarshall_values(&buf, out_values, 5);
//  o_log(O_LOG_INFO, "Result: %d\n", res2);

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
