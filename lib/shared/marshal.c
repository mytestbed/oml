/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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

#include "log.h"
#include <math.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <oml2/omlc.h>

#include "marshal.h"
#include "mbuf.h"
#include "htonll.h"
#include "oml_value.h"

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

#define BIG_S 15
#define BIG_L 30

#define LONG_T   0x1
#define DOUBLE_T 0x2
#define DOUBLE_NAN 0x3
#define STRING_T 0x4
#define INT32_T  0x5
#define UINT32_T 0x6
#define INT64_T  0x7
#define UINT64_T 0x8
#define BLOB_T   0x9

#define SYNC_BYTE 0xAA

#define PACKET_HEADER_SIZE 5
#define STREAM_HEADER_SIZE 2

#define LONG_T_SIZE       4
#define DOUBLE_T_SIZE     5
#define STRING_T_MAX_SIZE 254
#define INT32_T_SIZE      4
#define UINT32_T_SIZE     4
#define INT64_T_SIZE      8
#define UINT64_T_SIZE     8
#define BLOB_T_MAX_SIZE   UINT32_MAX

#define MAX_STRING_LENGTH STRING_T_MAX_SIZE

#define MIN_LENGTH 64

/*
 * Map from OML_*_VALUE types to protocol types.
 *
 * NOTE:  This array must be ordered identically to the OmlValueT enum.
 */
static const int oml_type_map [] =
  {
    DOUBLE_T,
    LONG_T,
    -1,
    STRING_T,
    INT32_T,
    UINT32_T,
    INT64_T,
    UINT64_T,
    BLOB_T
  };

/*
 * Map from protocol types to OML_*_VALUE types.
 *
 * NOTE: This array must be ordered identically to the values of the protocol types.
 */
static const size_t protocol_type_map [] =
  {
    OML_UNKNOWN_VALUE,
    OML_LONG_VALUE,
    OML_DOUBLE_VALUE, // DOUBLE_T
    OML_DOUBLE_VALUE, // DOUBLE_NAN
    OML_STRING_VALUE,
    OML_INT32_VALUE,
    OML_UINT32_VALUE,
    OML_INT64_VALUE,
    OML_UINT64_VALUE,
    OML_BLOB_VALUE
  };

/*
 * Map from protocol types to protocol sizes.
 *
 * NOTE:  This array must be ordered identically to the values of the protocol types.
 */
static const size_t protocol_size_map [] =
  {
    -1,
    LONG_T_SIZE,
    DOUBLE_T_SIZE,
    DOUBLE_T_SIZE,
    STRING_T_MAX_SIZE,
    INT32_T_SIZE,
    UINT32_T_SIZE,
    INT64_T_SIZE,
    UINT64_T_SIZE,
    BLOB_T_MAX_SIZE
  };

/*
 * Map from OML_*_VALUE types to size of protocol types on the wire.
 *
 * NOTE:  This array must be ordered identically to the OmlValueT enum.
 */
static const size_t oml_size_map [] =
  {
    DOUBLE_T_SIZE,
    LONG_T_SIZE,
    -1,
    STRING_T_MAX_SIZE,
    INT32_T_SIZE,
    UINT32_T_SIZE,
    INT64_T_SIZE,
    UINT64_T_SIZE,
    BLOB_T_MAX_SIZE
  };

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

static int marshal_header_short (MBuffer *mbuf)
{
  uint8_t buf[] = {
    SYNC_BYTE, SYNC_BYTE, OMB_DATA_P, 0, 0
  };
  return mbuf_write (mbuf, buf, LENGTH (buf));
}

static int marshal_header_long (MBuffer *mbuf)
{
  uint8_t buf[] = {
    SYNC_BYTE, SYNC_BYTE, OMB_LDATA_P, 0, 0, 0, 0
  };
  return mbuf_write (mbuf, buf, LENGTH (buf));
}

OmlBinMsgType
marshal_get_msgtype (MBuffer *mbuf)
{
  return (OmlBinMsgType)(mbuf_message (mbuf))[2];
}

/**
 * @brief Initialize the buffer to serialize a new measurement packet,
 * starting at the current write pointer.
 *
 * @param mbuf The buffer to serialize into.
 * @param msgtype the type of packet to build.
 *
 * @return 0 on success, -1 on failure.  This function can fail if
 * there is a memory allocation failure or if the buffer is
 * misconfigured.
 */
int
marshal_init(MBuffer *mbuf, OmlBinMsgType msgtype)
{
  int result;
  if (mbuf == NULL) return -1;

  result = mbuf_begin_write (mbuf);
  if (result == -1) {
    logerror("Couldn't start marshalling packet (mbuf_begin_write())\n");
    return -1;
  }

  switch (msgtype) {
  case OMB_DATA_P:  result = marshal_header_short (mbuf); break;
  case OMB_LDATA_P: result = marshal_header_long (mbuf); break;
  }

  if (result == -1) {
    logerror("Error when trying to marshal packet header\n");
    return -1;
  }

  return 0;
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
  int result = mbuf_write (mbuf, s, LENGTH (s));

  if (result == -1) {
    logerror("Unable to marshal table number and measurement count (mbuf_write())\n");
    mbuf_reset_write (mbuf);
    return -1;
  }

  v.longValue = seqno;
  marshal_value(mbuf, OML_LONG_VALUE, &v);

  v.doubleValue = now;
  marshal_value(mbuf, OML_DOUBLE_VALUE, &v);

  return 1;
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
  OmlBinMsgType type = marshal_get_msgtype (mbuf);
  switch (type) {
  case OMB_DATA_P: buf[5] += value_count; break;
  case OMB_LDATA_P: buf[7] += value_count; break;
  }
  return 1;
}

inline int
marshal_value(MBuffer* mbuf, OmlValueT val_type, OmlValueU* val)
{
  switch (val_type) {
    /* Treat OML_LONG_VALUE separately because size differs between 32-bit/64-bit */
  case OML_LONG_VALUE: {
    long v = oml_value_clamp_long (val->longValue);
    uint32_t uv = (uint32_t)v;
    uint32_t nv = htonl(uv);
    uint8_t buf[LONG_T_SIZE+1];

    buf[0] = LONG_T;
    memcpy(&buf[1], &nv, sizeof (nv));

    int result = mbuf_write (mbuf, buf, LENGTH (buf));
    if (result == -1)
      {
        logerror("Failed to marshal OML_LONG_VALUE (mbuf_write())\n");
        mbuf_reset_write (mbuf);
        return 0;
      }
    break;
  }
  case OML_INT32_VALUE:
  case OML_UINT32_VALUE:
  case OML_INT64_VALUE:
  case OML_UINT64_VALUE: {
    uint8_t buf[UINT64_T_SIZE+1]; // Max integer size
    uint32_t uv32;
    uint32_t nv32;
    uint64_t uv64;
    uint64_t nv64;
    uint8_t *p_nv;

    if (oml_size_map[val_type] == 4)
      {
        uv32 = val->uint32Value;
        nv32 = htonl(uv32);
        p_nv = (uint8_t*)&nv32;
      }
    else
      {
        uv64 = val->uint64Value;
        nv64 = htonll(uv64);
        p_nv = (uint8_t*)&nv64;
      }

    buf[0] = oml_type_map[val_type];
    memcpy(&buf[1], p_nv, oml_size_map[val_type]);

    int result = mbuf_write (mbuf, buf, oml_size_map[val_type] + 1);
    if (result == -1)
      {
        logerror("Failed to marshal %s value (mbuf_write())\n",
               oml_type_to_s (val_type));
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
      logerror("Double number '%lf' is out of bounds\n", v);
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
       logerror("Failed to marshal OML_DOUBLE_VALUE (mbuf_write())\n");
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
       logwarn("Attempting to send a NULL string; sending empty string instead\n");
     }

   size_t len = strlen(str);
   if (len > STRING_T_MAX_SIZE) {
     logerror("Truncated string '%s'\n", str);
     len = STRING_T_MAX_SIZE;
   }

   uint8_t buf[2] = { STRING_T, (uint8_t)(len & 0xff) };
   int result = mbuf_write (mbuf, buf, LENGTH (buf));

   if (result == -1)
     {
       logerror("Failed to marshal OML_STRING_VALUE type and length (mbuf_write())\n");
       mbuf_reset_write (mbuf);
       return 0;
     }

   result = mbuf_write (mbuf, (uint8_t*)str, len);

   if (result == -1)
     {
       logerror("Failed to marshal OML_STRING_VALUE (mbuf_write())\n");
       mbuf_reset_write (mbuf);
       return 0;
     }
   break;
 }
 case OML_BLOB_VALUE: {
   int result = 0;
   void *blob = val->blobValue.data;
   size_t length = val->blobValue.fill;
   if (blob == NULL || length == 0) {
     logwarn ("Attempting to send NULL or empty blob; blob of length 0 will be sent\n");
     length = 0;
   }

   uint8_t buf[5] = { BLOB_T, 0, 0, 0, 0 };
   size_t n_length = htonl (length);
   memcpy (&buf[1], &n_length, 4);

   result = mbuf_write (mbuf, buf, sizeof (buf));

   if (result == -1) {
     logerror ("Failed to marshall OML_BLOB_VALUE type and length (mbuf_write())\n");
     mbuf_reset_write (mbuf);
     return 0;
   }

   result = mbuf_write (mbuf, blob, length);

   if (result == -1) {
     logerror ("Failed to marshall %d bytes of OML_BLOB_VALUE data\n", length);
     mbuf_reset_write (mbuf);
     return 0;
   }
   break;
 }
 default:
   logerror("Unsupported value type '%d'\n", val_type);
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
  OmlBinMsgType type = marshal_get_msgtype (mbuf);
  size_t len = mbuf_message_length (mbuf);

  if (len > UINT32_MAX) {
    logwarn("Message length %d longer than maximum packet length (%d); "
            "packet will be truncated\n",
            len, UINT32_MAX);
    len = UINT32_MAX;
  }


  if (type == OMB_DATA_P && len > UINT16_MAX) {
    /*
     * We assumed a short packet, but there is too much data, so we
     * have to shift the whole buffer down by 2 bytes and convert to a
     * long packet.
     */
    uint8_t s[2] = {0};
    /* Put some padding in the buffer to make sure it has room, and maintains its invariants */
    mbuf_write (mbuf, s, sizeof (s));
    memmove (&buf[PACKET_HEADER_SIZE+2], &buf[PACKET_HEADER_SIZE],
             len - PACKET_HEADER_SIZE);
    len += 2;
    buf[2] = type = OMB_LDATA_P;
  }


  switch (type) {
  case OMB_DATA_P: {
    len -= PACKET_HEADER_SIZE; // Data length minus header
    uint16_t nlen = htons (len);
    memcpy (&buf[3], &nlen, sizeof (nlen));
    break;
  }
  case OMB_LDATA_P: {
    len -= PACKET_HEADER_SIZE + 2; // Data length minus header
    uint32_t nlen = htonl (len); // pure data length
    memcpy (&buf[3], &nlen, sizeof (nlen));
    break;
  }
  }

  return 1;
}

/** Read the header information contained in an MBuf.
 *
 * \param mbuf MBuf to read from
 * \param header reference to an OmlBinaryHeader into which the data from the mbuf should be unmasrhalled
 * \return 1 if everything is fine, the size of the missing section as a negative number if the buffer is too short or 0 if something failed
 */
int
unmarshal_init(MBuffer* mbuf, OmlBinaryHeader* header)
{
  uint8_t header_str[PACKET_HEADER_SIZE + 2];
  uint8_t stream_header_str[STREAM_HEADER_SIZE];

  // Assumption:  msgptr == rdptr at this point
  int result = mbuf_read (mbuf, header_str, 3);

  if (result == -1)
    return mbuf_remaining (mbuf) - 3;

  if (! (header_str[0] == SYNC_BYTE && header_str[1] == SYNC_BYTE)) {
    logerror("Out of sync.\n");
    return 0;
  }

  header->type = (OmlBinMsgType)header_str[2];

  if (header->type == OMB_DATA_P) {
    // Read 2 more bytes of the length field
    uint16_t nv = 0;
    result = mbuf_read (mbuf, (uint8_t*)&nv, sizeof (uint16_t));
    if (result == -1) {
      int n = mbuf_remaining (mbuf) - 2;
      mbuf_reset_read (mbuf);
      return n;
    }
    header->length = (int)ntohs (nv);
  } else if (header->type == OMB_LDATA_P) {
    // Read 4 more bytes of the length field
    uint32_t nv = 0;
    result = mbuf_read (mbuf, (uint8_t*)&nv, sizeof (uint32_t));
    if (result == -1) {
      int n = mbuf_remaining (mbuf) - 4;
      mbuf_reset_read (mbuf);
      return n;
    }
    header->length = (int)ntohl (nv);
  } else {
    logwarn ("Unknown packet type %d\n", (int)header->type);
    return -1;
  }

  int extra = mbuf_remaining (mbuf) - header->length;
  if (extra < 0) {
    mbuf_reset_read (mbuf);
    return extra;
  }

  result = mbuf_read (mbuf, stream_header_str, LENGTH (stream_header_str));
  if (result == -1)
    {
      logerror("Unable to read stream header\n");
      return 0;
    }

  header->values = (int)stream_header_str[0];
  header->stream = (int)stream_header_str[1];

  OmlValue seqno;
  OmlValue timestamp;

  if (unmarshal_typed_value (mbuf, "seq-no", OML_INT32_VALUE, &seqno) == -1)
    return 0;

  if (unmarshal_typed_value (mbuf, "timestamp", OML_DOUBLE_VALUE, &timestamp) == -1)
    return 0;

  header->seqno = seqno.value.int32Value;
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
    logwarn("Measurement packet contained %d too many values for internal storage (max %d, actual %d); skipping packet\n",
           (value_count - max_value_count),
           max_value_count,
           value_count);
    logwarn("Message length appears to be %d + 5\n", header->length);

    mbuf_read_skip (mbuf, header->length + PACKET_HEADER_SIZE);
    mbuf_begin_read (mbuf);

    // FIXME:  Check for sync
    return max_value_count - value_count;  // value array is too small
  }

  int i;
  OmlValue* val = values;
  for (i = 0; i < value_count; i++, val++) {
    if (unmarshal_value(mbuf, val) == 0) {
      logwarn("unmarshal_values():  Some kind of ERROR in unmarshal_value() call #%d of %d\n",
              i, value_count);
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
      o_log(O_LOG_ERROR,
            "Tried to unmarshal a value from the buffer, "
            "but didn't receive enough data to do that\n");
      return 0;
    }

  int type = mbuf_read_byte (mbuf);
  if (type == -1) return 0;
  OmlValueT oml_type = protocol_type_map[type];

  switch (type) {
  case LONG_T: {
    uint8_t buf [LONG_T_SIZE];

    if (mbuf_read (mbuf, buf, LENGTH (buf)) == -1)
      {
        logerror("Failed to unmarshal OML_LONG_VALUE; not enough data?\n");
        return 0;
      }

    uint32_t hv = ntohl(*((uint32_t*)buf));
    int32_t v = (int32_t)(hv);

    /*
     * The server no longer needs to know about OML_LONG_VALUE, as the
     * marshalling process now maps OML_LONG_VALUE into OML_INT32_VALUE
     * (by truncating to [INT_MIN, INT_MAX].  Therefore, unmarshall a
     * LONG_T value into an OML_INT32_VALUE object.
     */
    value->value.int32Value = v;
    value->type = OML_INT32_VALUE;
    break;
  }
  case INT32_T:
  case UINT32_T:
  case INT64_T:
  case UINT64_T: {
    uint8_t buf [UINT64_T_SIZE]; // Maximum integer size

    if (mbuf_read (mbuf, buf, protocol_size_map[type]) == -1)
      {
        logerror("Failed to unmarshall %d value; not enough data?\n",
               type);
        return 0;
      }

    if (protocol_size_map[type] == 4)
      value->value.uint32Value = ntohl(*((uint32_t*)buf));
    else
      value->value.uint64Value = ntohll(*((uint64_t*)buf));
    value->type = oml_type;
    break;
  }
    case DOUBLE_T: {
      uint8_t buf [DOUBLE_T_SIZE];

      if (mbuf_read (mbuf, buf, LENGTH (buf)) == -1)
        {
          logerror("Failed to unmarshal OML_DOUBLE_VALUE; not enough data?\n");
          return 0;
        }

      int hmant = (int)ntohl(*((uint32_t*)buf));
      double mant = hmant * 1.0 / (1 << BIG_L);
      int exp = (int8_t) buf[4];
      double v = ldexp(mant, exp);
      value->value.doubleValue = v;
      value->type = oml_type;
      break;
    }
    case STRING_T: {
      int len = 0;
      uint8_t buf [STRING_T_MAX_SIZE];

      len = mbuf_read_byte (mbuf);

      if (len == -1 || mbuf_read (mbuf, buf, len) == -1)
        {
          logerror("Failed to unmarshal OML_STRING_VALUE; not enough data?\n");
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
    case BLOB_T: {
      uint32_t n_len;

      if (mbuf_read (mbuf, (uint8_t*)&n_len, 4) == -1) {
        logerror ("Failed to unmarshal OML_BLOB_VALUE length field; not enough data?\n");
        return 0;
      }

      size_t len = ntohl (n_len);
      size_t remaining = mbuf_remaining (mbuf);

      if (len > remaining) {
        logerror ("Failed to unmarshal OML_BLOB_VALUE data:  not enough data available "
                  "(wanted %d, but only have %d bytes\n",
                  len, remaining);
        return 0;
      }

      void *ptr = mbuf_rdptr (mbuf);
      omlc_set_blob (value->value, ptr, len);
      value->type = OML_BLOB_VALUE;
      mbuf_read_skip (mbuf, len);
      break;
    }
    default:
      logerror("Unsupported value type '%d'\n", type);
      return 0;
  }

  return 1;
}

int
unmarshal_typed_value (MBuffer* mbuf, const char* name, OmlValueT type, OmlValue* value)
{
  if (unmarshal_value (mbuf, value) != 1)
    {
      logerror("Error reading %s from binary packet\n", name);
      return -1;
    }

  if (value->type != type)
    {
      logerror("Expected type '%s' for %s, but got type '%d' instead\n",
             oml_type_to_s (type), name, oml_type_to_s (value->type));
      return -1;
    }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
