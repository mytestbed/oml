/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
#ifndef DATABASE_H_
#define DATABASE_H_

#include <oml2/omlc.h>

#define MAX_DB_NAME_SIZE 64
#define MAX_TABLE_NAME_SIZE 64
#define MAX_COL_NAME_SIZE 64

struct _database;
struct _dbTable;
extern char* g_database_data_dir;

typedef int (*db_adapter_create)(struct _database *db);
typedef void (*db_adapter_release)(struct _database *db);
typedef int (*db_adapter_create_table)(struct _database *db, struct _dbTable *table);
typedef int (*db_adapter_insert)(struct _database* db, struct _dbTable* table,
                                 int sender_id, int seq_no, double time_stamp,
                                 OmlValue* values, int value_count);
typedef char* (*db_adapter_get_metadata) (struct _database* db, const char* key);
typedef int   (*db_adapter_set_metadata) (struct _database* db, const char* key, const char* value);
typedef int (*db_add_sender_id)(struct _database* db, char* sender_id);
typedef long (*db_get_time_offset)(struct _database* db, long start_time);
typedef long (*db_adapter_get_max_seq_no) (struct _database* db, struct _dbTable* table, int sender_id);

typedef struct _dbColumn {
  char       name[MAX_COL_NAME_SIZE];
  OmlValueT  type;
} DbColumn;

typedef struct _dbTable {
  char       name[MAX_TABLE_NAME_SIZE];
  DbColumn** columns;
  int        col_size;
  void*      adapter_hdl;
  struct _dbTable* next;
} DbTable;

typedef struct _database{
  char       name[MAX_DB_NAME_SIZE];
  char       hostname[MAX_DB_NAME_SIZE];
  char       user[MAX_DB_NAME_SIZE];

  int        ref_count;   // count number of clients using this DB

  DbTable*   first_table;

  void*      adapter_hdl;
  db_adapter_create create;
  db_adapter_create_table create_table;
  db_adapter_release release;
  db_adapter_insert  insert;
  db_adapter_get_metadata get_metadata;
  db_adapter_set_metadata set_metadata;
  db_add_sender_id   add_sender_id;
  db_adapter_get_max_seq_no get_max_seq_no;

  long               start_time;

  struct _database* next;
} Database;

/*
 *  Return the database instance for +name+.
 * If no database with this name exists, a new
 * one is created;
 */
Database*
database_find(char* name, char* hostname, char* user);

/**
 * Client no longer uses this database. If this
 * was the last client checking out, close database.
 */
void
database_release(Database* database);

DbTable*
database_get_table(Database* database, char* schema);

void
database_table_free(DbTable* table);

void
database_table_add_col (DbTable* table, const char* name, OmlValueT type, int index);

void
database_table_store_col(DbTable* table, DbColumn* col, int index);

#endif /*DATABASE_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
