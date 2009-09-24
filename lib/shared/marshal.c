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
/*!\file marshal.c
  \brief Implements marhsalling and unmarshalling of basic types for transmission across the network.

*/

#include <ocomm/o_log.h>
#include <math.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "marshal.h"
#include "mbuf.h"

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

#define BIG_S 15
#define BIG_L 30

#define LONG_T   0x1
#define DOUBLE_T 0x2
#define DOUBLE_NAN 0x3
#define STRING_T 0x4

#define SYNC_BYTE 0xAA

#define PACKET_HEADER_SIZE 5
#define STREAM_HEADER_SIZE 2

#define LONG_T_SIZE       4
#define DOUBLE_T_SIZE     5
#define STRING_T_MAX_SIZE 254

#define MAX_STRING_LENGTH STRING_T_MAX_SIZE

#define MIN_LENGTH 64

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
 * \brief
 * \param mbuf
 * \param packet_type
 * \return
 */
MBuffer*
marshal_init(MBuffer*  mbuf, OmlMsgType  packet_type)
{
  uint8_t buf[PACKET_HEADER_SIZE] = { SYNC_BYTE, SYNC_BYTE, packet_type, 0, 0 };
  int result;

  if (mbuf == NULL)
	return NULL;

  result = mbuf_begin_write(mbuf);
  if (result == -1)
	{
	  o_log (O_LOG_ERROR, "Couldn't start marshalling packet (mbuf_begin_write())\n");
	  return NULL;
	}

  result = mbuf_write (mbuf, buf, LENGTH (buf));
  if (result == -1)
	{
	  o_log (O_LOG_ERROR, "Error when trying to marshal packet header (mbuf_write())\n");
	  return NULL;
	}

  return mbuf;
}

/**
 * \brief
 * \param mbuf
 * \param ms
 * \param now
 * \return 1 if successful
 */
int
marshal_measurements(MBuffer* mbuf, int stream, int seqno, double now)
{
  OmlValueU v;
  uint8_t s[2] = { 0, (uint8_t)stream };
  mbuf = marshal_init(mbuf, OMB_DATA_P);
  int result = mbuf_write (mbuf, s, LENGTH (s));

  if (result == -1)
	{
	  o_log (O_LOG_ERROR, "Unable to marshal table number and measurement count (mbuf_write())\n");
	  mbuf_reset_write (mbuf);
	  return -1;
	}

  v.longValue = seqno;
  marshal_value(mbuf, OML_LONG_VALUE, &v);

  v.doubleValue = now;
  marshal_value(mbuf, OML_DOUBLE_VALUE, &v);

  return marshal_finalize(mbuf);
}

/**
 * \brief Marshal the array of +values+ into +buffer+ and return the size of the buffer used. If the returned number is negative marshalling didn't finish as the provided buffer was short of the number of bytes returned (when multiplied by -1).
 * \param mbuf
 * \param values
 * \param value_count
 * \return 1 when finished
 */
int
marshal_values(MBuffer* mbuf, OmlValue* values, int value_count)
{
  OmlValue* val = values;
  int i;

  for (i = 0; i < value_count; i++, val++) {
    marshal_value(mbuf, val->type, &val->value);
  }

  uint8_t* buf = mbuf_message (mbuf);
  buf[5] += value_count;
  return 1;
}

/**
 * \brief
 * \param mbuf
 * \param val_type
 * \param val
 * \return 1 if successful, 0 otherwise
 */
inline int
marshal_value(MBuffer* mbuf, OmlValueT val_type, OmlValueU* val)
{
  switch (val_type) {
  case OML_LONG_VALUE: {
	long v = val->longValue;
	uint32_t uv = (uint32_t)v;
	uint32_t nv = htonl(uv);
	uint8_t buf[5];

	buf[0] = LONG_T;
	memcpy(&buf[1], &nv, sizeof (nv));

	int result = mbuf_write (mbuf, buf, LENGTH (buf));

	if (result == -1)
	  {
		o_log (O_LOG_ERROR, "Failed to marshal OML_LONG_VALUE (mbuf_write())\n");
		mbuf_reset_write (mbuf);
		return 0;
	  }
	break;
  }
  case OML_DOUBLE_VALUE: {
	uint8_t type = DOUBLE_T;
	double v = val->doubleValue;
	int exp;
	double mant = frexp(v, &exp);
	int8_t nexp = (int8_t)exp;
	if (nexp != exp) {
	  o_log(O_LOG_ERROR, "Double number '%lf' is out of bounds\n", v);
	  type = DOUBLE_NAN;
	  nexp = 0;
   }
   int32_t imant = (int32_t)(mant * (1 << BIG_L));
   uint32_t nmant = htonl(imant);

   uint8_t buf[6] = { type, 0, 0, 0, 0, nexp };

   memcpy(&buf[1], &nmant, sizeof (nmant));

   int result = mbuf_write (mbuf, buf, LENGTH (buf));

   if (result == -1)
	 {
	   o_log (O_LOG_ERROR, "Failed to marshal OML_DOUBLE_VALUE (mbuf_write())\n");
	   mbuf_reset_write (mbuf);
	   return 0;
	 }
   break;
 }
 case OML_STRING_VALUE: {
   char* str = val->stringValue.ptr;

   if (str == NULL)
	 {
	   str = "";
	   o_log (O_LOG_WARN, "Attempting to send a NULL string; sending empty string instead\n");
	 }

   size_t len = strlen(str);
   if (len > STRING_T_MAX_SIZE) {
	 o_log(O_LOG_ERROR, "Truncated string '%s'\n", str);
	 len = STRING_T_MAX_SIZE;
   }

   uint8_t buf[2] = { STRING_T, (uint8_t)(len & 0xff) };
   int result = mbuf_write (mbuf, buf, LENGTH (buf));

   if (result == -1)
	 {
	   o_log (O_LOG_ERROR, "Failed to marshal OML_STRING_VALUE type and length (mbuf_write())\n");
	   mbuf_reset_write (mbuf);
	   return 0;
	 }

   result = mbuf_write (mbuf, (uint8_t*)str, len);

   if (result == -1)
	 {
	   o_log (O_LOG_ERROR, "Failed to marshal OML_STRING_VALUE (mbuf_write())\n");
	   mbuf_reset_write (mbuf);
	   return 0;
	 }
   break;
 }
 default:
   o_log(O_LOG_ERROR, "Unsupported value type '%d'\n", val_type);
   return 0;
 }

  return 1;
}

/**
 * \brief
 * \param mbuf
 * \return 1 when finished
 */
int
marshal_finalize(MBuffer*  mbuf)
{
  uint8_t* buf = mbuf_message (mbuf);

  size_t len = mbuf_message_length (mbuf);

  if (len > UINT16_MAX)
	{
	  o_log (O_LOG_WARN, "Message length %d longer than maximum packet length (%d); packet will be truncated\n",
			 len, UINT16_MAX);
	  len = UINT16_MAX;
	}
  len -= 5;  // 5 is the length of the header... FIXME:  MAGIC NUMBER!
  uint16_t nlen = htons(len);  // pure data length

  // Store the length of the packet
  memcpy(&buf[3], &nlen, sizeof (nlen));

  return 1;
}

/**
 * \brief Reads the header information contained in the +buffer+ contained in +mbuf+. It stores the message type in +type_p+.
 * \param mbuf
 * \param type_p
 * \return It returns 1 if everything is fine. If the buffer is too short it returns the size of the missing section as a negative number.
 */
int
unmarshal_init(MBuffer* mbuf, OmlBinaryHeader* header)
{
  uint8_t header_str[PACKET_HEADER_SIZE];
  uint8_t stream_header_str[STREAM_HEADER_SIZE];

  // Assumption:  msgptr == rdptr at this point
  int result = mbuf_read (mbuf, header_str, LENGTH (header_str));

  if (result == -1)
	return mbuf_remaining (mbuf) - PACKET_HEADER_SIZE;

  o_log (O_LOG_DEBUG, "HEADER: %s\n", to_octets (header_str, LENGTH (header_str)));

  if (! (header_str[0] == SYNC_BYTE && header_str[1] == SYNC_BYTE))
	{
	  o_log(O_LOG_ERROR, "Out of sync. Don't know how to get back\n");
	  return 0;
	}

  header->type = (OmlMsgType)header_str[2];
  uint16_t nv = 0;
  memcpy (&nv, &header_str[3], 2);
  uint16_t hv = ntohs (nv);
  header->length = (int)hv;

  int extra = mbuf_remaining (mbuf) - header->length;
  if (extra < 0)
	{
	  o_log (O_LOG_DEBUG, "Didn't get a full message (%d octets short), so unwinding the message buffer\n", -extra);
	  mbuf_reset_read (mbuf);
	  return extra;
	}

  result = mbuf_read (mbuf, stream_header_str, LENGTH (stream_header_str));
  if (result == -1)
	{
	  o_log (O_LOG_ERROR, "Unable to read stream header\n");
	  return 0;
	}

  header->values = (int)stream_header_str[0];
  header->stream = (int)stream_header_str[1];

  OmlValue seqno;
  OmlValue timestamp;

  if (unmarshal_typed_value (mbuf, "seq-no", OML_LONG_VALUE, &seqno) == -1)
	return 0;

  if (unmarshal_typed_value (mbuf, "timestamp", OML_DOUBLE_VALUE, &timestamp) == -1)
	return 0;

  header->seqno = seqno.value.longValue;
  header->timestamp = timestamp.value.doubleValue;

  return 1;
}

/**
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
unmarshal_measurements(
  MBuffer* mbuf,
  OmlBinaryHeader* header,
  OmlValue*   values,
  int         max_value_count
) {
  return unmarshal_values(mbuf, header, values, max_value_count);
}

/**
 * \brief Unmarshals the content of +buffer+ into an array of +values+ whose max. allocated size is +value_count+.
 * \param mbuffer
 * \param values type of sample
 * \param max_value_count max. length of above array
 * \return the number of values found. If the returned number is negative, there were more values in the buffer. The number (when multiplied by -1) indicates  by how much the +values+ array should be extended. If the number is less then -100, it indicates an error.
 */
int
unmarshal_values(
  MBuffer*  mbuf,
  OmlBinaryHeader* header,
  OmlValue*    values,
  int          max_value_count
) {
  int value_count = header->values;

  if (value_count > max_value_count) {
	o_log (O_LOG_WARN, "Measurement packet contained %d too many values for internal storage (max %d, actual %d); skipping packet\n",
		   (value_count - max_value_count),
		   max_value_count,
		   value_count);
	o_log (O_LOG_WARN, "Message length appears to be %d + 5\n", header->length);

	mbuf_read_skip (mbuf, header->length + PACKET_HEADER_SIZE);
	mbuf_begin_read (mbuf);

	// FIXME:  Check for sync
    return max_value_count - value_count;  // value array is too small
  }

  int i;
  OmlValue* val = values;
   //o_log(O_LOG_DEBUG, "value to analyse'%d'\n", value_count);
  for (i = 0; i < value_count; i++, val++) {
    if (unmarshal_value(mbuf, val) == 0) {
	  o_log (O_LOG_WARN, "Some kind of ERROR in unmarshal_value() call\n");
      return -1;
    }
  }
  return value_count;
}

/**
 * \brief
 * \param mbuffer
 * \param value
 * \return 1 if successful, 0 otherwise
 */
int
unmarshal_value(
  MBuffer*  mbuf,
  OmlValue*    value
) {
  if (mbuf_remaining(mbuf) == 0)
	{
	  o_log(O_LOG_ERROR, "Tried to unmarshal a value from the buffer, but didn't receive enough data to do that\n");
	  return 0;
	}

  int type = mbuf_read_byte (mbuf);
  if (type == -1) return 0;

  switch (type) {
    case LONG_T: {
	  uint8_t buf [LONG_T_SIZE];

	  if (mbuf_read (mbuf, buf, LENGTH (buf)) == -1)
		{
		  o_log(O_LOG_ERROR, "Failed to unmarshal OML_LONG_VALUE; not enough data?\n");
		  return 0;
		}

      uint32_t hv = ntohl(*((uint32_t*)buf));
      long v = (long)(hv);
      value->value.longValue = v;
      value->type = OML_LONG_VALUE;
      //o_log(O_LOG_DEBUG, "Unmarshalling long %ld.\n", v);
      break;
    }
    case DOUBLE_T: {
	  uint8_t buf [DOUBLE_T_SIZE];

	  if (mbuf_read (mbuf, buf, LENGTH (buf)) == -1)
		{
		  o_log(O_LOG_ERROR, "Failed to unmarshal OML_DOUBLE_VALUE; not enough data?\n");
		  return 0;
		}

      int hmant = (int)ntohl(*((uint32_t*)buf));
      double mant = hmant * 1.0 / (1 << BIG_L);
      int exp = (int8_t) buf[4];
      double v = ldexp(mant, exp);
      value->value.doubleValue = v;
      value->type = OML_DOUBLE_VALUE;
      //o_log(O_LOG_DEBUG, "Unmarshalling float %lf.\n", v);
      break;
    }
    case STRING_T: {
	  int len = 0;
	  uint8_t buf [STRING_T_MAX_SIZE];

	  len = mbuf_read_byte (mbuf);

	  if (len == -1 || mbuf_read (mbuf, buf, len) == -1)
		{
		  o_log(O_LOG_ERROR, "Failed to unmarshal OML_STRING_VALUE; not enough data?\n");
		  return 0;
		}

	  // FIXME:  All this fiddling with the string internals should be behind an API.
      if (value->type != OML_STRING_VALUE) {
        value->value.stringValue.size = 0;
        value->type = OML_STRING_VALUE;
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
      strncpy(value->value.stringValue.ptr, (char*)buf, len);
      *(value->value.stringValue.ptr + len) = '\0';
      break;
    }
    default:
      o_log(O_LOG_ERROR, "Unsupported value type '%d'\n", type);
      return 0;
  }

  return 1;
}

int
unmarshal_typed_value (MBuffer* mbuf, const char* name, OmlValueT type, OmlValue* value)
{
  if (unmarshal_value (mbuf, value) != 1)
	{
	  o_log (O_LOG_ERROR, "Error reading %s from binary packet\n", name);
	  return -1;
	}

  if (value->type != type)
	{
	  o_log (O_LOG_ERROR, "Expected type %d for %s, but got type '%d' instead\n",
			 type, name, value->type);
	  return -1;
	}
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
