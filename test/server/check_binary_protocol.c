/*
 * Copyright 2012 National ICT Australia (NICTA), Australia
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
/** \file Tests behaviours and issues related to the binary protocol. */
#include <stdint.h>
#include <check.h>

#include "mbuf.h"
#include "marshal.h"
#include "binary.h"

START_TEST(test_find_sync)
{
  uint8_t data[] = { 0xaa, 0xaa, 0x1, 0xaa, 0xaa, 0x2, };

  fail_unless(find_sync(data, 0) == NULL);
  fail_unless(find_sync(data, 1) == NULL);
  fail_unless(find_sync(data, sizeof(data)) == data);
  fail_unless(find_sync(data+1, sizeof(data)-1) == data+3);
  fail_unless(find_sync(data+3, sizeof(data)-3) == data+3);
  fail_unless(find_sync(data+4, sizeof(data)-4) == NULL);
  fail_unless(find_sync(data+5, sizeof(data)-5) == NULL);
}
END_TEST

START_TEST(test_bin_find_sync)
{
  MBuffer *mbuf = mbuf_create();
  uint8_t data[] = { 0xaa, 0xaa, 0x1, 0xaa, 0xaa, 0x2, };

  mbuf_write(mbuf, data, sizeof(data));

  fail_unless(bin_find_sync(mbuf) == 0);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == 2);
  fail_unless(bin_find_sync(mbuf) == 0);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == -1);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == -1);

  mbuf_destroy(mbuf);
}
END_TEST

Suite* binary_protocol_suite (void)
{
  Suite* s = suite_create ("Binary protocol");

  TCase *tc_bin_sync = tcase_create ("Sync");

  tcase_add_test (tc_bin_sync, test_find_sync);
  tcase_add_test (tc_bin_sync, test_bin_find_sync);
  suite_add_tcase (s, tc_bin_sync);

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
