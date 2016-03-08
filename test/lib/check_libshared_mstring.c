/*
 * Copyright 2013-2016 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_libshared_mstring.c
 * \brief Test MString support
 */
#include <check.h>

#include "ocomm/o_log.h"
#include "mstring.h"

START_TEST (test_mstring)
{
  int i;
  size_t size;
  MString *mstr = NULL;

  o_set_log_level(2);

  fail_unless(mstring_set(mstr, "a") < 0, "mstring_set accepted a NULL MString");
  fail_unless(mstring_cat(mstr, "a") < 0, "mstring_cat accepted a NULL MString");
  fail_unless(mstring_sprintf(mstr, "a") < 0, "mstring_sprintf accepted a NULL MString");

  mstr = mstring_create();
  fail_if(mstr == NULL, "mstring_create returned a NULL MString");
  fail_if(mstr->buf <= 0, "mstring_create returned an MString with a NULL buffer");
  fail_if(mstr->size <= 0, "mstring_create returned an MString of size %d", mstr->size);
  fail_unless(mstr->length == 0, "mstring_create returned an MString of non-zero length %d", mstr->length);

  fail_unless(mstring_set(mstr, NULL) < 0, "mstring_set accepted a NULL string");
  fail_unless(mstring_cat(mstr, NULL) < 0, "mstring_cat accepted a NULL string");
  fail_unless(mstring_sprintf(mstr, NULL) < 0, "mstring_sprintf accepted a NULL format string");

  fail_if(mstring_set(mstr, "a") < 0, "mstring_set refused a valid instruction");
  fail_unless(mstring_len(mstr) == 1, "mstring_set didn't set MString length correctly");
  fail_if(strcmp(mstring_buf(mstr), "a"), "mstring_set didn't set MString contents correctly");

  fail_if(mstring_cat(mstr, "a") < 0, "mstring_cat refused a valid instruction");
  fail_unless(mstring_len(mstr) == 2, "mstring_cat didn't set MString length correctly");
  fail_if(strcmp(mstring_buf(mstr), "aa"), "mstring_cat didn't set MString contents correctly");

  fail_if(mstring_sprintf(mstr, "b%c", 'c') < 0, "mstring_sprintf refused a valid instruction");
  fail_unless(mstring_len(mstr) == 4, "mstring_sprintf didn't set MString length correctly");
  fail_if(strcmp(mstring_buf(mstr), "aabc"), "mstring_sprintf didn't set MString contents correctly");

  /* DEFAULT_MSTRING_SIZE is 64 in lib/shared/mstring.c */
  size = mstr->size;
  for (i=0; i<60; i++) {
    fail_if(mstring_sprintf(mstr, "%c", (char)(i+33)) < 0, "mstring_sprintf refused a valid instruction (%d)", i);
  }

  fail_unless(mstring_buf(mstr) == mstr->buf, "mstring_buf didn't return the right pointer");
  fail_unless(mstring_len(mstr) == mstr->length, "mstring_len didn't return the right length");

  for (; i<96; i++) {
    fail_if(mstring_sprintf(mstr, "%c", (char)(i+33)) < 0, "mstring_sprintf refused a valid instruction past its initial size (%d)", i);
  }
  fail_if(mstr->size == size, "mstring_sprintf didn't adjust size properly (still %d)", size);
  fail_if(mstr->size < mstr->length, "mstring_sprintf didn't adjust size properly (%d < %d)", mstr->size, mstr->length);

  mstring_delete(mstr);
}
END_TEST

Suite* mstring_suite (void)
{
  Suite* s = suite_create ("MString");

  TCase* tc_mstring = tcase_create ("MString");

  tcase_add_test (tc_mstring, test_mstring);

  suite_add_tcase (s, tc_mstring);

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
