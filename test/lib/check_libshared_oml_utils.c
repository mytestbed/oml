/*
 * Copyright 2007-2014 National ICT Australia (NICTA), Australia
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

#include "ocomm/o_log.h"
#include "oml_utils.h"

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

static struct uri {
  char *uri;
  int ret;
  char *scheme;
  char *host;
  char *port;
  char *path;
} test_uris[] = {
  { "localhost", 0, "tcp", "localhost", DEF_PORT_STRING, NULL},
  { "tcp:localhost", 0, "tcp", "localhost", DEF_PORT_STRING, NULL},
  { "tcp:localhost:3004", 0, "tcp", "localhost", "3004", NULL},
  { "localhost:3004", 0, "tcp", "localhost", "3004", NULL},

  { "127.0.0.1", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "127.0.0.1:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "tcp:127.0.0.1", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "tcp:127.0.0.1:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "[127.0.0.1]", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},

  { "tcp:[127.0.0.1]", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "tcp:[127.0.0.1]:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "[127.0.0.1]:3004", 0, "tcp", "127.0.0.1", "3004", NULL},

  { "[::1]", 0, "tcp", "::1", DEF_PORT_STRING, NULL},
  { "[::1]:3004", 0, "tcp", "::1", "3004", NULL},
  { "tcp:[::1]", 0, "tcp", "::1", DEF_PORT_STRING, NULL},
  { "tcp:[::1]:3004", 0, "tcp", "::1", "3004", NULL},

  { "file:-", 0, "file", NULL, NULL, "-"},

  { "::1", -1, NULL, NULL, NULL, NULL},
  { "::1:3003", -1, NULL, NULL, NULL, NULL},
  { "tcp:::1", -1, NULL, NULL, NULL, NULL},
  { "tcp:::1:3003", -1, NULL, NULL, NULL, NULL},

  { "file:test_api_metadata", 0, "file", NULL, NULL, "test_api_metadata"},
};

#define check_match(field) do { \
  if (NULL == test_uris[_i].field) { \
    fail_unless(test_uris[_i].field == NULL, "Unexpected " #field " from parse_uri(%s, %s, %s, %s, %s): %s instead of %s", \
        test_uris[_i].uri, scheme, host, port, path, field, test_uris[_i].field); \
  } else { \
    fail_if(strcmp(field, test_uris[_i].field), "Unexpected " #field " from parse_uri(%s, %s, %s, %s, %s): %s instead of %s", \
        test_uris[_i].uri, scheme, host, port, path, field, test_uris[_i].field); \
  } \
} while(0)

START_TEST(test_util_parse_uri)
{
  int ret;
  const char *scheme, *host, *port, *path;

  ret = parse_uri(test_uris[_i].uri, &scheme, &host, &port, &path);

  fail_unless(test_uris[_i].ret == ret, "Unexpected status from parse_uri(%s, %s, %s, %s, %s): %d instead of %d",
      test_uris[_i].uri, scheme, host, port, path, ret, test_uris[_i].ret);
  check_match(scheme);
  check_match(host);
  check_match(port);
  check_match(path);
}
END_TEST

Suite* util_suite (void)
{
  Suite* s = suite_create ("oml_utils");

  TCase* tc_util = tcase_create ("oml_utils");

  tcase_add_test (tc_util, test_util_uri);
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
