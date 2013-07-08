/*
 * Copyright 2012-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file text_writer.c
 * \brief Tests behaviour and issues related to the text protocol.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <check.h>
#include <sqlite3.h>
#include <libgen.h>

#include "ocomm/o_log.h"
#include "oml_util.h"
#include "mem.h"
#include "mbuf.h"
#include "database.h"
#include "client_handler.h"
#include "sqlite_adapter.h"
#include "check_server.h"

extern char *dbbackend;
extern char *sqlite_database_dir;

/********************************************************************************/
/* Issue 672: timestamp from client not correctly processed by server over	*/
/* text protocol								*/
/********************************************************************************/

/* Prototypes of functions to test */

void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

START_TEST(test_text_insert)
{
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;

  char domain[] = "text-test";
  char dbname[sizeof(domain)+3];
  char table[] = "text_table";
  double time1 = 1.096202;
  double time2 = 2.092702;
  int d1 = 3319660544;
  int d2 = 106037248;

  char h[200];
  char s1[50];
  char s2[50];
  char select[200];

  int rc = -1;

  o_set_log_level(-1);
  logdebug("%s\n", __FUNCTION__);

  /* Remove pre-existing databases */
  *dbname=0;
  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  unlink(dbname);

  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\nschema: 1 %s size:uint32\n\n", domain, basename(__FILE__), __FUNCTION__, table);
  snprintf(s1, sizeof(s1), "%f\t1\t%d\t%d\n", time1, 1, d1);
  snprintf(s2, sizeof(s2), "%f\t1\t%d\t%d\n", time2, 2, d2);
  snprintf(select, sizeof(select), "select oml_ts_client, oml_seq, size from %s;", table);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "text insert socket";
  ch = check_server_prepare_client_handler("test_text_insert", &source);
  fail_unless(ch->state == C_HEADER);

  logdebug("Processing text protocol for issue #672\n");
  /* Process the header */
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->content == C_TEXT_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);

  /* Process the first sample */
  client_callback(&source, ch, s1, strlen(s1));

  /* Process the second sample */
  client_callback(&source, ch, s2, strlen(s2));

  database_release(ch->database);
  check_server_destroy_client_handler(ch);

  logdebug("Checking recorded data in %s.sq3\n", domain);
  /* Open database */
  db = database_find(domain);
  fail_if(db == NULL || ((Sq3DB*)(db->handle))->conn == NULL , "Cannot open SQLite3 database");
  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select, rc);

  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "First step of statement `%s' failed; rc=%d", select, rc);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time1) < 10e-10,
      "Invalid oml_ts_value in 1st row: expected `%f', got `%f'",
      time1, sqlite3_column_double(stmt, 0));
  fail_unless(fabs(sqlite3_column_int(stmt, 2) == d1),
      "Invalid size in 1st row: expected `%d', got `%d'",
      d1, sqlite3_column_int(stmt, 2));

  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "Second step of statement `%f' failed; rc=%d", select, rc);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time2) < 10e-10,
      "Invalid oml_ts_value in 2nd row: expected `%f', got `%f'",
      time2, sqlite3_column_double(stmt, 0));
  fail_unless(fabs(sqlite3_column_int(stmt, 2) == d2),
      "Invalid size in 2nd row: expected `%d', got `%d'",
      d2, sqlite3_column_int(stmt, 2));

  database_release(db);
}
END_TEST

#define MAXTYPETESTNAME 15
static struct {
 char *name;        /* name of this test, no longer than MAXTYPETESTNAME */
 char *proto_type;  /* type for schema */
 char *rep;         /* representation */
 char *exp;         /* expected database contents */
} type_tests[] = {
  /* 123456789012345 <- No longer than that */
  { "int32", "int32", "-2147483647", "-2147483647"},                      /* INT32_MIN+1 */
  { "uint32", "uint32", "2147483647", "2147483647"},                      /* UINT32_MAX */
  { "int64", "int64", "-9223372036854775807", "-9223372036854775807"},    /* INT32_MIN+1 */
  { "uint64", "uint64", "9223372036854775807", "9223372036854775807"},
  { "double", "double", "13.37", "13.37"},                                /* Leetness */
  { "dblNaNexpl", "double", "NAN", NULL},                                 /* Explicit NaN */
  { "dblNaNimpl", "double", "", NULL},                                    /* Implicit NaN */
  { "string", "string", "string", "string"},
  { "stringNULL", "string", "", ""},
  { "blob", "blob", "YWJjZGU=", "abcde"}, /* see test/lib/check_liboml2_base64.c:test_round_trip */
  { "blobNULL", "blob", "", ""},
  { "guid", "guid", "9223372036854775807", "9223372036854775807"},        /* Yup, they are uint64 */
  { "boolF", "bool", "FaLsE", "0"},
  { "bool1", "bool", "1", "1"},
  { "bool2", "bool", "2", "1"},
  { "bool0", "bool", "0", "1"}, /* that's right, the conversion is done by the OmlValue, see oml_value.c:oml_value_string_to_bool */
};

START_TEST(test_text_types)
{
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;

  char *domain = "text-test-types";
  char dbname[sizeof(domain)+3];
  char tablebase[] = "text_type_";
  char table[sizeof(tablebase)+MAXTYPETESTNAME+1];
  double time1 = 1.096202;
  int d1 = 3319660544;

  char h[200];
  char s[50];
  char select[200];

  int rc = -1;

  o_set_log_level(2);
  logdebug("%s\n", __FUNCTION__);

  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  snprintf(table, sizeof(table), "%s%s", tablebase, type_tests[_i].name);

  /* Remove pre-existing databases */
  unlink(dbname);

  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\nschema: 1 %s val:%s\n\n",
      domain, basename(__FILE__), __FUNCTION__, table, type_tests[_i].proto_type);
  snprintf(s, sizeof(s), "%f\t1\t%d\t%s\n", time1, 1, type_tests[_i].rep);
  snprintf(select, sizeof(select), "select val from %s;", table);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "text types socket";
  ch = check_server_prepare_client_handler("test_text_types", &source);
  fail_unless(ch->state == C_HEADER);

  logdebug("Processing text protocol for type %s\n", type_tests[_i].proto_type);
  /* Process the header */
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->content == C_TEXT_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);

  /* Process the first sample */
  client_callback(&source, ch, s, strlen(s));

  database_release(ch->database);
  check_server_destroy_client_handler(ch);

  logdebug("Checking recorded data in %s\n", dbname);
  /* Open database */
  db = database_find(domain);
  fail_if(db == NULL || ((Sq3DB*)(db->handle))->conn == NULL , "Cannot open SQLite3 database");
  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select, rc);

  rc = sqlite3_step(stmt);
  fail_unless(rc == SQLITE_ROW, "Statement `%s' failed; rc=%d", select, rc);
  const char *ret=(const char *)sqlite3_column_text(stmt, 0);
  if (NULL == ret) {
    fail_if(type_tests[_i].exp != NULL);
  } else {
    fail_if(strcmp(ret, type_tests[_i].exp),
        "%s: Invalid %s in data: expected `%s', got `%s'",
        type_tests[_i].name, type_tests[_i].proto_type,
        type_tests[_i].exp, sqlite3_column_text(stmt, 0));
  }
  database_release(db);
}
END_TEST

START_TEST(test_text_flexibility)
{
  /* XXX: Code duplication with check_binary_protocol.c:test_binary_flexibility*/
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;

  char domain[] = "text-flex-test";
  char dbname[sizeof(domain)+3];
  char table[3][12] = { "flex1_table", "flex2_table", "flex3_table" };
  double time1 = 1.096202;
  double time2 = 2.092702;
  int32_t d1 = 3319660544;
  int32_t d2 = 106037248;

  char h[200];
  char s1[200];
  char s2[200];
  char s3[200];
  char sample[200];
  char select1[200];
  char select2[200];
  char select3[200];

  int rc = -1;

  o_set_log_level(-1);
  logdebug("%s\n", __FUNCTION__);

  /* Remove pre-existing databases */
  *dbname=0;
  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  unlink(dbname);

  snprintf(s1, sizeof(s1), "1 %s size:uint32", table[0]);
  snprintf(s2, sizeof(s2), "2 %s size:uint32", table[1]);
  snprintf(s3, sizeof(s3), "1 %s bli:int32", table[2]);
  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\ncontent: text\nschema: %s\n\n", domain, basename(__FILE__), __FUNCTION__, s1);
  snprintf(select1, sizeof(select1), "select oml_ts_client, oml_seq, size from %s;", table[0]);
  snprintf(select2, sizeof(select2), "select oml_ts_client, oml_seq, size from %s;", table[1]);
  snprintf(select3, sizeof(select3), "select oml_ts_client, oml_seq, bli from %s;", table[2]);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "text flex socket";
  ch = check_server_prepare_client_handler("test_text_flex", &source);
  fail_unless(ch->state == C_HEADER);
  fail_unless(ch->table_count == 0, "Unexpected number of tables (%d instead of 0)", ch->table_count);

  logdebug("Sending header '%s'\n", h);
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->content == C_TEXT_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);
  fail_unless(ch->table_count == 2, "Unexpected number of tables (%d instead of 2)", ch->table_count);

  logdebug("Sending first sample\n");
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%d\n",
      /* time,  schema, sequence, sample */
      time1,    1,      1,        d1
      );
  client_callback(&source, ch, sample, strlen(sample));

  logdebug("Sending meta 'schema':'%s'\n", s2);
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t.\tschema\t%s\n",
      /* time,  schema, sequence, schemadef */
      time2,    0,      1,        s2
      );
  client_callback(&source, ch, sample, strlen(sample));
  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->table_count == 3, "Unexpected number of tables (%d instead of 3)", ch->table_count);

  logdebug("Sending second sample\n");
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%d\n",
      /* time,  schema, sequence, sample */
      time2,    2,      1,        d2
      );
  client_callback(&source, ch, sample, strlen(sample));

  logdebug("Overwriting schema: '%s'\n", s3);
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t.\tschema\t%s\n",
      /* time,  schema, sequence, schemadef */
      time2,    0,      1,        s3
      );
  client_callback(&source, ch, sample, strlen(sample));
  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->table_count == 3, "Unexpected number of tables (%d instead of 3)", ch->table_count);
  fail_unless(ch->state == C_TEXT_DATA);

  logdebug("Sending third sample\n");
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%d\n",
      /* time,  schema, sequence, sample */
      time1,    1,      1,        d1
      );
  client_callback(&source, ch, sample, strlen(sample));

  database_release(ch->database);
  check_server_destroy_client_handler(ch);

  logdebug("Checking recorded data in %s.sq3\n", domain);
  /* Open database */
  db = database_find(domain);
  fail_if(db == NULL || ((Sq3DB*)(db->handle))->conn == NULL , "Cannot open SQLite3 database");

  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select1, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select1, rc);
  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "First step of statement `%s' failed; rc=%d", select1, rc);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time1) < 1e-8,
      "Invalid oml_ts_value in 1st table: expected `%e', got `%e'",
      time1, sqlite3_column_double(stmt, 0), fabs(sqlite3_column_double(stmt, 0) - time1));
  fail_unless(sqlite3_column_int(stmt, 2) - d1 == 0,
      "Invalid size in 1st table: expected `%d', got `%d'", d1, sqlite3_column_double(stmt, 2));

  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select2, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select2, rc);
  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "First step of statement `%s' failed; rc=%d", select2, rc);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time2) < 1e-8,
      "Invalid oml_ts_value in 2nd table: expected `%e', got `%e'",
      time1, sqlite3_column_double(stmt, 0), fabs(sqlite3_column_double(stmt, 0) - time2));
  fail_unless(sqlite3_column_int(stmt, 2) - d2 == 0,
      "Invalid size in 2nd table: expected `%d', got `%d'", d1, sqlite3_column_double(stmt, 2));

  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select3, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select1, rc);
  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "First step of statement `%s' failed; rc=%d", select1, rc);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time1) < 1e-8,
      "Invalid oml_ts_value in 3rd table: expected `%e', got `%e'",
      time1, sqlite3_column_double(stmt, 0), fabs(sqlite3_column_double(stmt, 0) - time1));
  fail_unless(sqlite3_column_int(stmt, 2) - d1 == 0,
      "Invalid bli in 3rd table: expected `%d', got `%d'", d1, sqlite3_column_double(stmt, 2));

  database_release(db);
}
END_TEST

START_TEST(test_text_metadata)
{
  /* XXX: Code duplication with check_binary_protocol.c:test_binary_metadata*/
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;

  char domain[] = "text-meta-test";
  char dbname[sizeof(domain)+3];
  char table[3][12] = { "meta1_table" };
  double time1 = 1.096202;
#if DB_HAS_PKEY /* #814 */
  double time2 = 2.092702;
  char f1[] = "size";
#endif
  char k1[] = "key1";
  char v1[] = "val1";
  char k2[] = "key2";
  char v2[] = "val2";
  char *mp1 = table[1];
  char subject[10];

  char h[400];
  char s0[] = "0 _experiment_metadata subject:string key:string value:string";
  char s1[200];
  char sample[200];
  char select[200];

  int rc = -1;

  o_set_log_level(-1);
  logdebug("%s\n", __FUNCTION__);

  /* Remove pre-existing databases */
  *dbname=0;
  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  unlink(dbname);

  snprintf(s1, sizeof(s1), "1 %s size:uint32", table[0]);
  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\nschema: %s\ncontent: text\nschema: %s\n\n",
      domain, basename(__FILE__), __FUNCTION__, s0, s1);
  snprintf(select, sizeof(select), "select key, value, subject from _experiment_metadata;");

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "text meta socket";
  ch = check_server_prepare_client_handler("test_text_meta", &source);
  fail_unless(ch->state == C_HEADER);
  fail_unless(ch->table_count == 0, "Unexpected number of tables (%d instead of 0)", ch->table_count);

  logdebug("Sending header '%s'\n", h);
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
  fail_unless(ch->content == C_TEXT_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);
  fail_unless(ch->table_count == 2, "Unexpected number of tables (%d instead of 2)", ch->table_count);

  *subject=0;

  strcat(subject, ".");
  logdebug("Sending first meta '%s %s %s'\n", subject, k1, v1);
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%s\t%s\t%s\n",
      /* time,  schema, sequence, subject,  key, value; */
      time1,    0,      1,        subject,  k1,  v1
      );
  client_callback(&source, ch, sample, strlen(sample));
  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);

  logdebug("Sending second meta '%s %s %s'\n", subject, k2, v2);
  strcat(subject, mp1);
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%s\t%s\t%s\n",
      /* time,  schema, sequence, subject,  key, value; */
      time1,    0,      2,        subject,  k2,  v2
      );
  client_callback(&source, ch, sample, strlen(sample));
  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);

#if DB_HAS_PKEY /* #814 */
  logdebug("Sending third meta '%s %s %s'\n", subject, k1, v2);
  strcat(subject, ".");
  strcat(subject, f1);
  snprintf(sample, sizeof(sample), "%f\t%d\t%d\t%s\t%s\t%s\n",
      /* time,  schema, sequence, subject,  key, value; */
      time1,    0,      3,        subject,  k1,  v2
      );
  client_callback(&source, ch, sample, strlen(sample));
  fail_unless(ch->state == C_TEXT_DATA, "Inconsistent state: expected %d, got %d", C_TEXT_DATA, ch->state);
#endif

  database_release(ch->database);
  check_server_destroy_client_handler(ch);

  logdebug("Checking recorded data in %s.sq3\n", domain);
  /* Open database */
  db = database_find(domain);
  fail_if(db == NULL || ((Sq3DB*)(db->handle))->conn == NULL , "Cannot open SQLite3 database");

  *subject=0;

  strcat(subject, ".");
  rc = sqlite3_prepare_v2(((Sq3DB*)(db->handle))->conn, select, -1, &stmt, 0);
  fail_unless(rc == 0, "Preparation of statement `%s' failed; rc=%d", select, rc);
  rc = sqlite3_step(stmt);
  rc = sqlite3_step(stmt); /* Skip start_time */
  rc = sqlite3_step(stmt); /* Skip schema 0 */
  rc = sqlite3_step(stmt); /* Skip schema 1 */
  fail_unless(rc == 100, "First steps of statement `%s' failed; rc=%d", select, rc);
  fail_if(strcmp(k1, (const char*)sqlite3_column_text(stmt, 0)),
      "Invalid 1st key in metadata table: expected `%s', got `%s'",
      k1, (const char*)sqlite3_column_text(stmt, 0));
  fail_if(strcmp(v1, (const char*)sqlite3_column_text(stmt, 1)),
      "Invalid 1st value in metadata table: expected `%s', got `%s'",
      v1, (const char*)sqlite3_column_text(stmt, 1));
  fail_if(strcmp(subject, (const char*)sqlite3_column_text(stmt, 2)),
      "Invalid 1st subject in metadata table: expected `%s', got `%s'",
      subject, (const char*)sqlite3_column_text(stmt, 1));

  strcat(subject, mp1);
  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "Second step of statement `%s' failed; rc=%d", select, rc);
  fail_if(strcmp(k2, (const char*)sqlite3_column_text(stmt, 0)),
      "Invalid 2nd key in metadata table: expected `%s', got `%s'",
      k2, (const char*)sqlite3_column_text(stmt, 0));
  fail_if(strcmp(v2, (const char*)sqlite3_column_text(stmt, 1)),
      "Invalid 2nd value in metadata table: expected `%s', got `%s'",
      v2, (const char*)sqlite3_column_text(stmt, 1));
  fail_if(strcmp(subject, (const char*)sqlite3_column_text(stmt, 2)),
      "Invalid 2nd subject in metadata table: expected `%s', got `%s'",
      subject, (const char*)sqlite3_column_text(stmt, 1));

#if DB_HAS_PKEY /* #814 */
  strcat(subject, ".");
  strcat(subject, f1);
  rc = sqlite3_step(stmt);
  fail_unless(rc == 100, "Second step of statement `%s' failed; rc=%d", select, rc);
  fail_if(strcmp(k1, (const char*)sqlite3_column_text(stmt, 0)),
      "Invalid 3rd key in metadata table: expected `%s', got `%s'",
      k1, (const char*)sqlite3_column_text(stmt, 0));
  fail_if(strcmp(v2, (const char*)sqlite3_column_text(stmt, 1)),
      "Invalid 3rd value in metadata table: expected `%s', got `%s'",
      v2, (const char*)sqlite3_column_text(stmt, 1));
  fail_if(strcmp(subject, (const char*)sqlite3_column_text(stmt, 2)),
      "Invalid 3rd subject in metadata table: expected `%s', got `%s'",
      subject, (const char*)sqlite3_column_text(stmt, 1));
#endif

  database_release(db);
}
END_TEST

Suite*
text_protocol_suite (void)
{
  Suite* s = suite_create ("Text protocol");

  dbbackend = "sqlite";
  sqlite_database_dir = ".";

  TCase* tc_text_insert = tcase_create ("Text insert");
  tcase_add_test (tc_text_insert, test_text_insert);
  tcase_add_loop_test (tc_text_insert, test_text_types, 0, LENGTH (type_tests));
  suite_add_tcase (s, tc_text_insert);

  TCase* tc_text_flex = tcase_create ("Text flexibility");
  tcase_add_test (tc_text_flex, test_text_flexibility);
  tcase_add_test (tc_text_flex, test_text_metadata);
  suite_add_tcase (s, tc_text_flex);

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
