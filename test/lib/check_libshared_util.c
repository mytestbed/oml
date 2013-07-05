/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
#include <check.h>

#include "oml_util.h"

#define N_URI_TEST 5
START_TEST (test_util_uri)
{
  int i;
  OmlURIType res;
  struct {
    char *uri;
    OmlURIType expect;
  } test_data[N_URI_TEST];

  test_data[0].uri = "blah";
  test_data[0].expect = OML_URI_UNKNOWN;
  test_data[1].uri = "file://blah";
  test_data[1].expect = OML_URI_FILE;
  test_data[2].uri = "flush://blah";
  test_data[2].expect = OML_URI_FILE_FLUSH;
  test_data[3].uri = "tcp://blah";
  test_data[3].expect = OML_URI_TCP;
  test_data[4].uri = "udp://blah";
  test_data[4].expect = OML_URI_UDP;

  for (i=0; i<N_URI_TEST; i++) {
    res = oml_uri_type(test_data[i].uri);
    fail_unless(
        res == test_data[i].expect,
        "Invalid type for `%s': %d instead of %d", test_data[i].uri, res, test_data[i].expect); 
  }
}
END_TEST

START_TEST (test_util_find)
{
  char ws[] = "   ";
  char ts[] = " abc def";
  char *tsnwf = ts + 1;
  char *tsnw = tsnwf + 4;
  const char *got;

  /* XXX: This should really be using a loop test */

  fail_unless((got=skip_white(ws)) == ws+sizeof(ws)-1,
      "exp: %p :'%s'; got: %p: '%s'", ws+sizeof(ws)-1, ws+sizeof(ws)-1, got, got);
  fail_unless((got=skip_white(ts)) == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  fail_unless((got=skip_white(tsnwf)) == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  fail_unless((got=skip_white(tsnw)) == tsnw,
      "exp: %p :'%s'; got: %p: '%s'", tsnw, tsnw, got, got);

  fail_unless((got=find_white(ts)) == ts,
      "exp: %p :'%s'; got: %p: '%s'", ts, ts, got, got);
  fail_unless((got=find_white(tsnwf)) == tsnw - 1,
      "exp: %p :'%s'; got: %p: '%s'", tsnw-1, tsnw-1, got, got);
  fail_unless((got=find_white(tsnw)) == tsnw+strlen(tsnw),
      "exp: %p :'%s'; got: %p: '%s'", tsnw+strlen(tsnw), tsnw+strlen(tsnw), got, got);

  fail_unless((got=find_charn(ts, 'a', sizeof(ts))) == tsnwf,
      "exp: %p :'%s'; got: %p: '%s'", tsnwf, tsnwf, got, got);
  fail_unless((got=find_charn(ts, 'z', sizeof(ts) + 10)) == NULL,
      "exp: %p :'%s'; got: %p: '%s'", NULL, NULL, got, got);
  fail_unless((got=find_charn(ts, 'a', 1)) == NULL,
      "exp: %p :'%s'; got: %p: '%s'", NULL, NULL, got, got);
}
END_TEST

Suite* util_suite (void)
{
  Suite* s = suite_create ("Util");

  TCase* tc_util = tcase_create ("Util");

  tcase_add_test (tc_util, test_util_uri);
  tcase_add_test (tc_util, test_util_find);

  suite_add_tcase (s, tc_util);

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
