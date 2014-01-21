/*
 * Copyright 2013-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_liboml2_config.c
 * \brief Test the client-side XML configuration parsing.
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <check.h>

#include "ocomm/o_log.h"
#include "oml2/omlc.h"
#include "check_util.h"

OmlMPDef mp_def [] =
{
  { "f1", OML_UINT32_VALUE },
  { "f2", OML_UINT32_VALUE },
  { NULL, (OmlValueT)0 }
};


/** Check that the _experiment_metadat MP is sent when a config file is given (#1266) */
START_TEST (test_config_metadata)
{
  OmlMP *mp;
  OmlValueU v[2];
  char buf[1024];
  char config[] = "<omlc domain='check_liboml2_config' id='test_config_metadata'>\n"
                  "  <collect url='file:test_config_metadata' encoding='text'>\n"
                  "    <stream mp='test_config_metadata' samples='1' />\n"
                  "  </collect>\n"
                  "</omlc>";
  int s0found = 0;
  FILE *fp;

  logdebug("%s\n", __FUNCTION__);

  MAKEOMLCMDLINE(argc, argv, "file:test_config_metadata");
  argv[1] = "--oml-config";
  argv[2] = "test_config_metadata.xml";
  argc = 3;

  fp = fopen (argv[2], "w");
  fail_unless(fp != NULL, "Could not create configuration file %s: %s", argv[2], strerror(errno));
  fail_unless(fwrite(config, sizeof(config), 1, fp) == 1,
      "Could not write configuration in file %s: %s", argv[2], strerror(errno));
  fclose(fp);

  unlink("test_config_metadata");

  fail_if(omlc_init(__FUNCTION__, &argc, argv, NULL),
      "Could not initialise OML");
  mp = omlc_add_mp(__FUNCTION__, mp_def);
  fail_if(mp==NULL, "Could not add MP");
  fail_if(omlc_start(), "Could not start OML");

  omlc_set_uint32(v[0], 1);
  omlc_set_uint32(v[1], 2);

  fail_if(omlc_inject(mp, v), "Injection failed");

  omlc_close();

  fp = fopen(__FUNCTION__, "r");
  fail_unless(fp != NULL, "Output file %s missing", __FUNCTION__);

  while(fgets(buf, sizeof(buf), fp) && !s0found) {
    if (!strncmp(buf, "schema: 0", 9)) {
      fail_unless(!strncmp(buf, "schema: 0 _experiment_metadata", 30),
          "Schema 0 found, but not _experiment_metadata,  in '%s'", buf);
        s0found = 1;
    }
  }
  fail_unless(s0found, "Schema 0 not found");

  fclose(fp);
}
END_TEST

/** Check that an empty <collect /> sends all streams to that collection point (#1272) */
START_TEST (test_config_empty_collect)
{
  OmlMP *mp;
  OmlValueU v[2];
  char buf[1024];
  char config[] = "<omlc domain='check_liboml2_config' id='test_config_empty_collect'>\n"
                  "  <collect url='file:test_config_empty_collect' encoding='text' />\n"
                  "</omlc>";
  int s1found = 0;
  FILE *fp;

  logdebug("%s\n", __FUNCTION__);

  MAKEOMLCMDLINE(argc, argv, "file:test_config_empty_collect");
  argv[1] = "--oml-config";
  argv[2] = "test_config_empty_collect.xml";
  argc = 3;

  fp = fopen (argv[2], "w");
  fail_unless(fp != NULL, "Could not create configuration file %s: %s", argv[2], strerror(errno));
  fail_unless(fwrite(config, sizeof(config), 1, fp) == 1,
      "Could not write configuration in file %s: %s", argv[2], strerror(errno));
  fclose(fp);

  unlink("test_config_empty_collect");

  fail_if(omlc_init(__FUNCTION__, &argc, argv, NULL),
      "Could not initialise OML");
  mp = omlc_add_mp(__FUNCTION__, mp_def);
  fail_if(mp==NULL, "Could not add MP");
  fail_if(omlc_start(), "Could not start OML");

  omlc_set_uint32(v[0], 1);
  omlc_set_uint32(v[1], 2);

  fail_if(omlc_inject(mp, v), "Injection failed");

  omlc_close();

  fp = fopen(__FUNCTION__, "r");
  fail_unless(fp != NULL, "Output file %s missing", __FUNCTION__);

  while(fgets(buf, sizeof(buf), fp) && !s1found) {
    if (!strncmp(buf, "schema: 1", 9)) {
        s1found = 1;
    }
  }
  fail_unless(s1found, "Schema 1 never defined");

  fclose(fp);
}
END_TEST

/** Check that multiple <collect /> do not trigger a "Measurement stream 'd_lin' already exists" error (#1154) */
START_TEST (test_config_multi_collect)
{
  OmlMP *mp;
  OmlValueU v[2];
  char buf[1024], *bufp;
  char *dests[2] = { "test_config_multi_collect1", "test_config_multi_collect2" };
  char config[] = "<omlc domain='check_liboml2_config' id='test_config_multi_collect'>\n"
                  "  <collect url='file:test_config_multi_collect1' encoding='text' />\n"
                  "  <collect url='file:test_config_multi_collect2' encoding='text' />\n"
                  "</omlc>";
  int i, s1found = 0, emptyfound = 0, n, datafound[2] = {0, 0};
  FILE *fp;

  logdebug("%s\n", __FUNCTION__);

  MAKEOMLCMDLINE(argc, argv, "file:test_config_multi_collect");
  argv[1] = "--oml-config";
  argv[2] = "test_config_multi_collect.xml";
  argc = 3;

  fp = fopen (argv[2], "w");
  fail_unless(fp != NULL, "Could not create configuration file %s: %s", argv[2], strerror(errno));
  fail_unless(fwrite(config, sizeof(config), 1, fp) == 1,
      "Could not write configuration in file %s: %s", argv[2], strerror(errno));
  fclose(fp);

  unlink("test_config_multi_collect1");
  unlink("test_config_multi_collect2");

  fail_if(omlc_init(__FUNCTION__, &argc, argv, NULL),
      "Could not initialise OML");
  mp = omlc_add_mp(__FUNCTION__, mp_def);
  fail_if(mp==NULL, "Could not add MP");
  fail_if(omlc_start(), "Could not start OML");

  omlc_set_uint32(v[0], 1);
  omlc_set_uint32(v[1], 2);

  fail_if(omlc_inject(mp, v), "Injection failed");

  omlc_close();

  for (i=0; i<2; i++) {
    s1found = 0;
    fp = fopen(dests[i], "r");
    fail_unless(fp != NULL, "Output file %s missing", __FUNCTION__);

    emptyfound = 0;
    while(fgets(buf, sizeof(buf), fp) && (!s1found || !datafound[i])) {

      if (!strncmp(buf, "schema: 1", 9)) {
        s1found = 1;

      } else if (emptyfound) {
        /* We know we are passed the header;
         * We check that the data is here, that is, we have
         * 5 tab-delimited fields (ts, schemaid, seqno, d1, d2) */
        bufp = buf;
        n = 0;
        while(++n<6 && strtok(bufp, "\t")) {
          bufp = NULL; /* Make strtok continue on the same string */
        };
        if (n==6) {
          datafound[i] = 1;
        }

      } else if (*buf == '\n') {
        emptyfound = 1;
      }
    }
    fail_unless(s1found, "Schema 1 never defined in %s", dests[i]);
    fail_unless(datafound[i], "No actual data in %s", dests[i]);

    fclose(fp);
  }
}
END_TEST

Suite*
config_suite (void)
{
  Suite* s = suite_create ("Config");

  TCase* tc_config = tcase_create ("Config");

  tcase_add_test (tc_config, test_config_metadata);
  tcase_add_test (tc_config, test_config_empty_collect);
  tcase_add_test (tc_config, test_config_multi_collect);

  suite_add_tcase (s, tc_config);

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
