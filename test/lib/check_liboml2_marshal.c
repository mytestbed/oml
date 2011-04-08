/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <arpa/inet.h>
#include <check.h>
#include <marshal.h>
#include <oml_value.h>
#include <htonll.h>

#include "check_util.h"

#define FIRST_VALPTR(mbuf) (mbuf->base + 7)

#define EPSILON 1e-8

#define MAX_MARSHALLED_STRING_LENGTH 254

static const int LONG_T = 0x1;  // marshal.c LONG_T
static const int DOUBLE_T = 0x2;  // marshal.c DOUBLE_T
static const int STRING_T = 0x4;  // marshal.c STRING_T
static const int INT32_T = 0x5;  // marshal.c INT32_T
static const int UINT32_T = 0x6;  // marshal.c UINT32_T
static const int INT64_T = 0x7;  // marshal.c INT64_T
static const int UINT64_T = 0x8;  // marshal.c UINT64_T

static double double_values [] =
  {
    0.0,
    -0.0,
    1.0,
    -1.0,
    2.0,
    -2.0,
    1.0e-34,
    -1.0e-34,
    1.2345,
    -1.2345,
    0.12345e12,
    -0.12345e12,
    0.12345e24,
    -0.12345e24,
  };

static int32_t int32_values [] =
  {
    0,
    1,
    -1,
    2,
    -2,
    3,
    -3,
    4,
    -4,
    0x7FFFFFFD,
    0x7FFFFFFE,
    0x7FFFFFFF,
    0x80000000,
    0x80000001,
    0x80000002,
    42,
    123456789,
    -123456789,
  };

static long long_values [] =
  {
    0,
    1,
    -1,
    2,
    -2,
    3,
    -3,
    4,
    -4,
    2147483645LL,
    2147483646LL,
    2147483647LL,
#if LONG_MAX > INT_MAX
    2147483648LL,
    2147483649LL,
    2147483650LL,
#endif
    -2147483645LL,
    -2147483646LL,
    -2147483647LL,
    -2147483648LL,
#if LONG_MAX > INT_MAX
    -2147483649LL,
    -2147483650LL,
#endif
    42,
    123456789,
    -123456789,
  };

static int64_t int64_values [] =
  {
    0,
    1,
    -1,
    2,
    -2,
    3,
    -3,
    4,
    -4,
    2147483645LL,
    2147483646LL,
    2147483647LL,
    2147483648LL,
    2147483649LL,
    2147483650LL,
    -2147483645LL,
    -2147483646LL,
    -2147483647LL,
    -2147483648LL,
    -2147483649LL,
    -2147483650LL,
    42,
    123456789,
    -123456789,
    0x1FFFFFFFFLL,
    0x100000000LL,
    0x123456789LL,
    0x7FFFFFFFFFFFFFFDLL,
    0x7FFFFFFFFFFFFFFELL,
    0x7FFFFFFFFFFFFFFFLL,
    0x8000000000000000LL,
    0x8000000000000001LL,
    0x8000000000000002LL,
    0x123456789ABCDEF1LL
  };

static char* string_values [] =
  {
    "",
    "a",
    "ab",
    "abc",
    "abcd",
    "abcde",
    "abcdef",
    "abcdefg",
    "abcdefgh",
    "abcdefghi",
    "abcdefghij",

    /* 254 bytes = (15 * 16 + 14) */
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCD",

    /* 255 bytes = (15 * 16 + 15) */
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDE",

    /* 256 bytes = (16 * 16) */
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF"
    "0123456789ABCDEF",
  };

double relative_error (double v1, double v2)
{
  double a, b;

  if (v1 == v2) return 0.0;

  if (v1 == 0.0 && v2 != 0.0) { a = v2; b = v1; }
  else if (v2 == 0.0 && v1 != 0.0) { a = v1; b = v2; }
  else { a = v2; b = v1; }

  /* a is guaranteed to be non-zero */
  return fabs ((a - b) / a);
}

START_TEST (test_marshal_init)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* result = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != result);
  fail_unless (mbuf->base[2] == 0xAA);
  fail_unless (mbuf->base[3] == 0xAA);
  fail_unless ((int)mbuf->base[4] == OMB_DATA_P);
}
END_TEST

START_TEST (test_marshal_value_long)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.longValue = long_values[_i];
  int result = marshal_value (mbuf, OML_LONG_VALUE, &v);

  uint32_t nv = 0;
  memcpy (&nv, mbuf->base + 8, 4);
  uint32_t hv = ntohl (nv);
  long val = (int32_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != LONG_T); // LONG_T
  fail_if (val != oml_value_clamp_long (v.longValue));
}
END_TEST

START_TEST (test_marshal_value_int32)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.int32Value = int32_values[_i];
  int result = marshal_value (mbuf, OML_INT32_VALUE, &v);

  uint32_t nv = 0;
  memcpy (&nv, FIRST_VALPTR(mbuf) + 1, 4);
  uint32_t hv = ntohl (nv);
  int32_t val = (int32_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != INT32_T); // INT32_T
  fail_if (val != v.int32Value);
}
END_TEST

START_TEST (test_marshal_value_uint32)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.uint32Value = (uint32_t)int32_values[_i];
  int result = marshal_value (mbuf, OML_UINT32_VALUE, &v);

  uint32_t nv = 0;
  memcpy (&nv, FIRST_VALPTR(mbuf) + 1, 4);
  uint32_t hv = ntohl (nv);
  uint32_t val = (uint32_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != UINT32_T); // UINT32_T
  fail_if (val != v.uint32Value);
}
END_TEST

START_TEST (test_marshal_value_int64)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.int64Value = (int64_t)int64_values[_i];
  int result = marshal_value (mbuf, OML_INT64_VALUE, &v);

  uint64_t nv = 0;
  memcpy (&nv, FIRST_VALPTR (mbuf) + 1, 8);
  uint64_t hv = ntohll (nv);
  int64_t val = (int64_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != INT64_T); // INT64_T
  fail_if (val != v.int64Value);
}
END_TEST

START_TEST (test_marshal_value_uint64)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.uint64Value = (uint64_t)int64_values[_i];
  int result = marshal_value (mbuf, OML_UINT64_VALUE, &v);

  uint64_t nv = 0;
  memcpy (&nv, FIRST_VALPTR(mbuf) + 1, 8);
  uint64_t hv = ntohll (nv);
  uint64_t val = (uint64_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != UINT64_T); // UINT64_T
  fail_if (val != v.uint64Value);
}
END_TEST

START_TEST (test_marshal_value_double)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  v.doubleValue = double_values[_i];
  int iexp;
  double dmant = frexp (v.doubleValue, &iexp) * (1 << 30);
  int32_t imant = (int32_t)dmant;

  int result = marshal_value (mbuf, OML_DOUBLE_VALUE, &v);

  uint8_t type = *FIRST_VALPTR(mbuf);
  uint32_t nv = 0;
  memcpy (&nv, FIRST_VALPTR(mbuf) + 1, 4);
  int8_t exp = *(FIRST_VALPTR(mbuf) + 5);
  uint32_t hv = ntohl (nv);
  int mant = (int)hv;

  double val = ldexp (mant * 1.0 / (1 << 30), exp);

  fail_if (result != 1);
  fail_if (type != DOUBLE_T); // DOUBLE_T
  fail_if (mant != imant, "Value %g:  mismatched mantissa, expected %d, got %d\n", double_values[_i], imant, mant);
  fail_if (exp != iexp, "Value %g:  mismatched exponent, expected %d, got %d\n", double_values[_i], iexp, exp);
  fail_if (relative_error (val, v.doubleValue) > EPSILON, "Value %g expected, recovered %f from the buffer, delta=%g\n", double_values[_i], val, double_values[_i] - val);
}
END_TEST

unsigned char string_buf[MAX_MARSHALLED_STRING_LENGTH * 2];

START_TEST (test_marshal_value_string)
{
  MBuffer* mbuf = mbuf_create ();
  MBuffer* pmbuf = marshal_init (mbuf, OMB_DATA_P);

  fail_if (mbuf != pmbuf);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  char* test_string = string_values[_i];
  size_t len = strlen (test_string);
  v.stringValue.ptr = test_string;
  v.stringValue.is_const = 1;
  v.stringValue.size = len;
  v.stringValue.length = len + 1; // Underlying storage.

  int result = marshal_value (mbuf, OML_STRING_VALUE, &v);

  fail_if (result != 1);
  fail_if (*FIRST_VALPTR(mbuf) != 0x4); // STRING_T

  memset (string_buf, 0, MAX_MARSHALLED_STRING_LENGTH);
  memcpy (string_buf, FIRST_VALPTR (mbuf) + 2, *(FIRST_VALPTR (mbuf) + 1));

  if (len <= MAX_MARSHALLED_STRING_LENGTH)
    {
      fail_if (*(FIRST_VALPTR(mbuf) + 1) != len); // Length of string
      fail_if (strlen ((char*)string_buf) != len);
      fail_if (strcmp ((char*)string_buf, test_string) != 0);
      // FIXME:  Check that the buffer is zero past the last element of the test string.
    }
  else
    {
      fail_if (*(FIRST_VALPTR(mbuf) + 1) != MAX_MARSHALLED_STRING_LENGTH); // Length of string
      fail_if (strlen((char*)string_buf) != MAX_MARSHALLED_STRING_LENGTH);
      fail_if (strncmp ((char*)string_buf, test_string, MAX_MARSHALLED_STRING_LENGTH) != 0);
      // FIXME:  Check that the buffer doesn't contain more elements of the test string than it should
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_long)
{
  int VALUES_OFFSET = 7;
  const int UINT32_LENGTH = 5;
  const int UINT32_TYPE_OFFSET = 0;
  const int UINT32_VALUE_OFFSET = 1;
  const int UINT32_SIZE = sizeof (uint32_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (long_values); i++)
    {
      OmlValueU v;
      v.longValue = long_values[i];
      int result = marshal_value (mbuf, OML_LONG_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * UINT32_LENGTH];
      int type =  buf[UINT32_TYPE_OFFSET];
      uint8_t* valptr = &buf[UINT32_VALUE_OFFSET];

      uint32_t nv = 0;
      memcpy (&nv, valptr, UINT32_SIZE);
      uint32_t hv = ntohl (nv);
      long val = (int32_t)hv;

      fail_if (result != 1);
      fail_if (type != LONG_T); // LONG_T
      fail_if (val != oml_value_clamp_long (v.longValue));
    }

  marshal_finalize (mbuf);

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  //  mbuf->rdptr = mbuf->base + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (long_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      /*
       * Since OML_LONG_VALUE is deprecated, server now unmarshals it
       * to OML_INT32_VALUE; the marshalling process clamps the
       * OML_LONG_VALUE to an INT32 anyway.
       */
      fail_unless (value.type == OML_INT32_VALUE);
      fail_unless (value.value.int32Value == oml_value_clamp_long(long_values[i]),
                   "Unmarshalled value %ld, expected %ld\n",
                   value.value.int32Value, oml_value_clamp_long(long_values[i]));
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_int32)
{
  int VALUES_OFFSET = 7;
  const int UINT32_LENGTH = 5;
  const int UINT32_TYPE_OFFSET = 0;
  const int UINT32_VALUE_OFFSET = 1;
  const int UINT32_SIZE = sizeof (uint32_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int32_values); i++)
    {
      OmlValueU v;
      v.int32Value = int32_values[i];
      int result = marshal_value (mbuf, OML_INT32_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * UINT32_LENGTH];
      int type =  buf[UINT32_TYPE_OFFSET];
      uint8_t* valptr = &buf[UINT32_VALUE_OFFSET];

      uint32_t nv = 0;
      memcpy (&nv, valptr, UINT32_SIZE);
      uint32_t hv = ntohl (nv);
      int32_t val = (int32_t)hv;

      fail_if (result != 1);
      fail_if (type != INT32_T); // INT32_T
      fail_if (val != v.int32Value);
    }

  marshal_finalize (mbuf);

  mbuf_read_skip (mbuf, 2);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  //  mbuf->rdptr = mbuf->base + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int32_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_INT32_VALUE);
      fail_unless (value.value.int32Value == int32_values[i],
                   "Unmarshalled value %ld, expected %ld\n",
                   value.value.int32Value, int32_values[i]);
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_uint32)
{
  int VALUES_OFFSET = 7;
  const int UINT32_LENGTH = 5;
  const int UINT32_TYPE_OFFSET = 0;
  const int UINT32_VALUE_OFFSET = 1;
  const int UINT32_SIZE = sizeof (uint32_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int32_values); i++)
    {
      OmlValueU v;
      v.uint32Value = (uint32_t)int32_values[i];
      int result = marshal_value (mbuf, OML_UINT32_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * UINT32_LENGTH];
      int type =  buf[UINT32_TYPE_OFFSET];
      uint8_t* valptr = &buf[UINT32_VALUE_OFFSET];

      uint32_t nv = 0;
      memcpy (&nv, valptr, UINT32_SIZE);
      uint32_t hv = ntohl (nv);
      uint32_t val = (uint32_t)hv;

      fail_if (result != 1);
      fail_if (type != UINT32_T); // UINT32_T
      fail_if (val != v.uint32Value);
    }

  marshal_finalize (mbuf);

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  //  mbuf->rdptr = mbuf->base + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int32_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_UINT32_VALUE);
      fail_unless (value.value.uint32Value == (uint32_t)int32_values[i],
                   "Unmarshalled value %ld, expected %ld\n",
                   value.value.uint32Value, (uint32_t)int32_values[i]);
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_int64)
{
  int VALUES_OFFSET = 7;
  const int UINT64_LENGTH = 9;
  const int UINT64_TYPE_OFFSET = 0;
  const int UINT64_VALUE_OFFSET = 1;
  const int UINT64_SIZE = sizeof (uint64_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int64_values); i++)
    {
      OmlValueU v;
      v.int64Value = int64_values[i];
      int result = marshal_value (mbuf, OML_INT64_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * UINT64_LENGTH];
      int type =  buf[UINT64_TYPE_OFFSET];
      uint8_t* valptr = &buf[UINT64_VALUE_OFFSET];

      uint64_t nv = 0;
      memcpy (&nv, valptr, UINT64_SIZE);
      uint64_t hv = ntohll (nv);
      int64_t val = (int64_t)hv;

      fail_if (result != 1);
      fail_if (type != INT64_T); // INT64_T
      fail_if (val != v.int64Value);
    }

  marshal_finalize (mbuf);

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  //  mbuf->rdptr = mbuf->base + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int64_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_INT64_VALUE);
      fail_unless (value.value.int64Value == int64_values[i],
                   "Unmarshalled value %ld, expected %ld\n",
                   value.value.int64Value, int64_values[i]);
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_uint64)
{
  int VALUES_OFFSET = 7;
  const int UINT64_LENGTH = 9;
  const int UINT64_TYPE_OFFSET = 0;
  const int UINT64_VALUE_OFFSET = 1;
  const int UINT64_SIZE = sizeof (uint64_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int64_values); i++)
    {
      OmlValueU v;
      v.uint64Value = (uint64_t)int64_values[i];
      int result = marshal_value (mbuf, OML_UINT64_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * UINT64_LENGTH];
      int type =  buf[UINT64_TYPE_OFFSET];
      uint8_t* valptr = &buf[UINT64_VALUE_OFFSET];

      uint64_t nv = 0;
      memcpy (&nv, valptr, UINT64_SIZE);
      uint64_t hv = ntohll (nv);
      uint64_t val = (uint64_t)hv;

      fail_if (result != 1);
      fail_if (type != UINT64_T); // UINT64_T
      fail_if (val != v.uint64Value);
    }

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  //  mbuf->rdptr = mbuf->base + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int64_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_UINT64_VALUE);
      fail_unless (value.value.uint64Value == (uint64_t)int64_values[i],
                   "Unmarshalled value %ld, expected %ld\n",
                   value.value.uint64Value, (uint64_t)int64_values[i]);
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_double)
{
  int VALUES_OFFSET = 7;
  const int DOUBLE_LENGTH = 6;
  const int DOUBLE_TYPE_OFFSET = 0;
  const int DOUBLE_MANT_OFFSET = 1;
  const int DOUBLE_EXP_OFFSET = 5;
  const int DOUBLE_MANT_SIZE = sizeof (int32_t);
  int result;

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (double_values); i++)
    {
      OmlValueU v;
      v.doubleValue = double_values[i];
      result = marshal_value (mbuf, OML_DOUBLE_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * DOUBLE_LENGTH];
      int type =  buf[DOUBLE_TYPE_OFFSET];
      uint8_t* mantptr = &buf[DOUBLE_MANT_OFFSET];
      uint8_t* expptr = &buf[DOUBLE_EXP_OFFSET];

      uint32_t nv = 0;
      memcpy (&nv, mantptr, DOUBLE_MANT_SIZE);
      uint32_t hv = ntohl (nv);
      int32_t mant = (int32_t)hv;
      int8_t exp = *((int8_t*)expptr);

      double val = ldexp (mant * 1.0 / (1 << 30), exp);

      fail_if (result != 1);
      fail_if (type != DOUBLE_T, "Type == %d", type); // DOUBLE_T
      fail_if (fabs((val - v.doubleValue)/v.doubleValue) >= EPSILON,
               "Unmarshalled %g, expected %g\n",
               val, v.doubleValue);
    }

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  unmarshal_init (mbuf, &header);

  //  umbuf.curr_p = umbuf.buffer + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (double_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_DOUBLE_VALUE);
      fail_unless (relative_error (value.value.doubleValue, double_values[i]) < EPSILON,
                   "Unmarshalled value %g, expected %g\n",
                   value.value.doubleValue, double_values[i]);
    }
}
END_TEST

START_TEST (test_marshal_unmarshal_string)
{
  int VALUES_OFFSET = 7;
  const int STRING_TYPE_OFFSET = 0;
  const int STRING_LENGTH_OFFSET = 1;
  const int STRING_VALUE_OFFSET = 2;
  int result;
  char string[MAX_MARSHALLED_STRING_LENGTH + 16];

  MBuffer* mbuf = mbuf_create ();
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);
  int current_index = VALUES_OFFSET;

  unsigned int i = 0;
  for (i = 0; i < LENGTH (string_values); i++)
    {
      memset (string, 0, LENGTH(string));
      OmlValueU v;
      v.stringValue.ptr = string_values[i];
      v.stringValue.is_const = 1;
      v.stringValue.size = strlen (string_values[i]);
      v.stringValue.length = v.stringValue.size + 1; // Underlying storage.

      result = marshal_value (mbuf, OML_STRING_VALUE, &v);

      uint8_t* buf = &mbuf->base[current_index];
      int type =  buf[STRING_TYPE_OFFSET];
      uint8_t* lenptr = &buf[STRING_LENGTH_OFFSET];
      uint8_t* valptr = &buf[STRING_VALUE_OFFSET];
      size_t len = *lenptr;

      memcpy (string, valptr, len);

      fail_if (result != 1);
      fail_if (type != STRING_T); // STRING_T

      if (strlen (string_values[i]) <= MAX_MARSHALLED_STRING_LENGTH)
        {
          fail_if (len != strlen (string_values[i]));
          fail_if (strcmp (string, string_values[i]) != 0,
                   "Expected string:\n%s\nActual string:\n%s\n",
                   string_values[i],
                   string);
        }
      else
        {
          fail_if (len != MAX_MARSHALLED_STRING_LENGTH);
          fail_if (strlen (string) != len);
          fail_if (strncmp (string, string_values[i], MAX_MARSHALLED_STRING_LENGTH) != 0);
        }

      current_index += len + 2;
    }

  marshal_finalize (mbuf);

  // Skip over the padding bytes introduced by the kludge for
  // marshalling possibly long packets.
  mbuf_read_skip (mbuf, 2);

  OmlBinaryHeader header;
  unmarshal_init (mbuf, &header);

  //  umbuf.curr_p = umbuf.buffer + VALUES_OFFSET; // Kludge!

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (string_values); i++)
    {
      OmlValue value;

      unmarshal_value (mbuf, &value);

      fail_unless (value.type == OML_STRING_VALUE);

      int original_length = strlen (string_values[i]);
      int len = strlen (value.value.stringValue.ptr);
      if (original_length <= MAX_MARSHALLED_STRING_LENGTH)
        {
          fail_unless (len == original_length, "Expected length %d, unmarshalled length %d\n", original_length, len);
          fail_unless (strcmp (value.value.stringValue.ptr, string_values[i]) == 0,
                       "Expected string:\n%s\nUnmarshalled string:\n%s\n",
                       value.value.stringValue.ptr, string_values[i]);
        }
      else
        {
          fail_unless (len == MAX_MARSHALLED_STRING_LENGTH);
          fail_unless (strncmp (value.value.stringValue.ptr, string_values[i], MAX_MARSHALLED_STRING_LENGTH) == 0,
                       "Expected string:\n%s\nUnmarshalled string:\n%s\n",
                       value.value.stringValue.ptr, string_values[i]);
        }
    }
}
END_TEST


Suite*
marshal_suite (void)
{
  Suite* s = suite_create ("Marshal");

  /* Marshalling test cases */
  TCase* tc_marshal = tcase_create ("Marshal");

  /* Add tests to "Marshal" */
  tcase_add_test (tc_marshal, test_marshal_init);
  tcase_add_loop_test (tc_marshal, test_marshal_value_long,   0, LENGTH (long_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_int32,  0, LENGTH (int32_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_uint32, 0, LENGTH (int32_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_int64,  0, LENGTH (int64_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_uint64, 0, LENGTH (int64_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_double, 0, LENGTH (double_values));
  tcase_add_loop_test (tc_marshal, test_marshal_value_string, 0, LENGTH (string_values));

  tcase_add_test (tc_marshal, test_marshal_unmarshal_long);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_int32);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_uint32);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_int64);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_uint64);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_double);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_string);

  suite_add_tcase (s, tc_marshal);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
