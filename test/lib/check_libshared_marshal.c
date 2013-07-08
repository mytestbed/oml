/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
#include <inttypes.h>
#include <math.h>
#include <arpa/inet.h>
#include <check.h>

#include "oml2/omlc.h"
#include "oml_util.h"
#include "htonll.h"
#include "oml_value.h"
#include "marshal.h"
#include "check_util.h"

#define FIRST_VALPTR(mbuf) (mbuf->base + 5)

#define EPSILON 1e-8

#define MAX_MARSHALLED_STRING_LENGTH 254

static const int LONG_T = 0x1;        // marshal.c LONG_T
static const int DOUBLE_T = 0x2;      // marshal.c DOUBLE_T
static const int DOUBLE_NAN = 0x3;    // marshal.c DOUBLE_NAN
static const int STRING_T = 0x4;      // marshal.c STRING_T
static const int INT32_T = 0x5;       // marshal.c INT32_T
static const int UINT32_T = 0x6;      // marshal.c UINT32_T
static const int INT64_T = 0x7;       // marshal.c INT64_T
static const int UINT64_T = 0x8;      // marshal.c UINT64_T
static const int BLOB_T = 0x9;        // marshal.c BLOB_T
static const int GUID_T = 0xa;        // marshal.c GUID_T
static const int BOOL_FALSE_T = 0xb;  // marshal.c BOOL_FALSE_T
static const int BOOL_TRUE_T = 0xc;   // marshal.c BOOL_TRUE_T

#define PACKET_HEADER_SIZE 5 // marshal.c

static double double_values[] = {
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
  NAN,
};

static int32_t int32_values[] = {
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

static long long_values[] = {
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

static int64_t int64_values[] = {
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

static char* string_values[] = {
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

static const oml_guid_t guid_values[] = {
  UINT64_C(0x260a42fc515c3908),
  UINT64_C(0xd99f503f0d1fa354),
  UINT64_C(0x476d34b3f0fad7c4),
  UINT64_C(0x8ff1a1d42ec376a4),
  UINT64_C(0x15d8753573ebffa0),
  UINT64_C(0xa0bfe15748f8590f),
  UINT64_C(0xb7c8259f4120a29e),
  UINT64_C(0xe75c8763c4e1964c),
  UINT64_C(0x3d51cfbb1f13bba8),
  UINT64_C(0xa16bbf3bea144dd2),
  UINT64_C(0x811db8443b1630c0),
  UINT64_C(0x0659e7379b9973df),
  UINT64_C(0x398d76e0f527c258),
  UINT64_C(0xa5b70a2f38c881de),
  UINT64_C(0xec39e65a696ebe79),
  UINT64_C(0xdb140600a1ad20e4),
  UINT64_C(0xe35fb70c38023c68),
  UINT64_C(0xcef251ecf411bfa3),
  UINT64_C(0x7684bfeadcde2648),
  UINT64_C(0x0222091b4aa762b0)
};

static const uint8_t bool_values[] = {
  0,
  1,
  2,
};

double
relative_error (double v1, double v2)
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
  int result = marshal_init (mbuf, OMB_DATA_P);

  fail_if (result != 0);
  fail_unless (mbuf->base[0] == 0xAA);
  fail_unless (mbuf->base[1] == 0xAA);
  fail_unless ((int)mbuf->base[2] == OMB_DATA_P);
}
END_TEST

START_TEST (test_marshal_value_long)
{
  MBuffer* mbuf = mbuf_create ();
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_long(v, long_values[_i]);
  int result = marshal_value (mbuf, OML_LONG_VALUE, &v);

  uint32_t nv = 0;
  memcpy (&nv, FIRST_VALPTR(mbuf) + 1, 4);
  uint32_t hv = ntohl (nv);
  long val = (int32_t)hv;

  fail_if (result != 1);
  fail_if ((int)*(FIRST_VALPTR(mbuf)) != LONG_T); // LONG_T
  fail_if (val != oml_value_clamp_long (v.longValue),
      "Improrely clamped LONG: expected %ld, got %ld",
      val, oml_value_clamp_long (v.longValue));
}
END_TEST

START_TEST (test_marshal_value_int32)
{
  MBuffer* mbuf = mbuf_create ();
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_int32(v, int32_values[_i]);
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
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_uint32(v, (uint32_t)int32_values[_i]);
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
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_int64(v, (int64_t)int64_values[_i]);
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
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_uint64(v, (uint64_t)int64_values[_i]);
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
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_double(v, double_values[_i]);
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
  if (isnan(double_values[_i])) {
    fail_if (type != DOUBLE_NAN);
    imant = 0;
    iexp = 0;
  } else {
    fail_if (type != DOUBLE_T);
  }
  fail_if (mant != imant, "Value %g:  mismatched mantissa, expected %d, got %d\n", double_values[_i], imant, mant);
  fail_if (exp != iexp, "Value %g:  mismatched exponent, expected %d, got %d\n", double_values[_i], iexp, exp);
  fail_if (relative_error (val, v.doubleValue) > EPSILON, "Value %g expected, recovered %f from the buffer, delta=%g\n", double_values[_i], val, double_values[_i] - val);
}
END_TEST

unsigned char string_buf[MAX_MARSHALLED_STRING_LENGTH * 2];

START_TEST (test_marshal_value_string)
{
  MBuffer* mbuf = mbuf_create ();
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  char* test_string = string_values[_i];
  size_t len = strlen (test_string);
  omlc_set_string(v, test_string);

  int result = marshal_value (mbuf, OML_STRING_VALUE, &v);

  omlc_reset_string(v);

  fail_if (result != 1);
  fail_if (*FIRST_VALPTR(mbuf) != 0x4); // STRING_T

  memset (string_buf, 0, MAX_MARSHALLED_STRING_LENGTH);
  memcpy (string_buf, FIRST_VALPTR (mbuf) + 2, *(FIRST_VALPTR (mbuf) + 1));

  if (len <= MAX_MARSHALLED_STRING_LENGTH) {
      fail_if (*(FIRST_VALPTR(mbuf) + 1) != len); // Length of string
      fail_if (strlen ((char*)string_buf) != len);
      fail_if (strcmp ((char*)string_buf, test_string) != 0);
      // FIXME:  Check that the buffer is zero past the last element of the test string.
  } else {
      fail_if (*(FIRST_VALPTR(mbuf) + 1) != MAX_MARSHALLED_STRING_LENGTH); // Length of string
      fail_if (strlen((char*)string_buf) != MAX_MARSHALLED_STRING_LENGTH);
      fail_if (strncmp ((char*)string_buf, test_string, MAX_MARSHALLED_STRING_LENGTH) != 0);
      // FIXME:  Check that the buffer doesn't contain more elements of the test string than it should
  }
}
END_TEST

#warning test_marshal_value_blob is missing

START_TEST(test_marshal_guid)
{
  MBuffer* mbuf = mbuf_create ();
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_guid(v, guid_values[_i]);
  int result = marshal_value(mbuf, OML_GUID_VALUE, &v);
  fail_if(result != 1);
  fail_if(*(FIRST_VALPTR(mbuf)) != GUID_T);

  uint64_t nv = 0;
  memcpy(&nv, FIRST_VALPTR(mbuf) + 1, sizeof(nv));
  oml_guid_t val = (oml_guid_t) ntohll(nv);
  fail_if(val != guid_values[_i], "%llx != %llx\n", val, guid_values[_i]);
}
END_TEST

START_TEST(test_marshal_bool)
{
  MBuffer* mbuf = mbuf_create ();
  int initresult = marshal_init (mbuf, OMB_DATA_P);

  fail_if (initresult != 0);
  fail_if (mbuf->base == NULL);

  OmlValueU v;
  omlc_zero(v);
  omlc_set_bool(v, bool_values[_i]);
  int result = marshal_value(mbuf, OML_BOOL_VALUE, &v);
  fail_if(result != 1);
  fail_if(*(FIRST_VALPTR(mbuf)) != BOOL_TRUE_T && *(FIRST_VALPTR(mbuf)) != BOOL_FALSE_T);

  uint8_t val = (uint8_t) *FIRST_VALPTR(mbuf);
  fail_if((val == BOOL_FALSE_T && bool_values[_i]) ||
      (val == BOOL_TRUE_T && !bool_values[_i]),
      "%d and %d do not have the same truth value",
      (val==BOOL_TRUE_T)?1:((val==BOOL_FALSE_T)?0:-1), bool_values[_i]);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (long_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_long(v, long_values[i]);
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

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (long_values); i++) {
    unmarshal_value (mbuf, &value);

    /*
     * Since OML_LONG_VALUE is deprecated, server now unmarshals it
     * to OML_INT32_VALUE; the marshalling process clamps the
     * OML_LONG_VALUE to an INT32 anyway.
     */
    fail_unless (oml_value_get_type(&value) == OML_INT32_VALUE);
    fail_unless (omlc_get_int32(*oml_value_get_value(&value)) == oml_value_clamp_long(long_values[i]),
        "Unmarshalled value %ld, expected %ld\n",
        omlc_get_int32(*oml_value_get_value(&value)), oml_value_clamp_long(long_values[i]));
  }

  oml_value_reset(&value);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int32_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_int32(v, int32_values[i]);
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

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int32_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_INT32_VALUE);
    fail_unless (omlc_get_int32(*oml_value_get_value(&value)) == int32_values[i],
        "Unmarshalled value %ld, expected %ld\n",
        omlc_get_int32(*oml_value_get_value(&value)), int32_values[i]);
  }

  oml_value_reset(&value);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int32_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_uint32(v, (uint32_t)int32_values[i]);
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

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int32_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_UINT32_VALUE);
    fail_unless (omlc_get_uint32(*oml_value_get_value(&value)) == (uint32_t)int32_values[i],
        "Unmarshalled value %ld, expected %ld\n",
        omlc_get_uint32(*oml_value_get_value(&value)), (uint32_t)int32_values[i]);
  }
  oml_value_reset(&value);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int64_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_int64(v, int64_values[i]);
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

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int64_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_INT64_VALUE);
    fail_unless (omlc_get_int64(*oml_value_get_value(&value)) == int64_values[i],
        "Unmarshalled value %ld, expected %ld\n",
        omlc_get_int64(*oml_value_get_value(&value)), int64_values[i]);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (int64_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_uint64(v, (uint64_t)int64_values[i]);
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

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (int64_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_UINT64_VALUE);
    fail_unless (omlc_get_uint64(*oml_value_get_value(&value)) == (uint64_t)int64_values[i],
        "Unmarshalled value %ld, expected %ld\n",
        omlc_get_uint64(*oml_value_get_value(&value)), (uint64_t)int64_values[i]);
  }
  oml_value_reset(&value);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (double_values); i++) {
    OmlValueU v;
    omlc_zero(v);
    omlc_set_double(v, double_values[i]);
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
    if (isnan(double_values[i])) {
      fail_if (type != DOUBLE_NAN, "Type == %d", type); // DOUBLE_NAN
      /* NaN is actually encoded as 0., but only the type, DOUBLE_NAN, matters for unmarshalling */
      fail_unless (-EPSILON <= val && val <= EPSILON,
          "Unmarshalled %g, expected %g\n",
          val, 0);

    } else {
      fail_if (type != DOUBLE_T, "Type == %d", type); // DOUBLE_T
      fail_if (fabs((val - omlc_get_double(v))/omlc_get_double(v)) >= EPSILON,
          "Unmarshalled %g, expected %g\n",
          val, omlc_get_double(v));
    }
  }

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  unmarshal_init (mbuf, &header);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (double_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_DOUBLE_VALUE);
    if (isnan(double_values[i])) {
      fail_unless (isnan(omlc_get_double(*oml_value_get_value(&value))),
          "Unmarshalled value %g, expected %g\n",
          omlc_get_double(*oml_value_get_value(&value)), double_values[i]);

    } else {
      fail_unless (relative_error (omlc_get_double(*oml_value_get_value(&value)), double_values[i]) < EPSILON,
          "Unmarshalled value %g, expected %g\n",
          omlc_get_double(*oml_value_get_value(&value)), double_values[i]);
    }
  }
  oml_value_reset(&value);
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
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);
  int current_index = VALUES_OFFSET;

  unsigned int i = 0;
  for (i = 0; i < LENGTH (string_values); i++) {
    memset (string, 0, LENGTH(string));
    OmlValueU v;
    omlc_zero(v);
    omlc_set_string(v, string_values[i]);

    result = marshal_value (mbuf, OML_STRING_VALUE, &v);

    omlc_reset_string(v);

    uint8_t* buf = &mbuf->base[current_index];
    int type =  buf[STRING_TYPE_OFFSET];
    uint8_t* lenptr = &buf[STRING_LENGTH_OFFSET];
    uint8_t* valptr = &buf[STRING_VALUE_OFFSET];
    size_t len = *lenptr;

    memcpy (string, valptr, len);

    fail_if (result != 1);
    fail_if (type != STRING_T); // STRING_T

    if (strlen (string_values[i]) <= MAX_MARSHALLED_STRING_LENGTH) {
      fail_if (len != strlen (string_values[i]));
      fail_if (strcmp (string, string_values[i]) != 0,
          "Expected string:\n%s\nActual string:\n%s\n",
          string_values[i],
          string);
    } else {
      fail_if (len != MAX_MARSHALLED_STRING_LENGTH);
      fail_if (strlen (string) != len);
      fail_if (strncmp (string, string_values[i], MAX_MARSHALLED_STRING_LENGTH) != 0);
    }

    current_index += len + 2;
  }

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  unmarshal_init (mbuf, &header);
  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (string_values); i++) {
    unmarshal_value (mbuf, &value);

    fail_unless (oml_value_get_type(&value) == OML_STRING_VALUE);
    fail_if (omlc_get_string_ptr(*oml_value_get_value(&value)) == NULL);

    int original_length = strlen (string_values[i]);
    int len = omlc_get_string_length(*oml_value_get_value(&value));
    if (original_length <= MAX_MARSHALLED_STRING_LENGTH) {
      fail_unless (len == original_length, "Expected length %d, unmarshalled length %d\n", original_length, len);
      fail_unless (strcmp (omlc_get_string_ptr(*oml_value_get_value(&value)), string_values[i]) == 0,
          "Expected string: '%s', Unmarshalled string:'%s'\n",
          omlc_get_string_ptr(*oml_value_get_value(&value)), string_values[i]);
    } else {
      fail_unless (len == MAX_MARSHALLED_STRING_LENGTH);
      fail_unless (strncmp (omlc_get_string_ptr(*oml_value_get_value(&value)), string_values[i], MAX_MARSHALLED_STRING_LENGTH) == 0,
          "Expected string: '%s' Unmarshalled string: '%s'",
          omlc_get_string_ptr(*oml_value_get_value(&value)), string_values[i]);
    }
  }
  oml_value_reset(&value);
}
END_TEST

#warning test_unmarshal_value_blob is missing

START_TEST(test_marshal_unmarshal_guid)
{
  int VALUES_OFFSET = 7;
  const int GUID_LENGTH = 9;
  const int GUID_TYPE_OFFSET = 0;
  const int GUID_VALUE_OFFSET = 1;
  const int GUID_SIZE = sizeof(oml_guid_t);
  int result;
  OmlValue value;

  oml_value_init(&value);
  MBuffer* mbuf = mbuf_create();
  marshal_init(mbuf, OMB_DATA_P);
  result = marshal_measurements(mbuf, 42, 43, 42.0);
  fail_if(mbuf->base == NULL);
  fail_if(result == -1);
  VALUES_OFFSET = mbuf_fill(mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (guid_values); i++) {
    OmlValueU v;
    omlc_guid_generate(v);
    omlc_set_guid(v, guid_values[i]);

    int result = marshal_value(mbuf, OML_GUID_VALUE, &v);
    uint8_t* buf = &(mbuf->base[VALUES_OFFSET + i * GUID_LENGTH]);
    int type = buf[GUID_TYPE_OFFSET];
    uint8_t* valptr = &buf[GUID_VALUE_OFFSET];

    uint64_t nv = 0;
    memcpy(&nv, valptr, GUID_SIZE);
    uint64_t hv = ntohll(nv);
    oml_guid_t val = (oml_guid_t)hv;

    fail_if(result != 1);
    fail_if(type != GUID_T);
    fail_if(val != v.guidValue, "%llx != %llx\n", val, v.guidValue);
  }
  marshal_finalize(mbuf);

  OmlBinaryHeader header;
  result = unmarshal_init(mbuf, &header);
  fail_if(result == -1);
  fail_unless(header.type == OMB_DATA_P);
  for (i = 0; i < LENGTH(guid_values); i++) {
    unmarshal_value(mbuf, &value);
    fail_unless(oml_value_get_type(&value) == OML_GUID_VALUE);
    fail_unless(omlc_get_guid(value.value) == guid_values[i],
                "Unmarshalled value %llx, expected %llx\n",
                omlc_get_guid(value.value), guid_values[i]);
  }
  oml_value_reset(&value);
}
END_TEST

START_TEST (test_marshal_unmarshal_bool)
{
  int VALUES_OFFSET = 7;
  const int BOOL_LENGTH = 1;
  const int BOOL_VALUE_OFFSET = 0;
  uint8_t val;
  int result;
  OmlValue value;

  oml_value_init(&value);

  MBuffer* mbuf = mbuf_create ();
  marshal_init (mbuf, OMB_DATA_P);
  result = marshal_measurements (mbuf, 42, 43, 42.0);

  fail_if (mbuf->base == NULL);
  fail_if (result == -1);

  VALUES_OFFSET = mbuf_fill (mbuf);

  unsigned int i = 0;
  for (i = 0; i < LENGTH (bool_values); i++) {
      OmlValueU v;
      omlc_zero(v);
      omlc_set_bool(v, bool_values[i]);
      int result = marshal_value (mbuf, OML_BOOL_VALUE, &v);

      uint8_t* buf = &mbuf->base[VALUES_OFFSET + i * BOOL_LENGTH];
      uint8_t* valptr = &buf[BOOL_VALUE_OFFSET];

      fail_if (result != 1);
      fail_unless (BOOL_FALSE_T == *valptr || BOOL_TRUE_T == *valptr);
  }

  marshal_finalize (mbuf);

  OmlBinaryHeader header;
  result = unmarshal_init (mbuf, &header);
  fail_if (result == -1);

  fail_unless (header.type == OMB_DATA_P);

  for (i = 0; i < LENGTH (bool_values); i++) {
      unmarshal_value (mbuf, &value);

      fail_unless (oml_value_get_type(&value) == OML_BOOL_VALUE);
      val = omlc_get_bool(*oml_value_get_value(&value));
      fail_unless ((val && bool_values[i]) ||
          (!val && !bool_values[i]),
          "%d (shouldn't be -1) and %d do not have the same truth value",
          (BOOL_TRUE_T==val)?1:((BOOL_FALSE_T==val)?0:-1), bool_values[_i]);
  }

  oml_value_reset(&value);
}
END_TEST


#define dumpmessage(mbuf)                                                                                   \
  do {                                                                                                      \
    uint8_t *_m = mbuf_message(mbuf), *_p = _m;                                                             \
    logdebug("Message of length %d\n", mbuf_message_length(mbuf));                                          \
    do logdebug("> %02d: %#02x\t(%c)\n", (int)(_p-_m), (uint8_t)*_p, ((uint8_t)*_p>=' '?(uint8_t)*_p:'X')); \
    while (++_p < _m + mbuf_message_length(mbuf));                                                          \
  } while(0)

START_TEST (test_marshal_full)
{
  MBuffer *mbuf;
  OmlValue v, *va;
  OmlBinaryHeader h;
  int32_t d32 = -42;
  uint32_t u32 = 1337;
  int64_t d64 = (int64_t)d32 << 32;
  uint64_t u64 = (uint64_t)u32 << 32;
  double d = M_PI, d2;
  long l = d32;
  char s[] = "I am both a string AND a blob... Go figure.";
  oml_guid_t guid = omlc_guid_generate();
  uint8_t bfalse = OMLC_BOOL_FALSE, btrue = OMLC_BOOL_TRUE, bval;
  void *b = (void*)s, *p;
  int len = strlen(s), count=0;
  uint8_t *msg, offset;

  o_set_log_level(O_LOG_DEBUG2);

  oml_value_init(&v);

  /*
   * MARSHALLING STARTS HERE
   */
  mbuf = mbuf_create();
  fail_if(marshal_init(mbuf, OMB_DATA_P));
  msg = mbuf_message(mbuf);
  offset = 0;
  fail_unless(msg[offset] == 0xAA,
      "Fist sync byte not set properly; found '%#x' at offset %d",
      msg[offset], offset);
  offset++;
  fail_unless(msg[offset] == 0xAA,
      "Second sync byte not set properly; found '%#x' at offset %d",
      msg[offset], offset);
  offset++;
  fail_unless(msg[offset] == OMB_DATA_P,
      "Packet type not set properly; found '%#x' at offset %d",
      msg[offset], offset);
  offset++;
  fail_unless(mbuf_message_length(mbuf) == 5,
      "Current message should be 5 bytes");
  offset+=2;

  /* Verify length */
  fail_unless(marshal_measurements(mbuf, 1, 2, 3.) == 1);
  /* Before finalisation, the first byte of length contains the number of elements */
  fail_unless(msg[5] == 0,
      "Initial number of element not set properly; got %d instead of 0 at offset %d",
      msg[5], offset);
  offset++;
  fail_unless(msg[offset] == 1,
      "Stream number not set properly; got %d instead of 1 at offset %d",
      msg[offset], offset);
  offset++;

  /* Verify seqno */
  fail_unless(msg[offset] == INT32_T,
      "Seqno type not set properly; got %d instead of %d at offset %d",
      msg[offset], INT32_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohl(*(int32_t*)p) == 2,
      "Seqno not set properly; got %d instead of 2 at offset %d",
      ntohl(*(int32_t*)p), offset);
  offset+=4;

  /* Verify timestamp */
  fail_unless(msg[offset] == DOUBLE_T,
      "Timestamp type not set properly; got %d instead of %d",
      msg[offset], DOUBLE_T);
  offset++;
  d2 = ((double)(ntohl((int32_t)(msg[offset])) * (1 << msg[offset+4])))/(1<<30);
  fail_unless(d2 - 3. < EPSILON,
      "Timestamp not set properly; got M=%d, x=%d; 2^xM/2^30=%f instead of 3",
      ntohl((int32_t)(msg[offset])), msg[offset+4], d2, offset);
  offset+=5;

  /* Marshall and verify INT32 */
  oml_value_set_type(&v, OML_INT32_VALUE);
  omlc_set_int32(*oml_value_get_value(&v), d32);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == INT32_T,
      "d32 type not set properly; got %d instead of %d at offset %d",
      msg[offset], INT32_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohl(*(int32_t*)p) == d32,
      "d32 not set properly; got %d instead of %" PRId32 " at offset %d",
      ntohl(*(int32_t*)p), d32, offset);
  offset+=sizeof(int32_t);

  /* Marshall and verify UINT32 */
  oml_value_set_type(&v, OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v), u32);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == UINT32_T,
      "u32 type not set properly; got %d instead of %d at offset %d",
      msg[offset], UINT32_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohl(*(uint32_t*)p) == u32,
      "u32 not set properly; got %d instead of %" PRIu32 " at offset %d",
      ntohl(*(uint32_t*)p), u32, offset);
  offset+=sizeof(uint32_t);

  /* Marshall and verify INT64 */
  oml_value_set_type(&v, OML_INT64_VALUE);
  omlc_set_int64(*oml_value_get_value(&v), d64);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == INT64_T,
      "d64 type not set properly; got %d instead of %d at offset %d",
      msg[offset], INT64_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohll(*(int64_t*)p) == d64,
      "d64 not set properly; got %d instead of %" PRId64 " at offset %d",
      ntohll(*(int64_t*)p), d64, offset);
  offset+=sizeof(int64_t);

  /* Marshall and verify UINT64 */
  oml_value_set_type(&v, OML_UINT64_VALUE);
  omlc_set_uint64(*oml_value_get_value(&v), u64);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == UINT64_T,
      "u64 type not set properly; got %d instead of %d at offset %d",
      msg[offset], UINT64_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohll(*(uint64_t*)p) == u64,
      "u64 not set properly; got %d instead of %" PRIu64 " at offset %d",
      ntohll(*(uint64_t*)p), u64, offset);
  offset+=sizeof(uint64_t);

  /* Marshall and verify LONG */
  oml_value_set_type(&v, OML_LONG_VALUE);
  omlc_set_long(*oml_value_get_value(&v), l);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == LONG_T,
      "l type not set properly; got %d instead of %d at offset %d",
      msg[offset], LONG_T, offset);
  offset++;
  p = &msg[offset];
  /* Longs are 4 bytes on the wire, the size of an int32_t*/
  fail_unless(ntohl(*(int32_t*)p) == oml_value_clamp_long(l),
      "l not set properly; got %d instead of %ld at offset %d",
      ntohl(*(int32_t*)p), oml_value_clamp_long(l), offset);
  offset+=sizeof(int32_t);

  /* Marshall and verify DOUBLE */
  oml_value_set_type(&v, OML_DOUBLE_VALUE);
  omlc_set_double(*oml_value_get_value(&v), d);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == DOUBLE_T,
      "d type not set properly; got %d instead of %d",
      msg[offset], DOUBLE_T);
  offset++;
  d2 = ((double)(ntohl((int32_t)(msg[offset])) * (1 << msg[offset+4])))/(1<<30);
  fail_unless(d2 - d < EPSILON,
      "d not set properly; got M=%d, x=%d; 2^xM/2^30=%f instead of 2",
      ntohl((int32_t)(msg[offset])), msg[offset+4], d2, offset);
  offset+=5;

  /* Marshall and verify STRING */
  oml_value_set_type(&v, OML_STRING_VALUE);
  omlc_set_string_copy(*oml_value_get_value(&v), s, len);
  marshal_values(mbuf, &v, 1);
  omlc_reset_string(*oml_value_get_value(&v));
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == STRING_T,
      "s type not set properly; got %d instead of %d at offset %d",
      msg[offset], STRING_T, offset);
  offset++;
  fail_unless(msg[offset] == len,
      "s length not set properly; got %d instead of %d at offset %d",
      msg[offset], len, offset);
  offset++;
  fail_if(strncmp((char *)&msg[offset], s, len),
      "s mismatch");
  offset+=len;

  /* Marshall and verify BLOB */
  oml_value_set_type(&v, OML_BLOB_VALUE);
  omlc_set_blob(*oml_value_get_value(&v), s, len);
  marshal_values(mbuf, &v, 1);
  omlc_reset_blob(*oml_value_get_value(&v));
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == BLOB_T,
      "b type not set properly; got %d instead of %d at offset %d",
      msg[offset], BLOB_T, offset);
  offset++;
  /* Blobs have a 32 bits length field */
  p = &msg[offset];
  fail_unless(ntohl(*(uint32_t*)p) == len,
      "b length not set properly; got %d instead of %" PRIu32 " at offset %d",
      ntohl(*(uint32_t*)p), len, offset);
  offset+=sizeof(uint32_t);
  fail_if(strncmp((char *)&msg[offset], s, len),
      "b mismatch");
  offset+=len;

  /* Marshall and verify GUID */
  oml_value_set_type(&v, OML_GUID_VALUE);
  omlc_set_guid(*oml_value_get_value(&v), guid);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == GUID_T,
      "guid type not set properly; got %d instead of %d at offset %d",
      msg[offset], GUID_T, offset);
  offset++;
  p = &msg[offset];
  fail_unless(ntohll(*(oml_guid_t*)p) == guid,
      "guid not set properly; got %d instead of %" PRIu64 " at offset %d",
      ntohll(*(oml_guid_t*)p), (uint64_t)guid, offset);
  offset+=sizeof(oml_guid_t);

  /* Marshall and verify BOOL FALSE */
  oml_value_set_type(&v, OML_BOOL_VALUE);
  omlc_set_bool(*oml_value_get_value(&v), bfalse);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == BOOL_FALSE_T,
      "bool type not set properly; got %d instead of %d at offset %d",
      msg[offset], BOOL_FALSE_T, offset);
  offset++;
  /* The bool type already contains the value */

  /* Marshall and verify BOOL TRUE */
  oml_value_set_type(&v, OML_BOOL_VALUE);
  omlc_set_bool(*oml_value_get_value(&v), btrue);
  marshal_values(mbuf, &v, 1);
  count++;
  fail_unless(msg[5] == count,
      "Number of elements not set properly; got %d instead of %d",
      msg[5], count);
  fail_unless(msg[offset] == BOOL_TRUE_T,
      "bool type not set properly; got %d instead of %d at offset %d",
      msg[offset], BOOL_TRUE_T, offset);
  offset++;
  /* The bool type already contains the value */

  fail_unless(marshal_finalize(mbuf) == 1);

  dumpmessage(mbuf);

  fail_unless((msg[2] == OMB_DATA_P && mbuf_message_length(mbuf)<=UINT16_MAX) ||
      ((msg[2] == OMB_LDATA_P && mbuf_message_length(mbuf)>UINT16_MAX)),
      "Message type not properly adjusted");
  /* XXX: The following assumes OMB_DATA_P and will fail for OMB_LDATA_P;
   * need to add 2 to PACKET_HEADER_SIZE in that case */
  p = &msg[3];
  fail_unless(ntohs(*(uint16_t*)p) == (mbuf_message_length(mbuf) - PACKET_HEADER_SIZE),
      "Message length not set properly; got %d instead of %d at offset 3",
      ntohs(*(uint16_t*)p), (mbuf_message_length(mbuf) - PACKET_HEADER_SIZE));

  /*
   * UNMARSHALLING STARTS HERE
   */
  fail_unless(unmarshal_init(mbuf, &h) == 1);
  fail_unless(h.length == (mbuf_message_length(mbuf) - PACKET_HEADER_SIZE),
      "Message length not retrieved properly; got %d instead of %d at offset 3",
      h.length, (mbuf_message_length(mbuf) - PACKET_HEADER_SIZE));
  fail_unless(h.values == count,
      "Number of elements not retrieved properly; got %d instead of %d",
      h.values, count);
  fail_unless(h.stream == 1,
      "Stream number not set properly; got %d instead of 1 ",
      h.stream);
  fail_unless(h.seqno == 2,
      "Seqno not set properly; got %d instead of 2 ",
      h.seqno);
  fail_unless(h.timestamp - 3. < EPSILON,
      "Timestamp not set properly; got =%f instead of 3",
      h.timestamp);

  /* Unmarshal everything into dynamically allocated memory (va) */
  count = unmarshal_values(mbuf, &h, NULL, 0);
  fail_unless(count == -h.values,
      "unmarshal_values() with no storage did not report the right needed space; %d instead of %d",
      count, -h.values);
  va = malloc(h.values*sizeof(OmlValue));
  count = unmarshal_values(mbuf, &h, va, h.values);
  fail_unless(count == h.values,
      "unmarshal_values() did not return the expected success value; %d instead of %d",
      count, h.values);
  offset = 0;

  /* Verify unmarshalled INT32 */
  fail_unless(oml_value_get_type(&va[offset]) == OML_INT32_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_INT32_VALUE));
  fail_unless(omlc_get_int32(*oml_value_get_value(&va[offset])) == d32,
      "Read value at offset %d: got %" PRId32 " instead of %s",
      offset, omlc_get_int32(*oml_value_get_value(&va[offset])), d32);
  offset++;

  /* Verify unmarshalled UINT32 */
  fail_unless(oml_value_get_type(&va[offset]) == OML_UINT32_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_UINT32_VALUE));
  fail_unless(omlc_get_uint32(*oml_value_get_value(&va[offset])) == u32,
      "Read value at offset %d: got %" PRIu32 " instead of %s",
      offset, omlc_get_uint32(*oml_value_get_value(&va[offset])), u32);
  offset++;

  /* Verify unmarshalled INT64 */
  fail_unless(oml_value_get_type(&va[offset]) == OML_INT64_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_INT64_VALUE));
  fail_unless(omlc_get_int64(*oml_value_get_value(&va[offset])) == d64,
      "Read value at offset %d: got %" PRId64 " instead of %" PRId64 "",
      offset, omlc_get_int64(*oml_value_get_value(&va[offset])), d64);
  offset++;

  /* Verify unmarshalled UINT64 */
  fail_unless(oml_value_get_type(&va[offset]) == OML_UINT64_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_UINT64_VALUE));
  fail_unless(omlc_get_uint64(*oml_value_get_value(&va[offset])) == u64,
      "Read value at offset %d: got %" PRIu64 " instead of %" PRIu64 "",
      offset, omlc_get_uint64(*oml_value_get_value(&va[offset])), u64);
  offset++;

  /* Verify unmarshalled LONG (marshalled as OML_INT32_VALUE) */
  fail_unless(oml_value_get_type(&va[offset]) == OML_INT32_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_INT32_VALUE));
  fail_unless(omlc_get_int32(*oml_value_get_value(&va[offset])) == l,
      "Read value at offset %d: got %" PRId32 " instead of %" PRId32 "",
      offset, omlc_get_int32(*oml_value_get_value(&va[offset])), l);
  offset++;

  /* Verify unmarshalled DOUBLE */
  fail_unless(oml_value_get_type(&va[offset]) == OML_DOUBLE_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_DOUBLE_VALUE));
  fail_unless(omlc_get_double(*oml_value_get_value(&va[offset])) - d < EPSILON,
      "Read value at offset %d: got %f instead of %f",
      offset, omlc_get_double(*oml_value_get_value(&va[offset])), d);
  offset++;

  /* Verify unmarshalled STRING */
  fail_unless(oml_value_get_type(&va[offset]) == OML_STRING_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_STRING_VALUE));
  fail_unless(omlc_get_string_length(*oml_value_get_value(&va[offset])) == len,
      "Read string at offset %d: got invalid length  %d instead of %d",
      offset, omlc_get_string_length(*oml_value_get_value(&va[offset])), len);
  fail_if(strncmp(omlc_get_string_ptr(*oml_value_get_value(&va[offset])), s, len),
      "Read string at offset %d: mismatch",
      offset);
  offset++;

  /* Verify unmarshalled BLOB */
  fail_unless(oml_value_get_type(&va[offset]) == OML_BLOB_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_BLOB_VALUE));
  fail_unless(omlc_get_blob_length(*oml_value_get_value(&va[offset])) == len,
      "Read blob at offset %d: got invalid length  %d instead of %d",
      offset, omlc_get_blob_length(*oml_value_get_value(&va[offset])), len);
  fail_if(strncmp(omlc_get_blob_ptr(*oml_value_get_value(&va[offset])), b, len),
      "Read blob at offset %d: mismatch",
      offset);
  offset++;

  /* Verify unmarshalled GUID */
  fail_unless(oml_value_get_type(&va[offset]) == OML_GUID_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_GUID_VALUE));
  fail_unless(omlc_get_guid(*oml_value_get_value(&va[offset])) == guid,
      "Read value at offset %d: got %" PRIu64 " instead of %" PRIu64 "",
      offset, omlc_get_guid(*oml_value_get_value(&va[offset])), (uint64_t)guid);
  offset++;

  /* Verify unmarshalled BOOL FALSE */
  fail_unless(oml_value_get_type(&va[offset]) == OML_BOOL_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_BOOL_VALUE));
  bval = omlc_get_bool(*oml_value_get_value(&va[offset]));
  fail_unless(!bval && !bfalse,
      "Read value at offset %d: got truth value %" PRIu8 " instead of %" PRIu8 "",
      offset, bval, bfalse);
  offset++;

  /* Verify unmarshalled BOOL TRUE */
  fail_unless(oml_value_get_type(&va[offset]) == OML_BOOL_VALUE,
      "Read value at offset %d: got invalid type %s instead of %s",
      offset, oml_type_to_s(oml_value_get_type(&va[offset])), oml_type_to_s(OML_BOOL_VALUE));
  bval = omlc_get_bool(*oml_value_get_value(&va[offset]));
  fail_unless(bval && btrue,
      "Read value at offset %d: got truth value %" PRIu8 " instead of %" PRIu8 "",
      offset, bval, btrue);
  offset++;

  free(va);
  mbuf_destroy(mbuf);
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
  tcase_add_loop_test (tc_marshal, test_marshal_guid,         0, LENGTH (guid_values));
  tcase_add_loop_test (tc_marshal, test_marshal_bool,         0, LENGTH (bool_values));

  tcase_add_test (tc_marshal, test_marshal_unmarshal_long);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_int32);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_uint32);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_int64);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_uint64);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_double);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_string);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_guid);
  tcase_add_test (tc_marshal, test_marshal_unmarshal_bool);

  /* Do the full marshalling/unmarshalling test, types above should also be tested there */
  tcase_add_test (tc_marshal, test_marshal_full);

  suite_add_tcase (s, tc_marshal);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
