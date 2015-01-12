/*
 * Copyright 2015 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_libshared_zlib_utils.c
 * Test the Zlib helpers.
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */
#include <check.h>

#include "check_utils.h"
#include "zlib_utils.h"
#include "ocomm/o_log.h"

struct {
  char  *str;
  int   len;
  off_t off;
} zlib_sync_tests[] = {
  /* Basic patterns */
  { (char[]){ 0x1f, 0x8b }, 2, 0 },
  { (char[]){ 0x00, 0x00, 0xff, 0xff }, 4, 0 },

  /* Offsetted patterns */
  { (char[]){ 0x00, 0x00, 0x1f, 0x8b }, 4, 2 },
  { (char[]){ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff }, 6, 2 },

  /* Combined patterns */
  { (char[]){ 0x00, 0x00, 0x1f, 0x8b, 0x00, 0x00, 0x00, 0x00, 0xff, 0xff  }, 10, 2 },
  { (char[]){ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff, 0x00, 0x00, 0x1f, 0x8b }, 10, 2 },

  /* Valid patterns beyond length */
  { (char[]){ 0x00, 0x00, 0x1f, 0x8b }, 2, -1 },
  { (char[]){ 0x00, 0x00, 0x00, 0x00, 0xff, 0xff }, 4, -1 },

};

START_TEST (test_zlib_find_sync_loop)
{
  off_t off = oml_zlib_find_sync(zlib_sync_tests[_i].str, zlib_sync_tests[_i].len);
  ck_assert_msg(zlib_sync_tests[_i].off == off, "Incorrect offset: expected %d, found %d",
        zlib_sync_tests[_i].off, off);
}
END_TEST

Suite*
zlib_utils_suite(void)
{
  Suite *s = suite_create("zlib_utils");
  TCase *tc_zlib = tcase_create("zlib_utils");
  tcase_add_loop_test (tc_zlib, test_zlib_find_sync_loop, 0, LENGTH(zlib_sync_tests));
  suite_add_tcase(s, tc_zlib);
  return s;
}

/*
  Local Variables:
  mode: C
  tab-width: 2
  indent-tabs-mode: nil
  vim: sw=2:sts=2:expandtab
*/
