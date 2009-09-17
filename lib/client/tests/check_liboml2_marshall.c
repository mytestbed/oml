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
/*
 * Copyright (c) 2006-2009 National ICT Australia (NICTA), Australia
 *
 * Copyright (c) 2004-2009 WINLAB, Rutgers University, USA
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
 */#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <arpa/inet.h>
#include <check.h>
#include <marshall.h>

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
  v.longValue = 42;
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
  v.doubleValue = 42.73;
  int iexp;
  double dmant = frexp (v.doubleValue, &iexp) * (1 << 30);
  uint32_t imant = (uint32_t)dmant;

  int result = marshall_value (&mbuf, OML_DOUBLE_VALUE, &v);

  uint8_t type = mbuf.buffer[5];
  uint32_t nv = 0;
  memcpy (&nv, &mbuf.buffer[6], 4);
  uint8_t exp = mbuf.buffer[10];
  uint32_t hv = ntohl (nv);
  int mant = (int)hv;

  double val = ldexp (mant * 1.0 / (1 << 30), exp);

  fail_if (result != 1);
  fail_if (type != 0x2); // DOUBLE_T
  fail_if (mant != imant);
  fail_if (exp != iexp);
  fail_if (fabs(val -  v.doubleValue) < 1e-9);
}
END_TEST

START_TEST (test_marshall_value_string)
{
  OmlMBuffer mbuf;
  memset (&mbuf, 0, sizeof (OmlMBuffer));

  OmlMBuffer* pmbuf = marshall_init (&mbuf, OMB_DATA_P);

  fail_if (&mbuf != pmbuf);
  fail_if (mbuf.buffer == NULL);

  OmlValueU v;
  v.stringValue.ptr = "abcd";
  v.stringValue.is_const = 1;
  v.stringValue.size = 4;
  v.stringValue.length = 5;

  int result = marshall_value (&mbuf, OML_STRING_VALUE, &v);

  fail_if (result != 1);
  fail_if (mbuf.buffer[5] != 0x4); // STRING_T
  fail_if (mbuf.buffer[6] != strlen (v.stringValue.ptr)); // Length of string
  fail_if (strcmp ((char*)&mbuf.buffer[7], v.stringValue.ptr) != 0); // FIXME:  Packet won't include terminating '\0'???
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
  tcase_add_test (tc_marshall, test_marshall_value_long);
  tcase_add_test (tc_marshall, test_marshall_value_double);
  tcase_add_test (tc_marshall, test_marshall_value_string);

  suite_add_tcase (s, tc_marshall);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
