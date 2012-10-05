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
#include <math.h>
#include <check.h>
#include <sqlite3.h>
#include <libgen.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mbuf.h"
#include "oml_value.h"
#include "marshal.h"
#include "binary.h"
#include "database.h"
#include "client_handler.h"
#include "sqlite_adapter.h"

extern char *dbbackend;
extern char *sqlite_database_dir;

/* Prototypes of functions to test */

void client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

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
  char table[] = "resync1_table";
  double time1 = 1.096202;
  double time2 = 2.092702;
  int32_t d1 = 3319660544;
  int32_t d2 = 106037248;

  char h1[200];
  char select1[200];

  OmlValue v;

  int rc = -1;

  snprintf(h1, sizeof(h1),  "protocol: 4\ndomain: %s\nstart-time: 1332132092\nsender-id: %s\napp-name: %s\ncontent: binary\nschema: 1 %s size:uint32\n\n", domain, basename(__FILE__), __FUNCTION__, table);
  snprintf(select1, sizeof(select1), "select oml_ts_client, oml_seq, size from %s;", table);

  memset(&source, 0, sizeof(SockEvtSource));
  source.name = "binary resync socket";

  o_set_log_level(-1);

  /* Create the ClientHandler almost as server/client_handler.h:client_handler_new() would */
  ch = (ClientHandler*) xmalloc(sizeof(ClientHandler));
  ch->state = C_HEADER;
  ch->content = C_TEXT_DATA; /* That's how client_handler_new() does it */
  ch->mbuf = mbuf_create ();
  ch->socket = NULL;
  ch->event = &source;
  strncpy (ch->name, "test_binary_resync", MAX_STRING_SIZE);

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
  v.type = OML_UINT32_VALUE;
  omlc_set_uint32(v.value, d1);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 1, time1);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
#define printmbuf  logdebug("Sending message (in %p, at %p, from %p): %x %x %x %x  %x %x %x %x  %x %x %x %x  %x %x %x %x  %x %x %x %x  %x %x %x %x\n", \
      (mbuf_buffer(mbuf)),                       \
      (mbuf_message(mbuf)),                      \
      (mbuf_rdptr(mbuf)),                      \
      *((uint8_t*)mbuf_message(mbuf)),                     \
      *((uint8_t*)mbuf_message(mbuf)+1),   \
      *((uint8_t*)mbuf_message(mbuf)+2),   \
      *((uint8_t*)mbuf_message(mbuf)+3),   \
      *((uint8_t*)mbuf_message(mbuf)+4),   \
      *((uint8_t*)mbuf_message(mbuf)+5),   \
      *((uint8_t*)mbuf_message(mbuf)+6),   \
      *((uint8_t*)mbuf_message(mbuf)+7),   \
      *((uint8_t*)mbuf_message(mbuf)+8),   \
      *((uint8_t*)mbuf_message(mbuf)+9),   \
      *((uint8_t*)mbuf_message(mbuf)+10),  \
      *((uint8_t*)mbuf_message(mbuf)+11),  \
      *((uint8_t*)mbuf_message(mbuf)+12),  \
      *((uint8_t*)mbuf_message(mbuf)+13),  \
      *((uint8_t*)mbuf_message(mbuf)+14),  \
      *((uint8_t*)mbuf_message(mbuf)+15),  \
      *((uint8_t*)mbuf_message(mbuf)+16),  \
      *((uint8_t*)mbuf_message(mbuf)+17),  \
      *((uint8_t*)mbuf_message(mbuf)+18),  \
      *((uint8_t*)mbuf_message(mbuf)+19),  \
      *((uint8_t*)mbuf_message(mbuf)+20),  \
      *((uint8_t*)mbuf_message(mbuf)+21),  \
      *((uint8_t*)mbuf_message(mbuf)+22),  \
      *((uint8_t*)mbuf_message(mbuf)+23)   \
      );
  printmbuf;
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_remaining(mbuf));

  logdebug("Sending second sample for an invalid table, in two steps\n");
  v.type = OML_UINT32_VALUE;
  omlc_set_uint32(v.value, d2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 2, 1, time2);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
  client_callback(&source, ch, mbuf_buffer(mbuf), 5);
  fail_if(ch->state == C_PROTOCOL_ERROR, "An incomplete sample confused the client_handler");
  printmbuf;
  client_callback(&source, ch, mbuf_buffer(mbuf)+5, mbuf_remaining(mbuf)-5);
  fail_if(ch->state == C_PROTOCOL_ERROR, "A sample for a non existing stream confused the client_handler");

  logdebug("Sending some noise\n");
  mbuf_clear(mbuf);
  mbuf_write(mbuf, (uint8_t*)"BRuit", 6);
  client_callback(&source, ch, mbuf_buffer(mbuf), mbuf_remaining(mbuf));
  fail_if(ch->state == C_PROTOCOL_ERROR, "Some more noise disturbed the client_handler");

  logdebug("Sending third sample, in two steps\n");
  v.type = OML_UINT32_VALUE;
  omlc_set_uint32(v.value, d2);
  mbuf_clear(mbuf);
  marshal_init(mbuf, OMB_DATA_P);
  marshal_measurements(mbuf, 1, 2, time2);
  marshal_values(mbuf, &v, 1);
  marshal_finalize(mbuf);
  printmbuf;
  client_callback(&source, ch, mbuf_buffer(mbuf), 5);
  fail_if(ch->state == C_PROTOCOL_ERROR, "An incomplete sample after resync confused the client_handler");
  client_callback(&source, ch, mbuf_buffer(mbuf)+5, mbuf_remaining(mbuf)-5);

  fail_unless(ch->state == C_BINARY_DATA, "The client handler didn't manage to recover");

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
      "Invalid oml_ts_value in 2nd table: expected `%e', got `%e'",
      time2, sqlite3_column_double(stmt, 0), fabs(sqlite3_column_double(stmt, 0) - time2),
      fabs(sqlite3_column_double(stmt, 0) - time2)
      );
  fail_unless(sqlite3_column_int(stmt, 2) - d2 == 0,
      "Invalid size in 2nd table: expected `%d', got `%d'", d2, sqlite3_column_double(stmt, 2));

  /* Close all */
  client_handler_free(ch);
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
