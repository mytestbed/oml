#include <stdio.h>
#include <check.h>
#include <headers.h>
#include "util.h"

/*
 * Used by:  test_tag_from_string
 */
static struct {
  const char *name;
  enum HeaderTag tag;
} vector_header_names [] = {
  { "protocol", H_PROTOCOL },
  { "experiment-id", H_EXPERIMENT_ID },
  { "sender-id", H_SENDER_ID },
  { "app-name", H_APP_NAME },
  { "content", H_CONTENT },
  { "schema", H_SCHEMA },
  { "start_time", H_START_TIME },
  { "start-time", H_START_TIME },

  { "protocolx", H_NONE },
  { "experiment-idx", H_NONE },
  { "sender-idx", H_NONE },
  { "app-namex", H_NONE },
  { "contentx", H_NONE },
  { "schemax", H_NONE },
  { "start_timex", H_NONE },
  { "start-timex", H_NONE },

  /*
  { "protocol", H_NONE},
  { "experiment-id", H_NONE},
  { "sender-id", H_NONE},
  { "app-name", H_NONE},
  { "content", H_NONE},
  { "schema", H_NONE},
  { "start_time", H_NONE},
  { "start-time", H_NONE},
  */

  { "p", H_NONE},
  { "pr", H_NONE},
  { "pro", H_NONE},
  { "rpotocol", H_NONE},
  { "pretocol", H_NONE},

  { " protocol", H_NONE},
  { "experiment-id ", H_NONE},
  { "sschema", H_NONE },
  { "start time", H_NONE },
  { "starttime", H_NONE },

  { NULL, H_NONE },
};

/*
 * Used by: test_header_from_string
 */
static struct {
  const char *input;
  struct header header;
  int is_null;
} vector_headers [] = {
  { "protocol: 1", { H_PROTOCOL, "1", NULL }, 0 },
  { "experiment-id: abc", { H_EXPERIMENT_ID, "abc", NULL }, 0 },
  { "content: binary", { H_CONTENT, "binary", NULL }, 0 },
  { "content: text  ", { H_CONTENT, "text  ", NULL }, 0 },
  { "content: t", { H_CONTENT, "t", NULL }, 0 },
  { "app-name   :  generator", { H_APP_NAME, "generator", NULL }, 0 },
  { "schema : 1 label:string", { H_SCHEMA, "1 label:string", NULL }, 0 },
  { "start_time: 123456690", { H_START_TIME, "123456690", NULL }, 0 },
  { "start-time: 123456690", { H_START_TIME, "123456690", NULL }, 0 },
  { "", { H_NONE, NULL, NULL }, 1 },
  { " ", { H_NONE, NULL, NULL }, 1 },
  { NULL, { H_NONE, NULL, NULL }, 1 },
  { "not-a-header", { H_NONE, NULL, NULL }, 1 },
  { "not-a-header : with a value", { H_NONE, NULL, NULL }, 1 },
};


START_TEST (test_tag_from_string)
{
  const char *name = vector_header_names[_i].name;
  size_t len = name ? strlen (name) : 1;
  enum HeaderTag expected = vector_header_names[_i].tag;
  enum HeaderTag actual = tag_from_string (name, len);

  fail_unless (actual == expected);
}
END_TEST

START_TEST (test_header_from_string)
{
  struct header *header;
  const char *input = vector_headers[_i].input;
  const struct header *expected = &vector_headers[_i].header;
  const int is_null = vector_headers[_i].is_null;
  size_t len = input ? strlen (input) : 0;

  header = header_from_string(input, len);

  fail_if (!is_null && header == NULL);
  fail_if (is_null  && header != NULL);

  if (!is_null) {
    fail_unless (header->tag == expected->tag);
    fail_unless (strcmp (header->value, expected->value) == 0);
  }
}
END_TEST

START_TEST (test_header_from_string_short)
{
  struct header *header;
  const char *input = vector_headers[_i].input;
  const struct header *expected = &vector_headers[_i].header;
  const int is_null = vector_headers[_i].is_null;
  size_t len = input ? strlen (input) : 0;

  /* Adjust so we don't read the whole string, but only if the string
     is long enough */
  if (len > 2)
    len -= 2;

  header = header_from_string(input, len);

  fail_if (!is_null && header == NULL);
  fail_if (is_null  && header != NULL);

  if (!is_null) {
    size_t check_len = strlen (expected->value);
    if (check_len > 2)
      check_len -= 2;
    fail_unless (header->tag == expected->tag);
    fail_unless (header->value && strlen (header->value) == check_len);
    fail_unless (strncmp (header->value, expected->value, check_len) == 0);
  }
}
END_TEST

Suite *
headers_suite (void)
{
  Suite *s = suite_create ("Headers");

  /* Headers test cases */
  TCase *tc_tag_from_string = tcase_create ("HeadersTagFromString");
  TCase *tc_header_from_string = tcase_create ("HeadersHeaderFromString");

  /* Add tests to test case "HeadersTagFromString" */
  tcase_add_loop_test (tc_tag_from_string, test_tag_from_string,
                       0, LENGTH (vector_header_names));
  tcase_add_loop_test (tc_header_from_string, test_header_from_string,
                       0, LENGTH (vector_headers));
  tcase_add_loop_test (tc_header_from_string, test_header_from_string_short,
                       0, LENGTH (vector_headers));

  suite_add_tcase (s, tc_tag_from_string);
  suite_add_tcase (s, tc_header_from_string);

  return s;
}
