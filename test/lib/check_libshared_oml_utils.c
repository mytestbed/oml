/*
 * Copyright 2007-2015 National ICT Australia (NICTA), Australia
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

static struct uri_scheme {
  char *uri;
  OmlURIType expect;
} test_uri_schemes[] = {
  { "blah", OML_URI_UNKNOWN},
  { "file://blah", OML_URI_FILE },
  { "flush://blah", OML_URI_FILE_FLUSH },
  { "tcp://blah", OML_URI_TCP },
  { "udp://blah", OML_URI_UDP },
  /* Meaningless, but this allows to check compound URI schemes */
  { "tcp+file://blah", OML_URI_TCP | OML_URI_FILE },

  { "zlib+tcp://blah", OML_URI_ZLIB | OML_URI_TCP},
  { "zlib+file:blah", OML_URI_ZLIB | OML_URI_FILE},
  { "zlib://blah", OML_URI_ZLIB }, /* XXX: Not sure that this one is desirable as a URI, see comments in test_uris */
  { "zlib:blah", OML_URI_ZLIB }, /* XXX: Ditto */
};

START_TEST (test_util_uri_scheme)
{
  OmlURIType res;

  res = oml_uri_type(test_uri_schemes[_i].uri);
  fail_unless(
      res == test_uri_schemes[_i].expect,
      "Invalid type for `%s': %d instead of %d", test_uri_schemes[_i].uri, res, test_uri_schemes[_i].expect); 
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
  { "tcp://localhost", 0, "tcp", "localhost", DEF_PORT_STRING, NULL},
  { "tcp://localhost:3004", 0, "tcp", "localhost", "3004", NULL},
  { "localhost:3004", 0, "tcp", "localhost", "3004", NULL},

  { "127.0.0.1", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "127.0.0.1:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "tcp://127.0.0.1", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "tcp://127.0.0.1:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "[127.0.0.1]", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},

  { "tcp://[127.0.0.1]", 0, "tcp", "127.0.0.1", DEF_PORT_STRING, NULL},
  { "tcp://[127.0.0.1]:3004", 0, "tcp", "127.0.0.1", "3004", NULL},
  { "[127.0.0.1]:3004", 0, "tcp", "127.0.0.1", "3004", NULL},

  { "[::1]", 0, "tcp", "::1", DEF_PORT_STRING, NULL},
  { "[::1]:3004", 0, "tcp", "::1", "3004", NULL},
  { "tcp://[::1]", 0, "tcp", "::1", DEF_PORT_STRING, NULL},
  { "tcp://[::1]:3004", 0, "tcp", "::1", "3004", NULL},

  { "file:-", 0, "file", NULL, NULL, "-"},

  { "gzip+tcp://localhost", 0, "gzip+tcp", "localhost", DEF_PORT_STRING, NULL},
  { "gzip+tcp://localhost:3004", 0, "gzip+tcp", "localhost", "3004", NULL},
  { "gzip+file:-", 0, "gzip+file", NULL, NULL, "-"},
  { "gzip+file:/a", 0, "gzip+file", NULL, NULL, "/a"},
  { "gzip+file:///a", 0, "gzip+file", NULL, NULL, "///a"},

  { "zlib+tcp://localhost", 0, "zlib+tcp", "localhost", DEF_PORT_STRING, NULL},
  { "zlib+tcp://localhost:3004", 0, "zlib+tcp", "localhost", "3004", NULL},
  { "zlib+file:-", 0, "zlib+file", NULL, NULL, "-"},
  { "zlib+file:/a", 0, "zlib+file", NULL, NULL, "/a"},
  { "zlib+file:///a", 0, "zlib+file", NULL, NULL, "///a"},
  /* XXX: Do we want to allow this? */
  //{ "zlib://localhost:3004", 0, "zlib+tcp", "localhost", "3004", NULL},
  /* XXX: Or that? */
  //{ "zlib:/path/to/file", 0, "zlib+file", NULL, NULL, "/path/to/file"},
  //{ "zlib:///path/to/file", 0, "zlib+file", NULL, NULL, "///path/to/file"},
  //{ "zlib:file", 0, "zlib+file", NULL, NULL, "file"},
  /* XXX: A first issue here is that the '+' after 'zlib' is not present anymore, and doesn't match the RE */

  /* Backward compatibility */
  { "tcp:localhost:3004", 0, "tcp", "localhost", "3004", NULL},
  { "file:test_api_metadata", 0, "file", NULL, NULL, "test_api_metadata"},
  { "file://test_api_metadata", 0, "file", NULL, NULL, "//test_api_metadata"},
  { "file:///test_api_metadata", 0, "file", NULL, NULL, "///test_api_metadata"},

  { "::1", -1, NULL, NULL, NULL, NULL},
  { "::1:3003", -1, NULL, NULL, NULL, NULL},
  { "tcp:::1", -1, NULL, NULL, NULL, NULL},
  { "tcp:::1:3003", -1, NULL, NULL, NULL, NULL},
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
  tcase_add_loop_test (tc_util, test_util_uri_scheme, 0, LENGTH(test_uri_schemes));
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
