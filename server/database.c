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
#include "mem.h"
#include "database.h"
#include "util.h"
#include "oml_value.h"

#define DEF_COLUMN_COUNT 1
#define DEF_TABLE_COUNT 1

static Database *first_db = NULL;

db_adapter_create
database_create_function ();

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
    if (!strcmp(name, db->name) &&
        !strcmp(hostname, db->hostname) &&
        !strcmp(user, db->user)) {
      loginfo ("Experiment '%s': Database '%s' already open at %s with user '%s' (%d clients)\n",
                name, name, hostname, user, db->ref_count);
      db->ref_count++;
      return db;
    }
    db = db->next;
  }

  // need to create a new one
  Database *self = xmalloc(sizeof(Database));
  loginfo("Experiment '%s': Opening database '%s' at %s with user '%s'\n",
          name, name, hostname, user);
  strncpy(self->name, name, MAX_DB_NAME_SIZE);
  strncpy(self->hostname, hostname, MAX_DB_NAME_SIZE);
  strncpy(self->user, user, MAX_DB_NAME_SIZE);
  self->ref_count = 1;
  self->create = database_create_function ();

  if (self->create (self)) {
    xfree(self);
    return NULL;
  }

  char *start_time_str = self->get_metadata (self, "start_time");
  char *method;

  if (start_time_str == NULL)
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      self->start_time = tv.tv_sec;
      char s[64];
      snprintf (s, LENGTH(s), "%lu", self->start_time);
      self->set_metadata (self, "start_time", s);
      method = "Set";
    }
  else
    {
      self->start_time = strtol (start_time_str, NULL, 0);
      xfree (start_time_str);
      method = "Retrieved";
    }
  loginfo("Experiment '%s': %s DB start-time = %lu\n", name, method, self->start_time);

  // hook this one into the list of active databases
  self->next = first_db;
  first_db = self;

  return self;
}
/**
 * \brief Client no longer uses this database.
 * If this was the last client checking out, close database.
 * \param self the database to release
 */
void
database_release(Database* self)
{
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
    logerror("Experiment '%'s:  trying to release an unknown database\n", self->name);
    return;
  }
  if (prev_p == NULL)
    first_db = self->next; // was first
  else
    prev_p->next = self->next;

  // no longer needed
  DbTable* t_p = self->first_table;
  while (t_p != NULL) {
    DbTable* t = t_p->next;
    /* Release the backend storage for this table */
    self->table_free (self, t_p);
    /* Release the table */
    database_table_free(self, t_p);
    t_p = t;
  }

  loginfo ("Experiment '%s': closing database\n", self->name);
  self->release (self);

  xfree(self);
}

/*
 * Find the table with matching "name".  Return NULL if not found.
 */
DbTable*
database_find_table (Database *database, const char *name)
{
  DbTable *table = database->first_table;
  while (table) {
    if (!strcmp (table->schema->name, name))
      return table;
    table = table->next;
  }
  return NULL;
}

/*
 * Create a new table in the database, with given schema.  Register
 * the table with the database, so that database_find_table () will
 * find it.  Return a pointer to the table, or NULL on error.
 *
 * The schema is deep copied, so the caller can safely free the
 * schema.
 *
 * Note: this function does NOT issue the SQL required to create the
 * table in the actual storage backend.
 */
DbTable*
database_create_table (Database *database, struct schema *schema)
{
  DbTable *table = xmalloc (sizeof (DbTable));
  if (!table)
    return NULL;
  table->schema = schema_copy (schema);
  if (!table->schema) {
    xfree (table);
    return NULL;
  }
  table->next = database->first_table;
  database->first_table = table;
  return table;
}

DbTable*
database_find_or_create_table(Database *database, struct schema *schema)
{
  if (database == NULL) return NULL;
  DbTable *table = database_find_table (database, schema->name);
  int found = table ? 1 : 0;

  if (!found) {
    table = database_create_table (database, schema);
    if (!table)
      return NULL;
    if (database->table_create (database, table)) {
      database_table_free (database, table);
      return NULL;
    }
  } else {
    /* If the table already exists, validate the client's schema against existing */
    int same = schema_diff (schema, table->schema);
    if (same == -1) {
      logerror ("Schemas differ for table '%s'\n", schema->name);
      return NULL;
    } else if (same > 0) {
      struct schema_field *client = &schema->fields[same-1];
      struct schema_field *stored = &table->schema->fields[same-1];
      logerror ("Schema differ at column %d:\n", same);
      logerror ("Client declared         '%s:%s'\n",
                client->name, oml_type_to_s (client->type));
      logerror ("Existing table declared '%s:%s'\n",
                stored->name, oml_type_to_s (stored->type));
      return NULL;
    }
  }
  return table;
}

/*
 * Destroy a table in a database, by free all allocated data
 * structures.  Does not release the table in the backend adapter.
 */
void
database_table_free(Database *database, DbTable *table)
{
  if (database && table) {
    logdebug("Experiment '%s':  freeing table %s\n", database->name, table->schema->name);
    schema_free (table->schema);
    xfree(table);
  } else {
    logwarn("Experiment '%s':  tried to free a NULL table (or database was NULL).\n",
            (database ? database->name : "NONE"));
  }
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
