/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#define _GNU_SOURCE  /* For NAN */
#include <math.h>
#include <string.h>
#include <check.h>

#include "json.h"
#include "mem.h"

START_TEST(null_valued_vector_double_null_output)
{
  double *v = NULL;
  char *s = NULL;
  ssize_t n = vector_double_to_json(v, 0, &s);

  /* for a zero-sized vector we expect an empty string */
  ck_assert_int_eq(0, n);
  /* buffer must be allocated */
  ck_assert_ptr_ne(NULL, s);
  /* the empty vector should produce an empty string */
  ck_assert_uint_eq('\0', s[0]);
}
END_TEST

START_TEST(zero_sized_vector_double_null_output)
{
  double v[1];
  char *s = NULL;
  ssize_t n = vector_double_to_json(v, 0, &s);

  /* for a zero-sized vector we expect an empty string */
  ck_assert_int_eq(0, n);
  /* for a NULL string pointer we expect an output buffer to be allocated */
  ck_assert_ptr_ne(NULL, s);
  /* the empty vector should produce an empty string */
  ck_assert_uint_eq('\0', s[0]);
}
END_TEST

START_TEST(single_elt_vector_double_null_output)
{
  const double v[] = { 2.718281828 };
  char *s = NULL;
  ssize_t n = vector_double_to_json(v, sizeof(v)/sizeof(v[0]), &s);

  /* ensure it succeeded */
  ck_assert_int_ne(-1, n);
  ck_assert_ptr_ne(NULL, s);
  ck_assert_uint_eq('\0', s[n]);

  /* test output is as expected */
  const char *expected = "[ 2.718281828 ]";
  ck_assert_int_eq(strlen(expected), n);
  ck_assert_str_eq(expected, s);
}
END_TEST

START_TEST(zero_sized_vector_double_tiny_output)
{
  double v[1];
  char *s = oml_malloc(1);
  ssize_t n = vector_double_to_json(v, 0, &s);

  /* for a zero-sized vector we expect an empty string */
  ck_assert_int_eq(0, n);
  /* buffer must be allocated  */
  ck_assert_ptr_ne(NULL, s);
  /* the empty vector should produce an empty string */
  ck_assert_uint_eq('\0', s[0]);
}
END_TEST

START_TEST(null_valued_vector_double_tiny_output)
{
  double *v = NULL;
  char *s = oml_malloc(1);
  ssize_t n = vector_double_to_json(v, 0, &s);

  /* for a zero-sized vector we expect an empty string */
  ck_assert_int_eq(0, n);
  /* for a NULL string pointer we expect an output buffer to be allocated */
  ck_assert_ptr_ne(NULL, s);
  /* the empty vector should produce an empty string */
  ck_assert_uint_eq('\0', s[0]);
}
END_TEST

START_TEST(single_elt_vector_double_tiny_output)
{
  const double v[] = { 2.718281828 };
  char *s = oml_malloc(1);
  ssize_t n = vector_double_to_json(v, sizeof(v)/sizeof(v[0]), &s);
  /* ensure it succeeded */
  ck_assert_int_ne(-1, n);
  ck_assert_ptr_ne(NULL, s);
  ck_assert_uint_eq('\0', s[n]);

  /* test output is as expected */
  const char *expected = "[ 2.718281828 ]";
  ck_assert_int_eq(strlen(expected), n);
  ck_assert_ptr_ne(NULL, s);
  ck_assert_uint_eq('\0', s[n]);
  ck_assert_str_eq(expected, s);
}
END_TEST

START_TEST(vector_double_test_precision)
{
  const double v[] = {
    1.234567890123456,
    2.345678901234567,
    3.456789012345678
  };
  char *s = oml_malloc(1);
  ssize_t n = vector_double_to_json(v, sizeof(v)/sizeof(v[0]), &s);

  /* ensure it succeeded */
  ck_assert_int_ne(-1, n);
  ck_assert_ptr_ne(NULL, s);

  /* test output is as expected */
  const char *expected = "[ 1.23456789012346, 2.34567890123457, 3.45678901234568 ]";
  ck_assert_int_eq(strlen(expected), n);
  ck_assert_uint_eq('\0', s[n]);
  ck_assert_str_eq(expected, s);

}
END_TEST


Suite*
json_suite(void)
{
  Suite *s = suite_create("json");
  TCase *tc_core = tcase_create("json_tests");
  tcase_add_test(tc_core, null_valued_vector_double_null_output);
  tcase_add_test(tc_core, zero_sized_vector_double_null_output);
  tcase_add_test(tc_core, single_elt_vector_double_null_output);
  tcase_add_test(tc_core, null_valued_vector_double_tiny_output);
  tcase_add_test(tc_core, zero_sized_vector_double_tiny_output);
  tcase_add_test(tc_core, single_elt_vector_double_tiny_output);
  tcase_add_test(tc_core, vector_double_test_precision);
  suite_add_tcase(s, tc_core);
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
