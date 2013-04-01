/*
 * Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file  check_liboml2_api.c
 * \brief Test the user-visible OML API.
 */
#include <string.h>
#include <check.h>

#include "ocomm/o_log.h"
#include "oml2/omlc.h"
#include "oml_util.h"
#include "oml_value.h"
#include "validate.h"
#include "client.h"

typedef struct
{
  const char* name;
  int is_valid_name;
  int is_valid_app_name;
} Name;


static Name names_vector [] =
  {
    { "internal space",         0, 0 },
    { "internal space two",     0, 0 },
    { "internal space three x", 0, 0 },
    { " leadingspace",          0, 0 },
    { "  leadingspace",         0, 0 },
    { "   leadingspace",        0, 0 },
    { "trailingspace ",         0, 0 },
    { "trailingspace  ",        0, 0 },
    { "trailingspace   ",       0, 0 },
    { " leading space",         0, 0 },
    { "  leading space",        0, 0 },
    { "   leading space",       0, 0 },
    { "trailing space ",        0, 0 },
    { "trailing space  ",       0, 0 },
    { "trailing space   ",      0, 0 },
    { " leadingspaceandtrailingspace ",       0, 0},
    { "  leadingspaceandtrailingspace  ",     0, 0},
    { "   leadingspaceandtrailingspace   ",   0, 0},
    { "    leadingspaceandtrailingspace    ", 0, 0},
    { " leading and internal space",    0, 0 },
    { "  leading and internal space",   0, 0 },
    { "   leading and internal space",  0, 0 },
    { "internal and trailing space ",   0, 0 },
    { "internal and trailing space  ",  0, 0 },
    { "internal and trailing space   ", 0, 0 },
    { "",                       0, 0 },
    { " ",                      0, 0 },
    { "   ",                    0, 0 },
    { "     ",                  0, 0 },
    { "validname",              1, 1 },
    { "valid_name",             1, 1 },
    { "valid/name",             0, 1 },
    { "valid/app/name",         0, 1 },
    { "/",                      0, 0 },
    { "v",                      1, 1 },
    { "_",                      1, 1 },
    { "1",                      0, 0 },
    { "1_invalid_name",         0, 0 },
    { "1invalidname",           0, 0 },
    { "validname1",             1, 1 },
    { "valid2name",             1, 1 },
    { "valid_234_name",         1, 1 },
    { "1/valid/app/name",       0, 1 },
    { "1/invalid/app/name/",    0, 0 },
  };

/******************************************************************************/
/*                    APP and MP NAME HANDLING CHECKS                         */
/******************************************************************************/

START_TEST (test_api_app_name_spaces)
{
  int res;
  res = omlc_init (names_vector[_i].name, 0, NULL, NULL);
  if (names_vector[_i].is_valid_app_name)
    {
      fail_if (res != 0, "Valid app name '%s' was rejected\n", names_vector[_i]);
      fail_if (omlc_instance == NULL);
    }
  else
    {
      fail_if (res != -1, "Invalid app name '%s' was incorrectly accepted\n", names_vector[_i]);
      fail_unless (omlc_instance == NULL);
    }
}
END_TEST

START_TEST (test_api_validate_name)
{
  int res = validate_name (names_vector[_i].name);
  if (names_vector[_i].is_valid_name) {
    fail_if (res == 0, "MP name '%s' incorrectly marked as invalid\n", names_vector[_i].name);
  } else {
    fail_unless (res == 0, "MP name '%s' incorrectly marked as valid\n", names_vector[_i].name);
  }
}
END_TEST

START_TEST (test_api_name_spaces)
{
  OmlMPDef def [] =
    {
      { "field1", OML_INT32_VALUE },
      { NULL, (OmlValueT)0 }
    };

  OmlClient dummy;
  memset(&dummy, 0, sizeof(OmlClient));
  omlc_instance = &dummy;

  const char* name = names_vector[_i].name;
  int is_valid = names_vector[_i].is_valid_name;

  OmlMP* res = omlc_add_mp (name, def);

  if (is_valid)
    fail_if (res == NULL, "omlc_add_mp () failed to create MP for valid name '%s'\n", name);
  else
    fail_unless (res == NULL, "omlc_add_mp() created an MP for invalid name '%s'\n", name);
}
END_TEST

OmlMPDef mpdef [] = {
  { "label", OML_STRING_VALUE },
  { NULL, (OmlValueT)0 }
};
OmlMP *mp;

START_TEST(test_api_basic)
{
  OmlMP *mp;
  OmlValueU value;

  o_set_log_level (2);
  logdebug("%s\n", __FUNCTION__);

  /* XXX: Yuck */
  const char* argv[] = {
    "--oml-id", __FUNCTION__,
    "--oml-domain", __FILE__,
    "--oml-collect", "file:test_api_basic",
    "--oml-log-level", "2"};
  int argc = 8;

  omlc_zero(value);
  omlc_set_string(value, "1337");

  fail_if(omlc_init("app", &argc, argv, NULL), "Error initialising OML");

  mp = omlc_add_mp("MP", mpdef);
  fail_if(mp == NULL, "Failed to add MP before omlc_start");

  fail_unless(omlc_inject(mp, &value),
      "omlc_inject() succeeded before omlc_start was called");

  fail_if(omlc_start(), "Error starting OML");
  fail_if(omlc_inject(mp, &value),
      "omlc_inject() failed after omlc_start was called");

  fail_if(omlc_close(), "Error closing OML");
}
END_TEST

START_TEST(test_api_metadata)
{
  OmlMP *mp;
  OmlValueU value;
  OmlValueT type = OML_STRING_VALUE;
  omlc_zero(value);
  omlc_set_string(value, "1337");

  o_set_log_level (2);
  logdebug("%s\n", __FUNCTION__);

  /* XXX: Yuck */
  const char* argv[] = {
    "--oml-id", __FUNCTION__,
    "--oml-domain", __FILE__,
    "--oml-collect", "file:test_api_metadata",
    "--oml-log-level", "2"};
  int argc = 8;

  fail_if(omlc_init("app", &argc, argv, NULL), "Error initialising OML");

  mp = omlc_add_mp("MP", mpdef);
  fail_if(mp == NULL, "Failed to add MP");

  fail_unless(-1 == omlc_inject_metadata(NULL, "k", &value, type, NULL),
      "omlc_inject_metadata accepted to work before omlc_start()");

  fail_if(omlc_start(), "Error starting OML");

  /* Test argument validation */
  fail_unless(-1 == omlc_inject_metadata(NULL, "k", &value, type, NULL),
      "omlc_inject_metadata accepted a NULL MP");
  fail_unless(-1 == omlc_inject_metadata(mp, NULL, &value, type, NULL),
      "omlc_inject_metadata accepted a NULL key");
  fail_unless(-1 == omlc_inject_metadata(mp, "k", NULL, type, NULL),
      "omlc_inject_metadata accepted a NULL value");

  omlc_set_string(value, "value");
  fail_unless(0 == omlc_inject_metadata(mp, "k", &value, type, NULL),
      "omlc_inject_metadata refused valid metadata");
  fail_unless(0 == omlc_inject_metadata(mp, "k", &value, type, "label"),
      "omlc_inject_metadata refused metadata for an existing field");

  for (type = OML_INPUT_VALUE; type <= OML_BLOB_VALUE; type++) {
    if (type != OML_STRING_VALUE) {
      fail_unless(-1 == omlc_inject_metadata(NULL, "k", &value, type, NULL),
          "omlc_inject_metadata accepted a non-string value type");
    }
  }

  omlc_reset_string(value);

  fail_if(omlc_close(), "Error closing OML");
}
END_TEST

Suite*
api_suite (void)
{
  Suite* s = suite_create("API");

  TCase* tc_api_names = tcase_create("ApiNames");

  tcase_add_loop_test (tc_api_names, test_api_app_name_spaces, 0, LENGTH(names_vector));
  tcase_add_loop_test (tc_api_names, test_api_validate_name, 0, LENGTH(names_vector));
  tcase_add_loop_test (tc_api_names, test_api_name_spaces, 0, LENGTH(names_vector));
  suite_add_tcase (s, tc_api_names);

  TCase* tc_api_func = tcase_create("ApiFunctions");
  tcase_add_test(tc_api_func, test_api_basic);
  tcase_add_test(tc_api_func, test_api_metadata);
  suite_add_tcase (s, tc_api_func);

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
