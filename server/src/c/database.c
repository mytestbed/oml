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
//! Implements the interface to a local database

#include <malloc.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <ocomm/o_log.h>

#include "database.h"


#define DEF_COLUMN_COUNT 1
#define DEF_TABLE_COUNT 1

static int
parse_col_decl(DbTable* self, char* col_decl, int index, int check_only);
static void
table_free(DbTable* table);
static void
store_col(DbTable* table, DbColumn* col, int index);

static Database* firstDB = NULL;
/**
 * \fn Database* database_find(char* name)
 * \brief create a date with the name +name+
 * \param name the name of the database
 * \return a new database
 */
Database*
database_find(
    char* name
) {
  Database* db = firstDB;
  while (db != NULL) {
    if (strcmp(name, db->name) == 0) {
      db->ref_count++;
      return db;
    }
  }

  // need to create a new one
  Database* self = (Database *)malloc(sizeof(Database));
  memset(self, 0, sizeof(Database));
  o_log(O_LOG_DEBUG, "Creation of the database %s\n", name);
  strncpy(self->name, name, MAX_DB_NAME_SIZE);
  self->ref_count = 1;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  self->start_time = tv.tv_sec;

  // hook this one into the list of active databases
  self->next = firstDB;
  firstDB = self;

  if (sq3_create_database(self)) {
    free(self);
    return NULL;
  }

  return self;
}
/**
 * \fn void database_release(Database* self)
 * \brief Client no longer uses this database. If this was the last client checking out, close database.
 * \param self the database to release
 */
void
database_release(
  Database* self
) {
  if (--self->ref_count > 0) return; // still in use

  // unlink DB
  Database* db_p = firstDB;
  Database* prev_p = NULL;
  while (db_p != NULL && db_p != self) {
    prev_p = db_p;
    db_p = db_p->next;
  }
  if (db_p == NULL) {
    o_log(O_LOG_ERROR, "BUG: Releasing to unknown database\n");
    return;
  }
  if (prev_p == NULL) {
    firstDB = self->next; // was first
  } else {
    prev_p->next = self->next;
  }
  o_log(O_LOG_INFO, "Closing database '%s'\n", self->name);
  sq3_release(self);

  // no longer needed
  DbTable* t_p = self->first_table;
  while (t_p != NULL) {
    table_free(t_p);
    t_p = t_p->next;
  }
  free(self);
}
/**
 * \fn DbTable* database_get_table(Database* database, char* schema)
 * \brief get a table from the database
 * \param database the database to extract the table
 * \param schema name of the table
 * \return the table of or NULL if not successful
 */
DbTable*
database_get_table(
    Database* database,
    char* schema
) {
  if (database == NULL) return NULL;
  // table name
  char* p = schema;
  while (*p == ' ') p++; // skip white space
  char* tname = p;
  while (*p != ' ' && *p != '\0') p++;
  *(p++) = '\0';
  o_log(O_LOG_DEBUG, "Table name '%s'\n", tname);

  // Check if table already exists
  int check_only = 0; // only check col decl if table already exists
  DbTable* table = database->first_table;
  while (table != NULL) {
    if (strcmp(table->name, tname) == 0) {
      // table already exists
      check_only = 1;
      break;
    }
    table = table->next;
  }
  if (table == NULL) {
    table = (DbTable*)malloc(sizeof(DbTable));
    memset(table, 0, sizeof(DbTable));
    strncpy(table->name, tname, MAX_TABLE_NAME_SIZE);
  }

  int index = 0;
  while (*p != '\0') {
    while (*p == ' ') p++; // skip white space
    char* col = p;
    while (*p != ' ' && *p != '\0') p++;
    if (*p != '\0') *(p++) = '\0';
    parse_col_decl(table, col, index, check_only);
    o_log(O_LOG_DEBUG, "Column name '%s'\n", col);
    index++;
  }
  if (!check_only) {
    if (sq3_create_table(database, table)) {
      free(table);
      return NULL;
    }
    table->next = database->first_table;
    database->first_table = table;
  }
  return table;
}
/**
 * \fn static int parse_col_decl( DbTable* self, char* col_decl, int index, int check_only)
 * \brief Create a column in the database
 * \param self the database where we create the column
 * \param col_decl the name of the column
 * \param index the index of the column
 * \param check_only don't create col, just check it
 * \return 1 if successful 0 otherwise
 */
static int
parse_col_decl(
  DbTable* self,
  char*    col_decl,
  int      index,
  int      check_only
) {
  char* name = col_decl;
  char* p = name;
  while (*p != ':' && *p != '\0') p++;
  if (*p == '\0') {
    o_log(O_LOG_WARN, "Malformed schema type '%s'\n", name);
  }
  *(p++) = '\0';
  char* typeS = p;
  OmlValueT type;

  if (strcmp(typeS, "string") == 0) {
    type = OML_STRING_VALUE;
  } else if (strcmp(typeS, "long") == 0) {
    type = OML_LONG_VALUE;
  } else if (strcmp(typeS, "double") == 0) {
    type = OML_DOUBLE_VALUE;
  } else {
    o_log(O_LOG_ERROR, "Unknown column type '%s'\n", typeS);
    return 0;
  }

  DbColumn* col;

  if (check_only) {
    col = self->columns[index];
    if (col == NULL || strcmp(col->name, name) != 0 || col->type != type) {
      o_log(O_LOG_WARN, "Column '%s' of table '%s' different to previous declarations'\n",
          name, self->name);
      return 0;
    }
  } else {
    col = (DbColumn*)malloc(sizeof(DbColumn));
    memset(col, 0, sizeof(DbColumn));
    strncpy(col->name, name, MAX_COL_NAME_SIZE);
    col->type = type;
    store_col(self, col, index);
  }
  return 1;
}
/**
 * \fn static void store_col( DbTable*  table, DbColumn* col, int index)
 * \brief store the column in the databse
 * \param table the table of the
 * \param col the column to store
 * \param index the index of the column
 */
static void
store_col(
  DbTable*  table,
  DbColumn* col,
  int       index
) {
  if (index >= table->col_size) {
    DbColumn** old = table->columns;
    int old_count = table->col_size;

    table->col_size += DEF_COLUMN_COUNT;
    table->columns = (DbColumn**)malloc(table->col_size * sizeof(DbColumn*));
    int i;
    for (i = old_count - 1; i >= 0; i--) {
      table->columns[i] = old[i];
    }
  }
  table->columns[index] = col;
}
/**
 * \fn static void table_free(DbTable* table)
 * \brief Free the table
 * \param table the table to free
 */
static void
table_free(
  DbTable* table
) {
  if (table->columns != NULL) {
    DbColumn* c;
    int i;

    for (i = 0; i < table->col_size; i++) {
      if ((c = table->columns[i]) == NULL) {
        break; // reached end
      }
      free(c);
    }
    free(table->columns);
  }
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
