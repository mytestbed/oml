/*
 * Copyright 2007-2016 National ICT Australia (NICTA), Australia
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

static struct uri {
  char *uri;
  int ret;
  char *scheme;
  char *node;
  char *service;
} test_uris[] = {
  { "localhost", 0, NULL, "localhost", NULL},
  { "tcp:localhost", 0, "tcp", "localhost", NULL},
  { "tcp:localhost:3003", 0, "tcp", "localhost", "3003"},
  { "localhost:3003", 0, NULL, "localhost", "3003"},
  { "127.0.0.1", 0, NULL, "127.0.0.1", NULL},
  { "tcp:127.0.0.1", 0, "tcp", "127.0.0.1", NULL},
  { "tcp:127.0.0.1:3003", 0, "tcp", "127.0.0.1", "3003"},
  { "127.0.0.1:3003", 0, NULL, "127.0.0.1", "3003"},
  { "[127.0.0.1]", 0, NULL, "127.0.0.1", NULL},
  { "tcp:[127.0.0.1]", 0, "tcp", "127.0.0.1", NULL},
  { "tcp:[127.0.0.1]:3003", 0, "tcp", "127.0.0.1", "3003"},
  { "[127.0.0.1]:3003", 0, NULL, "127.0.0.1", "3003"},
  { "[::1]", 0, NULL, "::1", NULL},
  { "tcp:[::1]", 0, "tcp", "::1", NULL},
  { "tcp:[::1]:3003", 0, "tcp", "::1", "3003"},
  { "[::1]:3003", 0, NULL, "::1", "3003"},
  { "::1", -1, NULL, NULL, NULL},
  { "tcp:::1", -1, NULL, NULL, NULL},
  { "tcp:::1:3003", -1, "tcp", NULL, "3003"},
  { "::1:3003", -1, NULL, "[::1]", "3003"},
};

START_TEST(test_util_parse_uri)
{
  int ret;
  const char *scheme=NULL, *node=NULL, *service=NULL;

  ret = parse_uri(test_uris[_i].uri, &scheme, &node, &service);

  fail_unless(test_uris[_i].ret == ret, "Unexpected status from parse_uri(%s, ...): %d instead of %d",
      test_uris[_i].uri, ret, test_uris[_i].ret);
  if (test_uris[_i].ret == 0) {
    if (NULL == test_uris[_i].scheme) {
      fail_unless(test_uris[_i].scheme == NULL, "Unexpected scheme from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, scheme, test_uris[_i].ret);
    } else {
      fail_if(strcmp(scheme, test_uris[_i].scheme), "Unexpected scheme from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, scheme, test_uris[_i].ret);
    }
    if (NULL == test_uris[_i].node) {
      fail_unless(test_uris[_i].node == NULL, "Unexpected node from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, node, test_uris[_i].ret);
    } else {
      fail_if(strcmp(node, test_uris[_i].node), "Unexpected node from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, node, test_uris[_i].ret);
    }
    if (NULL == test_uris[_i].service) {
      fail_unless(test_uris[_i].service == NULL, "Unexpected service from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, service, test_uris[_i].ret);
    } else {
      fail_if(strcmp(service, test_uris[_i].service), "Unexpected service from parse_uri(%s, ...): %s instead of %s",
          test_uris[_i].uri, service, test_uris[_i].ret);
    }
  }
}
END_TEST

Suite* util_suite (void)
{
  Suite* s = suite_create ("Util");

  TCase* tc_util = tcase_create ("Util");

  tcase_add_test (tc_util, test_util_uri);
  tcase_add_test (tc_util, test_util_find);
  tcase_add_loop_test (tc_util, test_util_parse_uri, 0, LENGTH(test_uris));

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
