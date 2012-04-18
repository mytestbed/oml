/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
#include "table_descr.h"
#include "schema.h"

#define MAX_DB_NAME_SIZE 64
#define MAX_TABLE_NAME_SIZE 64
#define MAX_COL_NAME_SIZE 64

/**
 * Limit on the number of times the server tries to generate a table name for
 * a different schema.
 *
 * XXX: If this is changed, make sure enough memory is allocated in
 * database_find_or_create_table() to hold the new table name in the schema
 * structure.
 */
#define MAX_TABLE_RENAME 10

struct _database;
struct _dbTable;
typedef struct _dbTable DbTable;
typedef struct _database Database;

typedef int (*db_adapter_create)(Database *db);
typedef void (*db_adapter_release)(Database *db);
typedef int (*db_adapter_table_create)(Database *db, DbTable *table, int thin);
typedef int (*db_adapter_table_create_meta)(Database *db, const char *name);
typedef int (*db_adapter_table_free)(Database *db, DbTable *table);
typedef int (*db_adapter_insert)(Database *db, DbTable* table,
                                 int sender_id, int seq_no, double time_stamp,
                                 OmlValue* values, int value_count);
typedef char* (*db_adapter_get_metadata) (Database* db, const char* key);
typedef int   (*db_adapter_set_metadata) (Database* db, const char* key, const char* value);
typedef int (*db_add_sender_id)(Database* db, char* sender_id);
typedef TableDescr* (*db_adapter_get_table_list) (Database* db, int *num_tables);

struct _dbTable {
  struct schema *schema;
  void*      handle;
  struct _dbTable* next;
};

struct _database{
  char       name[MAX_DB_NAME_SIZE];

  int        ref_count;   // count number of clients using this DB
  DbTable*   first_table;
  time_t     start_time;
  void*      handle;

  db_adapter_create create;
  db_adapter_release release;
  db_adapter_table_create table_create;
  db_adapter_table_create_meta table_create_meta;
  db_adapter_table_free table_free;
  db_adapter_insert  insert;
  db_adapter_get_metadata get_metadata;
  db_adapter_set_metadata set_metadata;
  db_add_sender_id   add_sender_id;
  db_adapter_get_table_list get_table_list;

  struct _database* next;
};

/*
 *  Return the database instance for +name+.
 * If no database with this name exists, a new
 * one is created;
 */
Database *database_find(char* name);

/**
 * Client no longer uses this database. If this
 * was the last client checking out, close database.
 */
void database_release(Database* database);

DbTable *database_find_table(Database* database, const char* name);
DbTable *database_find_or_create_table(Database *database, struct schema *schema);
DbTable *database_create_table (Database *database, struct schema *schema);
void     database_table_free(Database *database, DbTable* table);

#endif /*DATABASE_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
