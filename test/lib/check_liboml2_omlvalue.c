/*
 * Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_liboml2_omlvalue.c
 * \brief Test OmlValue[UT] low-level manipulation functions
 */
#include <check.h>
#include <math.h>
#include <stdint.h>
#include <inttypes.h>

#include "oml2/omlc.h"
#include "mem.h"
#include "oml_util.h"
#include "oml_value.h"

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

START_TEST (test_stringU)
{
  OmlValueU v, v2;
  char *test = "test";
  char *const_test = "const test";
  size_t size, alloc_diff, bcount = xmembytes();

  omlc_zero(v);
  omlc_zero(v2);

  /* Set pointer */
  omlc_set_string(v, test);
  fail_unless(omlc_get_string_ptr(v) == test,
      "Test string pointer not copied properly");
  fail_unless(omlc_get_string_length(v) == strlen(test),
      "Test string length not set properly (%d instead of %d)",
      omlc_get_string_length(v), strlen(test));
  fail_unless(xmembytes() == bcount,
      "Test string shouldn't have allocated memory, but the allocation increased");
  fail_unless(omlc_get_string_size(v) == 0,
      "Test string allocated size not set properly (%d instead of 0)",
      omlc_get_string_size(v));
  fail_unless(omlc_get_string_is_const(v) == 0,
      "Test string should not be constant");

  /* Set const pointer */
  omlc_set_const_string(v, const_test);
  fail_unless(omlc_get_string_ptr(v) == const_test,
      "Const test string pointer not copied properly");
  fail_unless(omlc_get_string_length(v) == strlen(const_test),
      "Const test string length not set properly (%d instead of %d)",
      omlc_get_string_length(v), strlen(const_test));
  fail_unless(xmembytes() == bcount,
      "Const test string shouldn't have allocated memory, but the allocation increased");
  fail_unless(omlc_get_string_size(v) == 0,
      "Const test string allocated size not set properly (%d instead of 0)",
      omlc_get_string_size(v));
  fail_if(omlc_get_string_is_const(v) == 0,
      "Const test string should be constant");

  /* Duplicate pointer */
  omlc_set_string_copy(v, test, strlen(test));
  fail_if(omlc_get_string_ptr(v) == test,
      "Copied test string pointer not allocated properly");
  fail_if(strcmp(test, omlc_get_string_ptr(v)),
      "Copied test string mismatch ('%s' instead of '%s')",
      omlc_get_string_ptr(v), test);
  fail_unless(omlc_get_string_length(v) == strlen(test),
      "Copied test string length not set properly (%d instead of %d)",
      omlc_get_string_length(v), strlen(test));
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < strlen(test) + 1,
      "Copied test string allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, strlen(test) + 1);
  fail_unless(omlc_get_string_size(v) == xmalloc_usable_size(omlc_get_string_ptr(v)),
      "Copied test string allocated size not set properly (%d instead of %d)",
      omlc_get_string_size(v), xmalloc_usable_size(omlc_get_string_ptr(v)));
  fail_unless(omlc_get_string_is_const(v) == 0,
      "Copied test string should not be constant");

  /* Copy allocated pointer */
  omlc_copy_string(v2, v);
  fail_if(omlc_get_string_ptr(v2) == omlc_get_string_ptr(v),
      "Copied allocated string pointer not allocated properly");
  fail_if(strcmp(omlc_get_string_ptr(v2), omlc_get_string_ptr(v)),
      "Copied allocated string mismatch ('%s' instead of '%s')",
      omlc_get_string_ptr(v2), omlc_get_string_ptr(v));
  fail_unless(omlc_get_string_length(v2) == strlen(omlc_get_string_ptr(v2)),
      "Copied allocated string length not set properly (%d instead of %d)",
      omlc_get_string_length(v2), strlen(omlc_get_string_ptr(v2)) + 1);
  alloc_diff = xmembytes() - bcount - sizeof(size_t); /* xmalloc() always allocates at least sizeof(size_t) more for bookkeeping */
  bcount = xmembytes();
  fail_if(alloc_diff < strlen(omlc_get_string_ptr(v)) + 1,
      "Copied allocated string allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, strlen(omlc_get_string_ptr(v)) + 1);
  fail_unless(omlc_get_string_size(v) == xmalloc_usable_size(omlc_get_string_ptr(v2)),
      "Copied allocated string allocated size not set properly (%d instead of %d)",
      omlc_get_string_size(v2), xmalloc_usable_size(omlc_get_string_ptr(v2)));
  fail_unless(omlc_get_string_is_const(v2) == 0,
      "Copied allocated string should not be constant");

  /* Set pointer on allocated pointer */
  size = omlc_get_string_size(v);
  omlc_set_string(v, test);
  fail_unless(omlc_get_string_ptr(v) == test,
      "Override test string pointer not copied properly");
  fail_unless(omlc_get_string_length(v) == strlen(test),
      "Override test string length not set properly (%d instead of %d)",
      omlc_get_string_length(v), strlen(test));
  alloc_diff = bcount - xmembytes() + sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff <= size,
      "Override test string over allocated memory test didn't free the memory as expected (%d freed instead at least %d)",
      alloc_diff, size);
  bcount = xmembytes();
  fail_unless(omlc_get_string_size(v) == 0,
      "Override test string allocated size not set properly (%d instead of 0)",
      omlc_get_string_size(v));
  fail_unless(omlc_get_string_is_const(v) == 0,
      "Override test string should not be constant");

  /* Reset string and clear allocated pointer */
  size = omlc_get_string_size(v2);
  omlc_reset_string(v2);
  fail_unless(omlc_get_string_ptr(v2) == NULL,
      "Reset allocated string pointer not cleared properly");
  fail_unless(omlc_get_string_length(v2) == 0,
      "Reset allocated string length not cleared properly");
  alloc_diff = bcount - xmembytes() + sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff <= size,
      "Reset allocated string didn't free the memory as expected (%d freed instead at least %d)",
      alloc_diff, size);
  fail_unless(omlc_get_string_size(v2) == 0,
      "Reset allocated string allocated size not cleared properly");
  fail_unless(omlc_get_string_is_const(v2) == 0,
      "Reset allocated string should not be constant");

  /* Copy const pointer */
  omlc_set_const_string(v2, const_test);
  omlc_copy_string(v, v2);
  fail_if(omlc_get_string_ptr(v2) == omlc_get_string_ptr(v),
      "Const copy string pointer not allocated properly");
  fail_if(strcmp(omlc_get_string_ptr(v2), omlc_get_string_ptr(v)),
      "Const copy string mismatch ('%s' instead of '%s')",
      omlc_get_string_ptr(v2), omlc_get_string_ptr(v));
  fail_unless(omlc_get_string_length(v) == strlen(const_test),
      "Const copy string length not set properly (%d instead of %d)",
      omlc_get_string_length(v), strlen(const_test));
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < strlen(const_test) + 1,
      "Const copy string allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, strlen(const_test) + 1);
  fail_unless(omlc_get_string_size(v) == xmalloc_usable_size(omlc_get_string_ptr(v)),
      "Const copy string allocated size not set properly (%d instead of %d)",
      omlc_get_string_size(v), xmalloc_usable_size(omlc_get_string_ptr(v)));
  fail_unless(omlc_get_string_is_const(v) == 0,
      "Const copy string should not be constant");

  omlc_reset_string(v);
  omlc_reset_string(v2);
}
END_TEST

START_TEST (test_blobU)
{
  OmlValueU v, v2;
  char *str = "this is a string subtly disguised as a blob";
  char *str2 = "this is another string in blob's clothing, except longer";
  void *test = (void*) str;
  void *test2 = (void*) str2;
  size_t len = strlen(str);
  size_t len2 = strlen(str2);
  size_t bcount = xmembytes(), alloc_diff, size;

  omlc_zero(v);
  omlc_zero(v2);

  /* Set blob (copy) */
  omlc_set_blob(v, test, len);
  fail_if(omlc_get_blob_ptr(v) == test,
      "Test blob pointer not allocated properly");
  fail_if(strncmp(test, omlc_get_blob_ptr(v), len),
      "Test blob mismatch");
  fail_unless(omlc_get_blob_length(v) == len,
      "Test blob length not set properly (%d instead of %d)",
      omlc_get_blob_length(v), len);
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < len,
      "Test blob allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, len);
  fail_unless(omlc_get_blob_size(v) == xmalloc_usable_size(omlc_get_string_ptr(v)),
      "Test blob allocated size not set properly (%d instead of %d)",
      omlc_get_blob_size(v), xmalloc_usable_size(omlc_get_string_ptr(v)));

  /* Duplicate blob*/
  omlc_copy_blob(v2, v);
  fail_if(omlc_get_blob_ptr(v2) == omlc_get_blob_ptr(v),
      "Copied blob not allocated properly");
  fail_if(strncmp(omlc_get_blob_ptr(v2), omlc_get_blob_ptr(v), len),
      "Copied allocated blob mismatch");
  fail_unless(omlc_get_blob_length(v2) == omlc_get_blob_length(v),
      "Copied allocated blob length not set properly (%d instead of %d)",
      omlc_get_blob_length(v2), omlc_get_blob_length(v));
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < len,
      "Copied allocated blob allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, len);
  fail_unless(omlc_get_blob_size(v) == xmalloc_usable_size(omlc_get_blob_ptr(v2)),
      "Copied allocated blob allocated size not set properly (%d instead of %d)",
      omlc_get_blob_size(v2), xmalloc_usable_size(omlc_get_blob_ptr(v2)));

  /* Overwrite blob */
  omlc_set_blob(v, test2, len2);
  /* We know omlc_set_blob() already sets properly, we just want to check it does cleanup previously allocated memory */
  fail_if(xmembytes() > bcount + omlc_get_blob_size(v),
      "Overwritten blob did not deallocate memory properly (%d used but expected %d)",
      xmembytes(), bcount + omlc_get_blob_size(v));

  /* Reset blob and clear allocated pointer */
  omlc_reset_blob(v);
  size = omlc_get_blob_size(v);
  omlc_reset_blob(v);
  fail_unless(omlc_get_blob_ptr(v) == NULL,
      "Reset allocated blob pointer not cleared properly");
  fail_unless(omlc_get_blob_length(v) == 0,
      "Reset allocated blob length not cleared properly");
  alloc_diff = bcount - xmembytes() + sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff <= size,
      "Reset allocated blob didn't free the memory as expected (%d freed instead at least %d)",
      alloc_diff, size);
  fail_unless(omlc_get_blob_size(v) == 0,
      "Reset allocated blob allocated size not cleared properly");

  omlc_reset_blob(v2);
}
END_TEST

#define test_i(type, var, format)                                         \
  do {                                                                    \
    omlc_set_ ## type(to, var);                                           \
    fail_unless(omlc_get_ ## type(to) == var,                             \
        "Error setting " #type ", expected %" format ", got %" format,    \
        var, omlc_get_ ## type(to));                                      \
    from = to;                                                            \
    fail_unless(omlc_get_ ## type(from) == var,                           \
        "Error copying" #type ", expected %" format ", got %" format,     \
        var, omlc_get_ ## type(to));                                      \
  } while(0)


START_TEST (test_intrinsic)
{
  OmlValueU to, from;
  int32_t i32 = -123234;
  uint32_t u32 = 128937;
  int64_t i64 = -123234892374;
  uint64_t u64 = 128939087987;
  double d = M_PI;

  omlc_zero(to);
  omlc_zero(from);

  test_i(int32, i32, "i");
  test_i(uint32, u32, "u");
  test_i(int64, i64, PRId64);
  test_i(uint64, u64, PRIu64);
  test_i(double, d, "f");
  test_i(bool, 0, "d");
  test_i(bool, 1, "d");
}
END_TEST

START_TEST (test_string)
{
  char *test = "test";
  OmlValue v, v2;
  OmlValueU vu;
  size_t bcount = xmembytes();
  size_t alloc_diff;

  oml_value_init(&v);
  oml_value_init(&v2);

  omlc_zero(vu);

  /* Prepare the OmlValueU to be duplicated in the OmlValue */
  omlc_set_const_string(vu, test);

  oml_value_set(&v, &vu, OML_STRING_VALUE);
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < strlen(test) + 1,
      "OmlValue string allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, strlen(test) + 1);

  oml_value_duplicate(&v2, &v);
  fail_if(omlc_get_string_ptr(*oml_value_get_value(&v2)) == omlc_get_string_ptr(*oml_value_get_value(&v)),
      "Copied OmlValue string pointer not allocated properly");
  fail_if(strcmp(omlc_get_string_ptr(*oml_value_get_value(&v2)), omlc_get_string_ptr(*oml_value_get_value(&v))),
      "Copied OmlValue string mismatch ('%s' instead of '%s')",
      omlc_get_string_ptr(*oml_value_get_value(&v2)), omlc_get_string_ptr(*oml_value_get_value(&v)));
  fail_unless(omlc_get_string_length(*oml_value_get_value(&v2)) == strlen(omlc_get_string_ptr(*oml_value_get_value(&v2))),
      "Copied OmlValue string length not set properly (%d instead of %d)",
      omlc_get_string_length(*oml_value_get_value(&v2)), strlen(omlc_get_string_ptr(*oml_value_get_value(&v2))) + 1);
  alloc_diff = xmembytes() - bcount - sizeof(size_t); /* xmalloc() always allocates at least sizeof(size_t) more for bookkeeping */
  bcount = xmembytes();
  fail_if(alloc_diff < strlen(omlc_get_string_ptr(*oml_value_get_value(&v))) + 1,
      "Copied OmlValue string allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, strlen(omlc_get_string_ptr(*oml_value_get_value(&v))) + 1);
  fail_unless(omlc_get_string_size(*oml_value_get_value(&v2)) == xmalloc_usable_size(omlc_get_string_ptr(*oml_value_get_value(&v2))),
      "Copied OmlValue string allocated size not set properly (%d instead of %d)",
      omlc_get_string_size(*oml_value_get_value(&v2)), xmalloc_usable_size(omlc_get_string_ptr(*oml_value_get_value(&v2))));
  fail_unless(omlc_get_string_is_const(*oml_value_get_value(&v2)) == 0,
      "Copied OmlValue string should not be constant");

  oml_value_set_type(&v, OML_UINT64_VALUE);
  fail_unless(xmembytes() < bcount,
      "OmlValue string  was not freed after oml_value_set_type() (%d allocated, which is not less than %d)",
      xmembytes(), bcount);
  bcount = xmembytes();

  oml_value_reset(&v2);
  fail_unless(xmembytes() < bcount,
      "OmlValue string  was not freed after oml_value_reset() (%d allocated, which is not less than %d)",
      xmembytes(), bcount);
  bcount = xmembytes();

  oml_value_reset(&v);
  omlc_reset_string(vu);
}
END_TEST

START_TEST (test_blob)
{
  char *str = "this is a string subtly disguised as a blob";
  OmlValue v, v2;
  OmlValueU vu;
  size_t len = strlen(str);
  void *test = (void*)str;
  size_t bcount;
  size_t alloc_diff;

  oml_value_init(&v);
  oml_value_init(&v2);

  omlc_zero(vu);

  /* Prepare the OmlValueU to be duplicated in the OmlValue */
  omlc_set_blob(vu, test, len);
  bcount = xmembytes();

  oml_value_set(&v, &vu, OML_BLOB_VALUE);
  alloc_diff = xmembytes() - bcount - sizeof(size_t);
  bcount = xmembytes();
  fail_if(alloc_diff < len,
      "OmlValue blob allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, len);

  oml_value_duplicate(&v2, &v);
  fail_if(omlc_get_blob_ptr(*oml_value_get_value(&v2)) == omlc_get_blob_ptr(*oml_value_get_value(&v)),
      "Copied OmlValue blob pointer not allocated properly");
  fail_if(strncmp(omlc_get_blob_ptr(*oml_value_get_value(&v2)), omlc_get_blob_ptr(*oml_value_get_value(&v)), len),
      "Copied OmlValue blob mismatch");
  fail_unless(omlc_get_blob_length(*oml_value_get_value(&v2)) == omlc_get_blob_length(*oml_value_get_value(&v)),
      "Copied OmlValue blob length not set properly (%d instead of %d)",
      omlc_get_blob_length(*oml_value_get_value(&v2)), omlc_get_blob_length(*oml_value_get_value(&v2)));
  alloc_diff = xmembytes() - bcount - sizeof(size_t); /* xmalloc() always allocates at least sizeof(size_t) more for bookkeeping */
  bcount = xmembytes();
  fail_if(alloc_diff < len,
      "Copied OmlValue blob allocated memory not big enough (%d instead of at least %d)",
      alloc_diff, len);
  fail_unless(omlc_get_blob_size(*oml_value_get_value(&v2)) == xmalloc_usable_size(omlc_get_blob_ptr(*oml_value_get_value(&v2))),
      "Copied OmlValue blob allocated size not set properly (%d instead of %d)",
      omlc_get_blob_size(*oml_value_get_value(&v2)), xmalloc_usable_size(omlc_get_blob_ptr(*oml_value_get_value(&v2))));

  oml_value_set_type(&v, OML_UINT64_VALUE);
  fail_unless(xmembytes() < bcount,
      "OmlValue blob  was not freed after oml_value_set_type() (%d allocated, which is not less than %d)",
      xmembytes(), bcount);
  bcount = xmembytes();

  oml_value_reset(&v2);
  fail_unless(xmembytes() < bcount,
      "OmlValue blob  was not freed after oml_value_reset() (%d allocated, which is not less than %d)",
      xmembytes(), bcount);
  bcount = xmembytes();

  oml_value_reset(&v);
  omlc_reset_blob(vu);
}
END_TEST

static struct {
  const char* str;
  uint8_t     b;
} booltest[] = {
  {NULL, 0},
  {"f", 0},
  {"fal", 0},
  {"FaLsE", 0},
  {"TrUE", 1},
  {"TrUisM", 1},
  {"fAlSI", 1},
  {"fALsEiTuDe", 1},
  {"wHaTeVeR", 1},
};

START_TEST(test_bool_loop)
{
  fail_unless(oml_value_string_to_bool(booltest[_i].str) == booltest[_i].b,
      "'%s' was not resolved as bool %d", booltest[_i].str?booltest[_i].str:"(nil)", booltest[_i].b);
}
END_TEST

Suite*
omlvalue_suite (void)
{
  Suite* s = suite_create ("OmlValue");

  TCase* tc_omlvalue = tcase_create ("OmlValue");
  tcase_add_test (tc_omlvalue, test_stringU);
  tcase_add_test (tc_omlvalue, test_blobU);
  tcase_add_test (tc_omlvalue, test_intrinsic);
  tcase_add_test (tc_omlvalue, test_string);
  tcase_add_test (tc_omlvalue, test_blob);
  tcase_add_loop_test (tc_omlvalue, test_bool_loop, 0, LENGTH(booltest));

  suite_add_tcase (s, tc_omlvalue);

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
