/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
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
#include <check.h>
#include <htonll.h>
#include "check_util.h"

START_TEST (test_bswap_16)
{
  struct
  {
    uint16_t in;
    uint16_t ref;
    uint16_t out;
  } vector [] =
      {
        { 0x1234, 0x3412, 0 },
        { 0x0, 0x0, 0 },
        { 0xFFFF, 0xFFFF, 0 },
        { 0x8000, 0x0080, 0 }
      };
  int i = 0;
  for (i = 0; i < LENGTH(vector); i++)
    {
      vector[i].out = bswap_16(vector[i].in);
      fail_unless (vector[i].out == vector[i].ref);
    }
}
END_TEST

START_TEST (test_bswap_32)
{
  struct
  {
    uint32_t in;
    uint32_t ref;
    uint32_t out;
  } vector [] =
      {
        { 0x12345678, 0x78563412, 0 },
        { 0x0, 0x0, 0 },
        { 0xFFFF, 0xFFFF0000, 0 },
        { 0xFFFF0000, 0x0000FFFF, 0 },
        { 0x8000, 0x00800000, 0 },
        { 0x80000000, 0x00000080, 0 }
      };
  int i = 0;
  for (i = 0; i < LENGTH(vector); i++)
    {
      vector[i].out = bswap_32(vector[i].in);
      fail_unless (vector[i].out == vector[i].ref);
    }
}
END_TEST

START_TEST (test_bswap_64)
{
  struct
  {
    uint64_t in;
    uint64_t ref;
    uint64_t out;
  } vector [] =
      {
        { 0x123456789ABCDEF1LL, 0xF1DEBC9A78563412LL, 0 },
        { 0x0, 0x0, 0 },
        { 0xFFFFFFFFFFFFFFFFLL, 0xFFFFFFFFFFFFFFFFLL, 0 },
        { 0xFFFF, 0xFFFF000000000000LL, 0 },
        { 0x8000, 0x0080000000000000LL, 0 },
        { 0x8000, 0x0080000000000000LL, 0 },
        { 0x0000000080000000LL, 0x0000008000000000LL, 0 }
      };
  int i = 0;
  for (i = 0; i < LENGTH(vector); i++)
    {
      vector[i].out = bswap_64(vector[i].in);
      fail_unless (vector[i].out == vector[i].ref);
    }
}
END_TEST

Suite*
bswap_suite (void)
{
  Suite *s = suite_create("Bswap");

  /* Bswap test cases */
  TCase *tc_bswap = tcase_create ("Bswap");

  /* Add tests to the test case "Bswap" */
  tcase_add_test (tc_bswap, test_bswap_16);
  tcase_add_test (tc_bswap, test_bswap_32);
  tcase_add_test (tc_bswap, test_bswap_64);

  suite_add_tcase (s, tc_bswap);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
