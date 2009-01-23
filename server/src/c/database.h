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
#ifndef DATABASE_H_
#define DATABASE_H_

#include <oml2/omlc.h>

#define MAX_DB_NAME_SIZE 64
#define MAX_TABLE_NAME_SIZE 64
#define MAX_COL_NAME_SIZE 64

struct _database;
struct _dbTable;

    /*! Called to calculate the final measurements, send the results to
 * a stream and reset the internal state for a new sampling period.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int (*db_adapter_insert)(
    struct _database* db,
    struct _dbTable*  table,
    int               sender_id,
    int               seq_no,
    double            time_stamp,
    OmlValue*         values,
    int               value_count
);

/*! Add the name of a sender to the 'sender' table
 * and return its id for refernce in the respective
 * measurement table.
 */
typedef int (*db_add_sender_id)(
    struct _database* db,
    char*             sender_id
);

typedef long (*db_get_time_offset)(
    struct _database* db,
    long              start_time
);

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

  int        ref_count;   // count number of clients using this DB

  DbTable*   first_table;

  void*      adapter_hdl;
  db_adapter_insert  insert;

  db_add_sender_id   add_sender_id;

  long               start_time;

  struct _database* next;
} Database;





/*
 *  Return the database instance for +name+.
 * If no database with this name exists, a new
 * one is created;
 */
Database*
database_find(
    char* name
);

/**
 * Client no longer uses this database. If this
 * was the last client checking out, close database.
 */
void
database_release(
  Database* database
);

DbTable*
database_get_table(
    Database* database,
    char* schema
);

#endif /*DATABASE_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
