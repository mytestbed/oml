/*
 * Copyright 2007-2014 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#include <check.h>
#include <string.h>

#include "string_utils.h"

START_TEST (test_util_find)
{
  char ws[] = "   ";
  char ts[] = " abc def";
  char *tsnwf = ts + 1;
  char *tsnw = tsnwf + 4;
  const char *got;

  /* XXX: This should really be using a loop test */

  got=skip_white(ws);
  fail_unless(got == ws+sizeof(ws)-1,
      "exp: %p :'%s'; got: %p: '%s'", ws+sizeof(ws)-1, ws+sizeof(ws)-1, got, got);
  got=skip_white(ts);
  fail_unless(got == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  got=skip_white(tsnwf);
  fail_unless(got == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  got=skip_white(tsnw);
  fail_unless(got == tsnw,
      "exp: %p :'%s'; got: %p: '%s'", tsnw, tsnw, got, got);

  got=find_white(ts);
  fail_unless(got == ts,
      "exp: %p :'%s'; got: %p: '%s'", ts, ts, got, got);
  got=find_white(tsnwf);
  fail_unless(got == tsnw - 1,
      "exp: %p :'%s'; got: %p: '%s'", tsnw-1, tsnw-1, got, got);
  got=find_white(tsnw);
  fail_unless(got == tsnw+strlen(tsnw),
      "exp: %p :'%s'; got: %p: '%s'", tsnw+strlen(tsnw), tsnw+strlen(tsnw), got, got);

  got=find_charn(ts, 'a', sizeof(ts));
  fail_unless(got == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  got=find_charn(ts, 'z', sizeof(ts) + 10);
  fail_unless(got == NULL,
      "exp: %p :'%s'; got: %p: '%s'", NULL, NULL, got, got);
  got=find_charn(ts, 'a', 1);
  fail_unless(got == NULL,
      "exp: %p :'%s'; got: %p: '%s'", NULL, NULL, got, got);
}
END_TEST


START_TEST(test_round_trip)
{
  const size_t TEST_SZ = 128;
  char in[TEST_SZ+1], out[2*TEST_SZ+1];

  /* encode */

  strcpy(in, "");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 0);
  fail_unless('\0' == out[0]);

  strcpy(in, "a");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 1);
  fail_unless(strcmp(in, out) == 0);

  strcpy(in, "\t");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 2);
  fail_unless(strcmp("\\t", out) == 0);

  strcpy(in, "\\");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 2);
  fail_unless(strcmp("\\\\", out) == 0);

  strcpy(in, "\\x");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 3);
  fail_unless(strcmp("\\\\x", out) == 0);

  strcpy(in, "\\\\");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 4);
  fail_unless(strcmp("\\\\\\\\", out) == 0);

  strcpy(in, "foo\\bar");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 8);
  fail_unless(strcmp("foo\\\\bar", out) == 0);

  strcpy(in, "\t\r\n");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 6);
  fail_unless(strcmp("\\t\\r\\n", out) == 0);

  strcpy(in, "foo\tbar\rbaz\n");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 15);
  fail_unless(strcmp("foo\\tbar\\rbaz\\n", out) == 0);

  strcpy(in, "\\a\\b\\c\\d\\e");
  backslash_encode(in, out);
  fail_unless(strlen(out) == 15);
  fail_unless(strcmp("\\\\a\\\\b\\\\c\\\\d\\\\e", out) == 0);

  /* decode */

  strcpy(in, "");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 0);
  fail_unless('\0' == out[0]);

  strcpy(in, "a");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 1);
  fail_unless(strcmp(in, out) == 0);

  strcpy(in, "\\t");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 1);
  fail_unless(strcmp("\t", out) == 0);

  strcpy(in, "\\t\\r\\n");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 3);
  fail_unless(strcmp("\t\r\n", out) == 0);

  strcpy(in, "foo\\tbar\\rbaz\\n");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 12);
  fail_unless(strcmp("foo\tbar\rbaz\n", out) == 0);

  strcpy(in, "\\\\");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 1);
  fail_unless(strcmp("\\", out) == 0);

  strcpy(in, "\\a\\b\\c\\d");
  backslash_decode(in, out);
  fail_unless(strlen(out) == 4);
  fail_unless(strcmp("abcd", out) == 0);
}
END_TEST

Suite*
string_utils_suite(void)
{
  Suite *s = suite_create("string_utils");
  TCase *tc_core = tcase_create("string_utils");
  tcase_add_test(tc_core, test_round_trip);
  tcase_add_test (tc_core, test_util_find);
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
