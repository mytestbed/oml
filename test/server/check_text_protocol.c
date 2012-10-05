/*
 * Copyright 2012-2013 National ICT Australia (NICTA), Australia
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
/** \file Tests behaviour and issues related to the text protocol. */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <check.h>
#include <sqlite3.h>
#include <libgen.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mbuf.h"
#include "database.h"
#include "client_handler.h"
#include "sqlite_adapter.h"

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

  snprintf(h, sizeof(h),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\nschema: 1 %s size:uint32\n\n", domain, basename(__FILE__), __FUNCTION__, table);
  snprintf(s1, sizeof(s1), "%f\t1\t%d\t%d\n", time1, 1, d1);
  snprintf(s2, sizeof(s2), "%f\t1\t%d\t%d\n", time2, 2, d2);
  snprintf(select, sizeof(select), "select oml_ts_client, oml_seq, size from %s;", table);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "text insert socket";

  o_set_log_level(-1);

  /* Create the ClientHandler almost as server/client_handler.h:client_handler_new() would */
  ch = (ClientHandler*) xmalloc(sizeof(ClientHandler));
  ch->state = C_HEADER;
  ch->content = C_TEXT_DATA;
  ch->mbuf = mbuf_create ();
  ch->socket = NULL;
  ch->event = &source;
  strncpy (ch->name, "test_text_insert", MAX_STRING_SIZE);

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

  /* Close all */
  client_handler_free(ch);
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

  /* Add tests to test case "FilterCore" */
  tcase_add_test (tc_text_insert, test_text_insert);

  /* Add the test cases to this test suite */
  suite_add_tcase (s, tc_text_insert);

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
