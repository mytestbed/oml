/*
 * Copyright 2012 National ICT Australia (NICTA), Australia
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
/** \file Tests behaviours and issues related to the binary protocol. */
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <math.h>
#include <check.h>
#include <sqlite3.h>
#include <libgen.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mbuf.h"
#include "oml_util.h"
#include "oml_value.h"
#include "marshal.h"
#include "binary.h"
#include "database.h"
#include "client_handler.h"
#include "sqlite_adapter.h"
#include "check_server.h"

extern char *dbbackend;
extern char *sqlite_database_dir;

/* Prototypes of functions to test */

void client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

#define printmbuf(mbuf)  logdebug("Sending message (in %p, at %p, from %p):\n%s\n", \
      (mbuf_buffer(mbuf)),                                                          \
      (mbuf_message(mbuf)),                                                         \
      (mbuf_rdptr(mbuf)),                                                           \
      to_octets(mbuf_message(mbuf), mbuf_remaining(mbuf)))

START_TEST(test_find_sync)
{
  uint8_t data[] = { 0xaa, 0xaa, 0x1, 0xaa, 0xaa, 0x2, };

  fail_unless(find_sync(data, 0) == NULL);
  fail_unless(find_sync(data, 1) == NULL);
  fail_unless(find_sync(data, sizeof(data)) == data);
  fail_unless(find_sync(data+1, sizeof(data)-1) == data+3);
  fail_unless(find_sync(data+3, sizeof(data)-3) == data+3);
  fail_unless(find_sync(data+4, sizeof(data)-4) == NULL);
  fail_unless(find_sync(data+5, sizeof(data)-5) == NULL);
}
END_TEST

START_TEST(test_bin_find_sync)
{
  MBuffer *mbuf = mbuf_create();
  uint8_t data[] = { 0xaa, 0xaa, 0x1, 0xaa, 0xaa, 0x2, };

  mbuf_write(mbuf, data, sizeof(data));

  fail_unless(bin_find_sync(mbuf) == 0);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == 2);
  fail_unless(bin_find_sync(mbuf) == 0);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == -1);
  mbuf_read_skip(mbuf, 1);
  fail_unless(bin_find_sync(mbuf) == -1);

  mbuf_destroy(mbuf);
}
END_TEST

START_TEST(test_binary_resync)
{
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;
  MBuffer* mbuf = mbuf_create();

  char domain[] = "binary-resync-test";
  char dbname[sizeof(domain)+3];
  char table[] = "resync1_table";
  double time1 = 1.096202;
  double time2 = 2.092702;
  int32_t d1 = 3319660544;
  int32_t d2 = 106037248;

  char h1[200];
  char select1[200];

  OmlValue v;

  int rc = -1;

  o_set_log_level(-1);
  logdebug("%s\n", __FUNCTION__);

  /* Remove pre-existing databases */
  *dbname=0;
  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  unlink(dbname);

  snprintf(h1, sizeof(h1),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\ncontent: binary\nschema: 1 %s size:uint32\n\n", domain, basename(__FILE__), __FUNCTION__, table);
  snprintf(select1, sizeof(select1), "select oml_ts_client, oml_seq, size from %s;", table);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "binary resync socket";
  ch = check_server_prepare_client_handler("test_binary_resync", &source);
  fail_unless(ch->state == C_HEADER);

  logdebug("Sending header '%s'\n", h1);
  client_callback(&source, ch, h1, strlen(h1));

  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
  fail_unless(ch->content == C_BINARY_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);

  logdebug("Sending first sample\n");
  oml_value_set_type(&v, OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v), d1);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 1, time1);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_remaining(mbuf));

  logdebug("Sending second sample for an invalid table, in two steps\n");
  oml_value_set_type(&v, OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v), d2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 2, 1, time2);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), 5);
  printmbuf(mbuf);
  fail_if(ch->state == C_PROTOCOL_ERROR, "An incomplete sample confused the client_handler");
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf)+5, mbuf_remaining(mbuf)-5);
  printmbuf(mbuf);
  fail_if(ch->state == C_PROTOCOL_ERROR, "A sample for a non existing stream confused the client_handler");

  logdebug("Sending some noise\n");
  mbuf_clear(mbuf);
  mbuf_write(mbuf, (uint8_t*)"BRuit", 6);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_remaining(mbuf));
  fail_if(ch->state == C_PROTOCOL_ERROR, "Some more noise disturbed the client_handler");

  logdebug("Sending third sample, in two steps\n");
  oml_value_set_type(&v, OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v), d2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 2, time2);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), 5);
  fail_if(ch->state == C_PROTOCOL_ERROR, "An incomplete sample after resync confused the client_handler");
  client_callback(&source, ch, mbuf_buffer(mbuf)+5, mbuf_remaining(mbuf)-5);

  fail_unless(ch->state == C_BINARY_DATA, "The client handler didn't manage to recover");

  database_release(ch->database);
  check_server_destroy_client_handler(ch);
  mbuf_destroy(mbuf);

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
  sqlite3_step(stmt);
  fail_unless(fabs(sqlite3_column_double(stmt, 0) - time2) <  1e-8,
      "Invalid oml_ts_value in 1st table: expected `%e', got `%e'",
      time2, sqlite3_column_double(stmt, 0), fabs(sqlite3_column_double(stmt, 0) - time2),
      fabs(sqlite3_column_double(stmt, 0) - time2)
      );
  fail_unless(sqlite3_column_int(stmt, 2) - d2 == 0,
      "Invalid size in 1st table: expected `%d', got `%d'", d2, sqlite3_column_double(stmt, 2));

  database_release(db);
}
END_TEST

START_TEST(test_binary_flexibility)
{
  /* XXX: Code duplication with check_text_protocol.c:test_text_flexibility */
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;
  MBuffer* mbuf = mbuf_create();

  char domain[] = "binary-flex-test";
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
  char select1[200];
  char select2[200];
  char select3[200];

  OmlValue v[3];
  oml_value_array_init(v, 3);

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
  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\ncontent: binary\nschema: %s\n\n", domain, basename(__FILE__), __FUNCTION__, s1);
  snprintf(select1, sizeof(select1), "select oml_ts_client, oml_seq, size from %s;", table[0]);
  snprintf(select2, sizeof(select2), "select oml_ts_client, oml_seq, size from %s;", table[1]);
  snprintf(select3, sizeof(select3), "select oml_ts_client, oml_seq, bli from %s;", table[2]);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "binary flexibility socket";
  ch = check_server_prepare_client_handler("test_binary_flexibility", &source);
  fail_unless(ch->state == C_HEADER);
  fail_unless(ch->table_count == 0, "Unexpected number of tables (%d instead of 0)", ch->table_count);

  logdebug("Sending header '%s'\n", h);
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
  fail_unless(ch->content == C_BINARY_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);
  fail_unless(ch->table_count == 2, "Unexpected number of tables (%d instead of 2)", ch->table_count);

  logdebug("Sending first sample\n");
  oml_value_set_type(&v[0], OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v[0]), d1);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 1, time1);
  marshal_values(mbuf, &v[0], 1);
  marshal_finalize(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));

  logdebug("Sending meta 'schema':'%s'\n", s2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 0, 1, time2);
  oml_value_set_type(&v[0], OML_STRING_VALUE);
  oml_value_set_type(&v[1], OML_STRING_VALUE);
  oml_value_set_type(&v[2], OML_STRING_VALUE);
  omlc_set_const_string(*oml_value_get_value(&v[0]), ".");
  omlc_set_const_string(*oml_value_get_value(&v[1]), "schema");
  omlc_set_const_string(*oml_value_get_value(&v[2]), s2);
  marshal_values(mbuf, v, 3);
  marshal_finalize(mbuf);
  oml_value_array_reset(v, 3);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));
  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
  fail_unless(ch->table_count == 3, "Unexpected number of tables (%d instead of 3)", ch->table_count);

  logdebug("Sending second sample\n");
  oml_value_set_type(&v[0], OML_UINT32_VALUE);
  omlc_set_uint32(*oml_value_get_value(&v[0]), d2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 2, 1, time2);
  marshal_values(mbuf, &v[0], 1);
  marshal_finalize(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));

  logdebug("Overwriting schema: '%s'\n", s3);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 0, 1, time2); /* XXX: We should probably not ignore time or sequence */
  oml_value_set_type(&v[0], OML_STRING_VALUE);
  oml_value_set_type(&v[1], OML_STRING_VALUE);
  oml_value_set_type(&v[2], OML_STRING_VALUE);
  omlc_set_const_string(*oml_value_get_value(&v[0]), ".");
  omlc_set_const_string(*oml_value_get_value(&v[1]), "schema");
  omlc_set_const_string(*oml_value_get_value(&v[2]), s3);
  marshal_values(mbuf, v, 3);
  marshal_finalize(mbuf);
  oml_value_array_reset(v, 3);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));
  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
  fail_unless(ch->table_count == 3, "Unexpected number of tables (%d instead of 3)", ch->table_count);

  logdebug("Sending third sample\n");
  oml_value_set_type(&v[0], OML_INT32_VALUE);
  omlc_set_int32(*oml_value_get_value(&v[0]), d1);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 1, time1);
  marshal_values(mbuf, &v[0], 1);
  marshal_finalize(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));

  database_release(ch->database);
  check_server_destroy_client_handler(ch);
  mbuf_destroy(mbuf);

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

START_TEST(test_binary_metadata)
{
  /* XXX: Code duplication with check_binary_protocol.c:test_binary_metadata*/
  ClientHandler *ch;
  Database *db;
  sqlite3_stmt *stmt;
  SockEvtSource source;
  MBuffer* mbuf = mbuf_create();

  char domain[] = "binary-meta-test";
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
  char select[200];

  int rc = -1;

  OmlValue v[3];
  oml_value_array_init(v, 3);

  /* Remove pre-existing databases */
  *dbname=0;
  snprintf(dbname, sizeof(dbname), "%s.sq3", domain);
  unlink(dbname);

  o_set_log_level(-1);
  logdebug("%s\n", __FUNCTION__);

  snprintf(s1, sizeof(s1), "1 %s size:uint32", table[0]);
  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\nschema: %s\ncontent: binary\nschema: %s\n\n",
      domain, basename(__FILE__), __FUNCTION__, s0, s1);
  snprintf(select, sizeof(select), "select key, value, subject from _experiment_metadata;");

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "bin meta socket";
  ch = check_server_prepare_client_handler("test_bin_meta", &source);
  fail_unless(ch->state == C_HEADER);
  fail_unless(ch->table_count == 0, "Unexpected number of tables (%d instead of 0)", ch->table_count);

  logdebug("Sending header '%s'\n", h);
  client_callback(&source, ch, h, strlen(h));

  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
  fail_unless(ch->content == C_BINARY_DATA);
  fail_if(ch->database == NULL);
  fail_if(ch->sender_id == 0);
  fail_if(ch->sender_name == NULL);
  fail_if(ch->app_name == NULL);
  fail_unless(ch->table_count == 2, "Unexpected number of tables (%d instead of 2)", ch->table_count);

  *subject=0;

  strcat(subject, ".");
  logdebug("Sending first meta '%s %s %s'\n", subject, k1, v1);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 0, 1, time1);
  oml_value_set_type(&v[0], OML_STRING_VALUE);
  oml_value_set_type(&v[1], OML_STRING_VALUE);
  oml_value_set_type(&v[2], OML_STRING_VALUE);
  omlc_set_const_string(*oml_value_get_value(&v[0]), subject);
  omlc_set_const_string(*oml_value_get_value(&v[1]), k1);
  omlc_set_const_string(*oml_value_get_value(&v[2]), v1);
  marshal_values(mbuf, v, 3);
  marshal_finalize(mbuf);
  oml_value_array_reset(v, 3);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));
  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);

  logdebug("Sending second meta '%s %s %s'\n", subject, k2, v2);
  strcat(subject, mp1);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 0, 1, time1);
  oml_value_set_type(&v[0], OML_STRING_VALUE);
  oml_value_set_type(&v[1], OML_STRING_VALUE);
  oml_value_set_type(&v[2], OML_STRING_VALUE);
  omlc_set_const_string(*oml_value_get_value(&v[0]), subject);
  omlc_set_const_string(*oml_value_get_value(&v[1]), k2);
  omlc_set_const_string(*oml_value_get_value(&v[2]), v2);
  marshal_values(mbuf, v, 3);
  marshal_finalize(mbuf);
  oml_value_array_reset(v, 3);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));
  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);

#if DB_HAS_PKEY /* #814 */
  logdebug("Sending third meta '%s %s %s'\n", subject, k1, v2);
  strcat(subject, ".");
  strcat(subject, f1);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 0, 1, time1);
  oml_value_set_type(&v[0], OML_STRING_VALUE);
  oml_value_set_type(&v[1], OML_STRING_VALUE);
  oml_value_set_type(&v[2], OML_STRING_VALUE);
  omlc_set_const_string(*oml_value_get_value(&v[0]), subject);
  omlc_set_const_string(*oml_value_get_value(&v[1]), k1);
  omlc_set_const_string(*oml_value_get_value(&v[2]), v1);
  marshal_values(mbuf, v, 3);
  marshal_finalize(mbuf);
  oml_value_array_reset(v, 3);
  printmbuf(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_fill(mbuf));
  fail_unless(ch->state == C_BINARY_DATA, "Inconsistent state: expected %d, got %d", C_BINARY_DATA, ch->state);
#endif

  database_release(ch->database);
  check_server_destroy_client_handler(ch);
  mbuf_destroy(mbuf);

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

Suite* binary_protocol_suite (void)
{
  Suite* s = suite_create ("Binary protocol");

  dbbackend = "sqlite";
  sqlite_database_dir = ".";

  TCase *tc_bin_sync = tcase_create ("Sync");
  tcase_add_test (tc_bin_sync, test_find_sync);
  tcase_add_test (tc_bin_sync, test_bin_find_sync);
  tcase_add_test (tc_bin_sync, test_binary_resync);
  suite_add_tcase (s, tc_bin_sync);

  TCase* tc_bin_flex = tcase_create ("Binary flexibility");
  tcase_add_test (tc_bin_flex, test_binary_flexibility);
  tcase_add_test (tc_bin_flex, test_binary_metadata);
  suite_add_tcase (s, tc_bin_flex);

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
