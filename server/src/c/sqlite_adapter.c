/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
#include <string.h>
#include <stdio.h>
#include <malloc.h>
#include <sqlite3.h>
#include <ocomm/o_log.h>
#include <time.h>
#include <sys/time.h>
#include "database.h"

typedef struct _sq3DB {
  sqlite3*  db_hdl;
  int       sender_cnt;
} Sq3DB;

typedef struct _sq3Table {
  sqlite3_stmt* insert_stmt;  // prepared insert statement
} Sq3Table;

static int
sql_stmt(Sq3DB* self, const char* stmt);

static int
sq3_insert(
  Database* db, DbTable*  table,
  int sender_id, int seq_no, double ts,
  OmlValue* values, int value_count
);
static int
sq3_add_sender_id(Database* db, char* sender_id);

// FIXME:  why is this defined but not used?
//static sqlite3* db;
static int first_row;
/**
 * \fn int sq3_create_database(Database* db)
 * \brief Create a sqlite3 database
 * \param db the databse to associate with the sqlite3 database
 * \return 0 if successful, -1 otherwise
 */
int
sq3_create_database(
  Database* db
) {

  char fname[128];
  sqlite3* db_hdl;
  int rc;
  snprintf(fname, 127, "%s.sq3", db->name);
  rc = sqlite3_open(fname, &db_hdl);
  if (rc) {
    o_log(O_LOG_ERROR, "Can't open database: %s\n", sqlite3_errmsg(db_hdl));
    sqlite3_close(db_hdl);
    return -1;
  }

  Sq3DB* self = (Sq3DB*)malloc(sizeof(Sq3DB));
  memset(self, 0, sizeof(Sq3DB));
  self->db_hdl = db_hdl;
  db->insert = sq3_insert;
  db->add_sender_id = sq3_add_sender_id;

  db->adapter_hdl = self;


//  char s[512];  // don't check for length
  //char* s= "CREATE TABLE _senders (id INTEGER PRIMARY KEY, name TEXT);";
  //if (sql_stmt(self, s)) return -1;

  return 0;
}
/**
 * \fn void sq3_release(Database* db)
 * \brief Release the sqlite3 database
 * \param db the database that contains the sqlite3 database
 */
void
sq3_release(
  Database* db
) {
  Sq3DB* self = (Sq3DB*)db->adapter_hdl;
  sqlite3_close(self->db_hdl);
  // TODO: Release tabel specific data

  free(self);
  db->adapter_hdl = NULL;
}
/**
 * \fn static int sq3_add_sender_id(Database* db, char* sender_id)
 * \brief  Add sender ID to the table
 * \param db the database that contains the sqlite3 db
 * \param sender_id the sender ID
 * \return the index of the sender
 */
static int
sq3_add_sender_id(
  Database* db,
  char*     sender_id
) {
  Sq3DB* self = (Sq3DB*)db->adapter_hdl;
  int index = ++self->sender_cnt;

  // FIXME:  Why is this code commented out?  Function doesn't seem to do much of interest otherwise.
  //char s[512];  // don't check for length
  // TDEBUG
  //sprintf(s, "INSERT INTO _senders VALUES (%d, '%s');\0", index, sender_id);
  //if (sql_stmt(self, s)) return -1;
  return index;
}
/**
 * \fn int sq3_create_table(Database* db, DbTable* table)
 * \brief Create a sqlite3 table
 * \param db the database that contains the sqlite3 db
 * \param table the table to associate in sqlite3 database
 * \return 0 if successful, -1 otherwise
 */
int
sq3_create_table(
  Database* db,
  DbTable* table
) {
  if (table->columns == NULL) {
    o_log(O_LOG_WARN, "No cols defined for table '%s'\n", table->name);
    return -1;
  }

  int max = table->col_size;

//  BEGIN TRANSACTION;
//       CREATE TABLE t1 (t1key INTEGER
//                    PRIMARY KEY,data TEXT,num double,timeEnter DATE);
//       INSERT INTO "t1" VALUES(1, 'This is sample data', 3, NULL);
//       INSERT INTO "t1" VALUES(2, 'More sample data', 6, NULL);
//       INSERT INTO "t1" VALUES(3, 'And a little more', 9, NULL);
//       COMMIT;
//

  char create[512];  // don't check for length
  char insert[512];  // don't check for length

  char* cs = create;
  sprintf(cs, "CREATE TABLE %s (oml_sender_id INTEGER, oml_seq INTEGER, oml_ts_client REAL, oml_ts_server REAL", table->name);
  cs += strlen(cs);

  char* is = insert;
  sprintf(is, "INSERT INTO %s VALUES (?, ?, ?, ?", table->name);
  is += strlen(is);

  int first = 0;
  DbColumn* col = table->columns[0];
  int i = 0;
  while (col != NULL && max > 0) {
    char* t;
    col = table->columns[i];
    switch (col->type) {
      case OML_LONG_VALUE: t = "INTEGER"; break;
      case OML_DOUBLE_VALUE: t = "REAL"; break;
      case OML_STRING_VALUE: t = "TEXT"; break;
      default:
        o_log(O_LOG_ERROR, "Bug: Unknown type %d in col '%s'\n", col->type, col->name);
    }
    if (first) {
      sprintf(cs, "%s %s", col->name, t);
      sprintf(is, "?");
      first = 0;
    } else {
      sprintf(cs, ", %s %s", col->name, t);
      sprintf(is, ", ?");
    }
    cs += strlen(cs);
    is += strlen(is);
    i++;
    max--;
  }
  sprintf(cs, ");");
  sprintf(is, ");");
  o_log(O_LOG_DEBUG, "schema: %s\n", create);
  o_log(O_LOG_DEBUG, "insert: %s\n", insert);

  Sq3DB* sq3db = (Sq3DB*)db->adapter_hdl;

  if (sql_stmt(sq3db, create)) return -1;

  Sq3Table* sq3table = (Sq3Table*)malloc(sizeof(Sq3Table));
  memset(sq3table, 0, sizeof(Sq3Table));
  table->adapter_hdl = sq3table;
  if (sqlite3_prepare_v2(sq3db->db_hdl, insert, -1, &sq3table->insert_stmt, 0) != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Could not prepare statement (%s).\n", sqlite3_errmsg(sq3db->db_hdl));
    return -1;
  }
  return 0;
}
/**
 * \fn static intsq3_insert(Database* db, DbTable*  table, int sender_id, int seq_no, double time_stamp, OmlValue* values, int value_count)
 * \brief Insert value in the sqlite3 database
 * \param db the database that contains the sqlite3 db
 * \param table the table to insert data in
 * \param sender_id the sender ID
 * \param seq_no the sequence number
 * \param time_stamp the timestamp of the receiving data
 * \param values the values to insert
 * \param value_count the number of values
 * \return 0 if successful, -1 otherwise
 */
static int
sq3_insert(
  Database* db,
  DbTable*  table,
  int       sender_id,
  int       seq_no,
  double    time_stamp,
  OmlValue* values,
  int       value_count
) {
  Sq3DB* sq3db = (Sq3DB*)db->adapter_hdl;
  Sq3Table* sq3table = (Sq3Table*)table->adapter_hdl;
  int i;
  double time_stamp_server;
  sqlite3_stmt* stmt = sq3table->insert_stmt;
  //o_log(O_LOG_DEBUG, "insert");
  o_log(O_LOG_DEBUG2, "sq3_insert(%s): insert row %d \n",
	table->name, seq_no);

  if (sqlite3_bind_int(stmt, 1, sender_id) != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Could not bind 'oml_sender_id' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }
  if (sqlite3_bind_int(stmt, 2, seq_no) != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Could not bind 'oml_seq' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }
  if (sqlite3_bind_double(stmt, 3, time_stamp) != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Could not bind 'oml_ts_client' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_stamp_server = tv.tv_sec - db->start_time + 0.000001 * tv.tv_usec;
  if (sqlite3_bind_double(stmt, 4, time_stamp_server) != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Could not bind 'oml_ts_server' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }

  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    DbColumn* col = table->columns[i];
    if (v->type != col->type) {
      o_log(O_LOG_ERROR, "Mismatch in %dth value type fo rtable '%s'\n", i, table->name);
      return -1;
    }
    int res;
    switch (col->type) {
      case OML_LONG_VALUE:
        res = sqlite3_bind_int(stmt, i + 5, (int)v->value.longValue);
        break;
      case OML_DOUBLE_VALUE:
        res = sqlite3_bind_double(stmt, i + 5, v->value.doubleValue);
        break;
      case OML_STRING_VALUE:
        res = sqlite3_bind_text (stmt, i + 5, v->value.stringValue.ptr,
                -1, SQLITE_TRANSIENT);
        break;
      default:
        o_log(O_LOG_ERROR, "Bug: Unknown type %d in col '%s'\n", col->type, col->name);
        return -1;
    }
    if (res != SQLITE_OK) {
      o_log(O_LOG_ERROR, "Could not bind column '%s' (%s).\n",
          col->name, sqlite3_errmsg(sq3db->db_hdl));
    }
  }
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    o_log(O_LOG_ERROR, "Could not step (execute) stmt.\n");
    return -1;
  }
  return sqlite3_reset(stmt);
}

/**
 * \fn static int select_callback(void*  p_data, int    num_fields, char** p_fields, char** p_col_names)
 * \brief
 * \param p_data
 * \param num_fields
 * \param p_fields
 * \param p_col_names
 * \return
 */
static int
select_callback(
  void*  p_data,
  int    num_fields,
  char** p_fields,
  char** p_col_names
) {

  int i;

  int* nof_records = (int*) p_data;
  (*nof_records)++;

  if (first_row) {
    first_row = 0;

    for (i=0; i < num_fields; i++) {
      printf("%20s", p_col_names[i]);
    }

    printf("\n");
    for (i=0; i< num_fields*20; i++) {
      printf("=");
    }
    printf("\n");
  }

  for(i=0; i < num_fields; i++) {
    if (p_fields[i]) {
      printf("%20s", p_fields[i]);
    }
    else {
      printf("%20s", " ");
    }
  }

  printf("\n");
  return 0;
}
/**
 * \fn static int select_stmt(Sq3DB* self, const char* stmt)
 * \brief Prespare sqlite statement
 * \param self the sqlite3 database
 * \param stmt the statement to prepare
 * \return 0 if successfull, -1 otherwise
 */
static int
select_stmt(
  Sq3DB* self,
  const char* stmt
) {
  char* errmsg;
  int   ret;
  int   nrecs = 0;
  o_log(O_LOG_DEBUG, "prepare to exec 1 \n");
  first_row = 1;

  ret = sqlite3_exec(self->db_hdl, stmt, select_callback, &nrecs, &errmsg);

  if (ret != SQLITE_OK) {
    o_log(O_LOG_ERROR, "Error in select statement %s [%s].\n", stmt, errmsg);
    return -1;
  }
  return nrecs;
}
/**
 * \fn static int sql_stmt(Sq3DB* self, const char* stmt)
 * \brief Execute sqlite3 statement
 * \param self the sqlite3 database
 * \param stmt the statement to prepare
 * \return 0 if successfull, -1 otherwise
 */
static int
sql_stmt(
  Sq3DB* self,
  const char* stmt
) {
  char *errmsg;
  int   ret;
  o_log(O_LOG_DEBUG, "prepare to exec %s \n", stmt);
  ret = sqlite3_exec(self->db_hdl, stmt, 0, 0, &errmsg);

  if (ret != SQLITE_OK) {
    o_log(O_LOG_WARN, "Error in statement: %s [%s].\n", stmt, errmsg);
    return -1;
  }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
