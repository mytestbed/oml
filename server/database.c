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
//! Implements the interface to a local database

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <log.h>
#include <assert.h>
#include "sqlite_adapter.h"
#include "database.h"
#include "util.h"
#include "oml_value.h"

#define DEF_COLUMN_COUNT 1
#define DEF_TABLE_COUNT 1

static Database *first_db = NULL;
/**
 * \brief create a date with the name +name+
 * \param name the name of the database
 * \return a new database
 */
Database*
database_find(char* name, char* hostname, char* user)
{
  Database* db = first_db;
  while (db != NULL) {
      if (strcmp(name, db->name) == 0 && strcmp(hostname, db->hostname) == 0 && strcmp(user, db->user) == 0) {
      db->ref_count++;
      o_log(O_LOG_DEBUG, "Database %s at %s with %s found (%d clients)\n", name, hostname, user,db->ref_count);
      return db;
    }
    db = db->next;
  }

  // need to create a new one
  Database *self = (Database*)malloc(sizeof(Database));
  memset(self, 0, sizeof(Database));
  logdebug("Creating new database %s at %s with %s\n", name, hostname, user);
  strncpy(self->name, name, MAX_DB_NAME_SIZE);
  strncpy(self->hostname, hostname, MAX_DB_NAME_SIZE);
  strncpy(self->user, user, MAX_DB_NAME_SIZE);
  self->ref_count = 1;

  /* if (sq3_create_database(self)) { */
  if (psql_create_database(self)) {
    free(self);
    return NULL;
  }

  char* start_time_str = self->get_metadata (self, "start_time");

  if (start_time_str == NULL)
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      self->start_time = tv.tv_sec;
      char s[64];
      snprintf (s, LENGTH(s), "%lu", self->start_time);
      self->set_metadata (self, "start_time", s);
      logdebug("Set DB start-time = %lu\n", self->start_time);
    }
  else
    {
      self->start_time = strtol (start_time_str, NULL, 0);
      free (start_time_str);
      logdebug("Retrieved DB start-time = %lu\n", self->start_time);
    }

  // hook this one into the list of active databases
  self->next = first_db;
  first_db = self;

  return self;
}
/**
 * \brief Client no longer uses this database. If this was the last client checking out, close database.
 * \param self the database to release
 */
void
database_release(Database* self)
{
  // FIXME:  This is currently being tripped by an upstream bug that needs to be investigated.
  if (self == NULL) {
    logerror("Tried to do database_release() on a NULL database.\n");
    return;
  }
  if (--self->ref_count > 0) return; // still in use

  // unlink DB
  Database* db_p = first_db;
  Database* prev_p = NULL;
  while (db_p != NULL && db_p != self) {
    prev_p = db_p;
    db_p = db_p->next;
  }
  if (db_p == NULL) {
    logerror("BUG: Releasing to unknown database\n");
    return;
  }
  if (prev_p == NULL) {
    first_db = self->next; // was first
  } else {
    prev_p->next = self->next;
  }

  loginfo ("Closing database '%s'\n", self->name);
  /* sq3_release(self); */
  psql_release(self);

  // no longer needed
  DbTable* t_p = self->first_table;
  while (t_p != NULL)
    {
      DbTable* t = t_p->next;
      database_table_free(t_p);
      t_p = t;
    }
  free(self);
}

/**
 * \brief Create a column in the database
 * \param self the database where we create the column
 * \param col_decl the name of the column
 * \param index the index of the column
 * \param check_only don't create col, just check it
 * \return 1 if successful 0 otherwise
 */
static int
parse_col_decl(DbTable* self, char* col_decl, int index, int check_only)
{
  char* name = col_decl;
  char* p = name;

  while (*p != ':' && *p != '\0') p++;

  if (*p == '\0')
    {
      logerror("Malformed column schema '%s'\n", name);
      return 0;
    }
  *(p++) = '\0';

  char* type_s = p;
  OmlValueT type = oml_type_from_s (type_s);
  // OML_LONG_VALUE is deprecated, and converted to INT32 internally in server.
  type = (type == OML_LONG_VALUE) ? OML_INT32_VALUE : type;
  if (type == OML_UNKNOWN_VALUE)
    {
      logerror("Unknown column type '%s'\n", type_s);
      return 0;
    }

  if (check_only)
    {
      if (index < self->col_size)
        {
          DbColumn* col = self->columns[index];
          if (col == NULL || strcmp(col->name, name) != 0 || col->type != type)
            {
              logwarn("Column '%s' of table '%s' different to previous declarations'\n",
                    name, self->name);
              return 0;
            }
        }
      else
        return 0; // This column is out of range for the previous schema... error!
    }
  else
    database_table_add_col (self, name, type, index);

  return 1;
}

/**
 * \brief get a table from the database
 * \param database the database to extract the table
 * \param schema name of the table
 * \return the table of or NULL if not successful
 */
DbTable*
database_get_table(Database* database, char* schema)
{
  if (database == NULL) return NULL;
  // table name
  char* p = schema;
  while (*p == ' ') p++; // skip white space
  char* tname = p;
  while (*p != ' ' && *p != '\0') p++;
  *(p++) = '\0';

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
    assert (check_only == 0);
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
    if (!parse_col_decl(table, col, index, check_only))
      {
        logwarn("A bad column specification was found in schema for table %s\n", table->name);
        if (!check_only)
          {
            logerror("This table will not be registered now\n");
            logerror("(but another client can register it with a valid schema later on)\n");
            database_table_free (table);
          }
        return NULL;
      }
    logdebug("Column name '%s'\n", col);
    index++;
  }
  if (!check_only) {
    /* if (sq3_create_table(database, table)) { */
    if (psql_create_table(database, table)) {
      free(table);
      return NULL;
    }
    table->next = database->first_table;
    database->first_table = table;
  }
  return table;
}

void
database_table_add_col (DbTable* table, const char* name, OmlValueT type, int index)
{
  DbColumn* col = (DbColumn*) malloc (sizeof (DbColumn));
  memset (col, 0, sizeof (DbColumn));
  strncpy (col->name, name, MAX_COL_NAME_SIZE);
  col->type = type;
  database_table_store_col (table, col, index);
}
/**
 * \brief store the column in the databse
 * \param table the table of the
 * \param col the column to store
 * \param index the index of the column
 */
void
database_table_store_col(
  DbTable*  table,
  DbColumn* col,
  int       index
) {
  assert (table != NULL && col != NULL);
  assert (index >= 0);
  if (index >= table->col_size) {
    DbColumn** old = table->columns;
    int old_count = table->col_size;

    table->col_size += DEF_COLUMN_COUNT;
    table->columns = (DbColumn**)malloc(table->col_size * sizeof(DbColumn*));
    int i;
    for (i = old_count - 1; i >= 0; i--) {
      table->columns[i] = old[i];
    }

    free (old);
  }
  table->columns[index] = col;
}
/**
 * \brief Free the table
 * \param table the table to free
 */
void
database_table_free(DbTable* table)
{
  if (table)
    {
      logdebug("Freeing table %s\n", table->name);
      if (table->columns != NULL)
        {
          int i;
          for (i = 0; i < table->col_size; i++)
            {
              if (table->columns[i] == NULL)
                break; // reached end
              free(table->columns[i]);
            }
          free(table->columns);
        }
      sq3_table_free (table);
      free(table);
    }
  else
    logwarn("Tried to free a NULL table\n");
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
