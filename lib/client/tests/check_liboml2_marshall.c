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
  fail_if ((int)*(mbuf.buffer + 5) != 0x1); // LONG_T
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
  fail_if (type != 0x2); // DOUBLE_T
  fail_if (mant != imant, "Value %g:  mismatched mantissa, expected %d, got %d\n", double_values[_i], imant, mant);
  fail_if (exp != iexp, "Value %g:  mismatched exponent, expected %d, got %d\n", double_values[_i], iexp, exp);
  fail_if (fabs(val -  v.doubleValue)/v.doubleValue > EPSILON, "Value %g expected, recovered %f from the buffer, delta=%g\n", double_values[_i], val, double_values[_i] - val);
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

  suite_add_tcase (s, tc_marshall);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
