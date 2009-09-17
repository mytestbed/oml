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
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <arpa/inet.h>
#include <check.h>
#include <marshall.h>

#include "util.h"

#define EPSILON 1e-8

#define MAX_MARSHALLED_STRING_LENGTH 254

static const int LONG_T = 0x1;  // marshall.c LONG_T
static const int DOUBLE_T = 0x2;  // marshall.c DOUBLE_T
static const int STRING_T = 0x4;  // marshall.c STRING_T

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

static int32_t long_values [] =
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

	/* 255 bytes = (15 * 16 + 14) */
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

START_TEST (test_marshall_init)
{
  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));

  OmlMBuffer* result = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != result);
  fail_if (mbuf.buffer == NULL);
  fail_if (mbuf.buffer_length != 64);
  fail_unless (mbuf.buffer[0] == 0xAA);
  fail_unless (mbuf.buffer[1] == 0xAA);
  fail_unless ((int)mbuf.buffer[2] == OMB_DATA_P);
  fail_unless (mbuf.curr_p == mbuf.buffer + 5);
  fail_unless (mbuf.buffer_remaining == mbuf.buffer_length - (mbuf.curr_p - mbuf.buffer));
}
END_TEST

START_TEST (test_marshall_value_long)
{
  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));

  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  OmlValueU v;
  v.longValue = long_values[_i];
  int result = marshall_value (&mbuf, OML_LONG_VALUE, &v);

  uint32_t nv = 0;
  memcpy (&nv, mbuf.buffer + 6, 4);
  uint32_t hv = ntohl (nv);
  int val = (int)hv;

  fail_if (result != 1);
  fail_if ((int)*(mbuf.buffer + 5) != LONG_T); // LONG_T
  fail_if (val != v.longValue);
}
END_TEST

START_TEST (test_marshall_value_double)
{
  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));

  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  OmlValueU v;
  v.doubleValue = double_values[_i];
  int iexp;
  double dmant = frexp (v.doubleValue, &iexp) * (1 << 30);
  uint32_t imant = (uint32_t)dmant;

  int result = marshall_value (&mbuf, OML_DOUBLE_VALUE, &v);

  uint8_t type = mbuf.buffer[5];
  uint32_t nv = 0;
  memcpy (&nv, &mbuf.buffer[6], 4);
  int8_t exp = mbuf.buffer[10];
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

START_TEST (test_marshall_value_string)
{
  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));
  marshall_resize(&mbuf, LENGTH (string_buf));

  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  OmlValueU v;
  char* test_string = string_values[_i];
  int len = strlen (test_string);
  v.stringValue.ptr = test_string;
  v.stringValue.is_const = 1;
  v.stringValue.size = len;
  v.stringValue.length = len + 1; // Underlying storage.

  int result = marshall_value (&mbuf, OML_STRING_VALUE, &v);

  fail_if (result != 1);
  fail_if (mbuf.buffer[5] != 0x4); // STRING_T

  memset (string_buf, 0, MAX_MARSHALLED_STRING_LENGTH);
  memcpy (string_buf, &mbuf.buffer[7], mbuf.buffer[6]);

  if (len <= MAX_MARSHALLED_STRING_LENGTH)
	{
	  fail_if (mbuf.buffer[6] != len); // Length of string
	  fail_if (strlen(string_buf) != len);
	  fail_if (strcmp (string_buf, test_string) != 0);
	  // FIXME:  Check that the buffer is zero past the last element of the test string.
	}
  else
	{
	  fail_if (mbuf.buffer[6] != MAX_MARSHALLED_STRING_LENGTH); // Length of string
	  fail_if (strlen(string_buf) != MAX_MARSHALLED_STRING_LENGTH);
	  fail_if (strncmp (string_buf, test_string, MAX_MARSHALLED_STRING_LENGTH) != 0);
	  // FIXME:  Check that the buffer doesn't contain more elements of the test string than it should
	}
}
END_TEST

START_TEST (test_marshall_unmarshall_long)
{
  const int VALUES_OFFSET = 5;
  const int UINT32_LENGTH = 5;
  const int UINT32_TYPE_OFFSET = 0;
  const int UINT32_VALUE_OFFSET = 1;
  const int UINT32_SIZE = sizeof (uint32_t);

  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));
  marshall_resize (&mbuf, 2 * LENGTH(long_values) * UINT32_LENGTH);
  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  int i = 0;
  for (i = 0; i < LENGTH (long_values); i++)
	{
	  OmlValueU v;
	  v.longValue = long_values[i];
	  int result = marshall_value (&mbuf, OML_LONG_VALUE, &v);

	  uint8_t* buf = &mbuf.buffer[VALUES_OFFSET + i * UINT32_LENGTH];
	  int type =  buf[UINT32_TYPE_OFFSET];
	  uint8_t* valptr = &buf[UINT32_VALUE_OFFSET];

	  uint32_t nv = 0;
	  memcpy (&nv, valptr, UINT32_SIZE);
	  uint32_t hv = ntohl (nv);
	  int val = (int)hv;

	  fail_if (result != 1);
	  fail_if (type != LONG_T); // LONG_T
	  fail_if (val != v.longValue);
	}

  mbuf.buffer_fill = VALUES_OFFSET + i * UINT32_LENGTH;
  mbuf.curr_p = mbuf.buffer;

  OmlMsgType msg_type = 0;
  marshall_finalize (&mbuf);
  unmarshall_init (&mbuf, &msg_type);

  mbuf.curr_p = mbuf.buffer + VALUES_OFFSET; // Kludge!

  fail_unless (msg_type == OMB_DATA_P);

  for (i = 0; i < LENGTH (long_values); i++)
	{
	  OmlValue value;

	  unmarshall_value (&mbuf, &value);

	  fail_unless (value.type == OML_LONG_VALUE);
	  fail_unless (value.value.longValue == long_values[i],
				   "Unmarshalled value %ld, expected %ld\n",
				   value.value.longValue, long_values[i]);
	}
}
END_TEST

START_TEST (test_marshall_unmarshall_double)
{
  const int VALUES_OFFSET = 5;
  const int DOUBLE_LENGTH = 6;
  const int DOUBLE_TYPE_OFFSET = 0;
  const int DOUBLE_MANT_OFFSET = 1;
  const int DOUBLE_EXP_OFFSET = 5;
  const int DOUBLE_MANT_SIZE = sizeof (int32_t);
  const int DOUBLE_EXP_SIZE = sizeof (int8_t);

  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));
  marshall_resize (&mbuf, 2 * LENGTH(long_values) * DOUBLE_LENGTH);
  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  int i = 0;
  for (i = 0; i < LENGTH (double_values); i++)
	{
	  OmlValueU v;
	  v.doubleValue = double_values[i];
	  int result = marshall_value (&mbuf, OML_DOUBLE_VALUE, &v);

	  uint8_t* buf = &mbuf.buffer[VALUES_OFFSET + i * DOUBLE_LENGTH];
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

  mbuf.buffer_fill = VALUES_OFFSET + i * DOUBLE_LENGTH;
  mbuf.curr_p = mbuf.buffer;

  OmlMsgType msg_type = 0;
  marshall_finalize (&mbuf);
  unmarshall_init (&mbuf, &msg_type);

  mbuf.curr_p = mbuf.buffer + VALUES_OFFSET; // Kludge!

  fail_unless (msg_type == OMB_DATA_P);

  for (i = 0; i < LENGTH (double_values); i++)
	{
	  OmlValue value;

	  unmarshall_value (&mbuf, &value);

	  fail_unless (value.type == OML_DOUBLE_VALUE);
	  fail_unless (relative_error (value.value.doubleValue, double_values[i]) < EPSILON,
				   "Unmarshalled value %g, expected %g\n",
				   value.value.doubleValue, double_values[i]);
	}
}
END_TEST

START_TEST (test_marshall_unmarshall_string)
{
  const int VALUES_OFFSET = 5;
  const int STRING_TYPE_OFFSET = 0;
  const int STRING_LENGTH_OFFSET = 1;
  const int STRING_VALUE_OFFSET = 2;
  char string[MAX_MARSHALLED_STRING_LENGTH + 16];

  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));
  marshall_resize (&mbuf, 2 * LENGTH(string_values) * MAX_MARSHALLED_STRING_LENGTH);
  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  uint8_t* curr_p = &mbuf.buffer[VALUES_OFFSET];

  int i = 0;
  for (i = 0; i < LENGTH (string_values); i++)
	{
	  memset (string, 0, LENGTH(string));
	  OmlValueU v;
	  v.stringValue.ptr = string_values[i];
	  v.stringValue.is_const = 1;
	  v.stringValue.size = strlen (string_values[i]);
	  v.stringValue.length = v.stringValue.size + 1; // Underlying storage.

	  int result = marshall_value (&mbuf, OML_STRING_VALUE, &v);

	  uint8_t* buf = curr_p;
	  int type =  buf[STRING_TYPE_OFFSET];
	  uint8_t* lenptr = &buf[STRING_LENGTH_OFFSET];
	  uint8_t* valptr = &buf[STRING_VALUE_OFFSET];
	  int len = *lenptr;

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

	  curr_p += len + 2;
	}

  mbuf.buffer_fill = curr_p - mbuf.buffer;
  mbuf.curr_p = mbuf.buffer;

  OmlMsgType msg_type = 0;
  marshall_finalize (&mbuf);
  unmarshall_init (&mbuf, &msg_type);

  mbuf.curr_p = mbuf.buffer + VALUES_OFFSET; // Kludge!

  fail_unless (msg_type == OMB_DATA_P);

  for (i = 0; i < LENGTH (string_values); i++)
	{
	  OmlValue value;

	  unmarshall_value (&mbuf, &value);

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
marshall_suite (void)
{
  Suite* s = suite_create ("Marshall");

  /* Marshalling test cases */
  TCase* tc_marshall = tcase_create ("Marshall");

  /* Add tests to "Marshall" */
  tcase_add_test (tc_marshall, test_marshall_init);
  tcase_add_loop_test (tc_marshall, test_marshall_value_long,   0, LENGTH (long_values));
  tcase_add_loop_test (tc_marshall, test_marshall_value_double, 0, LENGTH (double_values));
  tcase_add_loop_test (tc_marshall, test_marshall_value_string, 0, LENGTH (string_values));

  tcase_add_test (tc_marshall, test_marshall_unmarshall_long);
  tcase_add_test (tc_marshall, test_marshall_unmarshall_double);
  tcase_add_test (tc_marshall, test_marshall_unmarshall_string);

  suite_add_tcase (s, tc_marshall);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
