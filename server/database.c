/*
 * Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file database.c
 * Generic interface for database backends
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>

#include "ocomm/o_log.h"
#include "oml_util.h"
#include "oml_value.h"
#include "mem.h"
#include "mstring.h"
#include "database.h"
#include "hook.h"
#include "sqlite_adapter.h"

#if HAVE_LIBPQ
#include <libpq-fe.h>
#include "psql_adapter.h"
#endif

#define DEF_COLUMN_COUNT 1
#define DEF_TABLE_COUNT 1

static struct db_backend
{
  const char * const name;
  db_adapter_create fn;
} backends [] =
  {
    { "sqlite", sq3_create_database },
#if HAVE_LIBPQ
    { "postgresql", psql_create_database },
#endif
  };

char* dbbackend = DEFAULT_DB_BACKEND;

static Database *first_db = NULL;

/** Get the list of valid database backends.
 *
 * \return a comma-separated list of the available backends
 */
static const char *
database_valid_backends ()
{
  static char s[256] = {0};
  int i;
  if (s[0] == '\0') {
    char *p = s;
    for (i = LENGTH (backends) - 1; i >= 0; i--) {
      int n = sprintf(p, "%s", backends[i].name);
      if (i) {
        p[n++] = ',';
        p[n++] = ' ';
      }
      p[n] = '\0';
      p += n;
    }
  }
  return s;
}

/** Setup the selected database backend.
 *
 * Must be called after dropping privileges.
 *
 * \param backend name of the selected backend
 * \return 0 on success, -1 otherwise
 *
 * \see sq3_backend_setup, psql_backend_setup
 */
int
database_setup_backend (const char* backend)
{
  logdebug ("Database backend: '%s'\n", backend);

  if (!database_create_function (backend)) {
    logerror ("Unknown database backend '%s' (valid backends: %s)\n",
         backend, database_valid_backends ());
    return -1;
  }

  if (!strcmp (backend, "sqlite")) {
    if(sq3_backend_setup ()) return -1;
#if HAVE_LIBPQ
  } else if (!strcmp (backend, "postgresql")) {
    if(psql_backend_setup ()) return -1;
#endif
  }
  return 0;
}

/** Get the database-creation function for the selected backend.
 *
 * \param backend name of the selected backed
 * \return the backend's db_adapter_create function
 */
db_adapter_create
database_create_function (const char *backend)
{
  size_t i = 0;
  for (i = 0; i < LENGTH (backends); i++)
    if (!strncmp (backend, backends[i].name, strlen (backends[i].name)))
      return backends[i].fn;

  return NULL;
}

/** Find a database instance for name.
 *
 * If no database with this name exists, a new one is created.
 *
 * \param name name of the database to find
 * \return a pointer to the database
 */
Database*
database_find (const char* name)
{
  Database* db = first_db;
  while (db != NULL) {
    if (!strcmp(name, db->name)) {
      loginfo ("%s: Database already open (%d client%s)\n",
                name, db->ref_count, db->ref_count>1?"s":"");
      db->ref_count++;
      return db;
    }
    db = db->next;
  }

  // need to create a new one
  Database *self = xmalloc(sizeof(Database));
  logdebug("%s: Creating or opening database\n", name);
  strncpy(self->name, name, MAX_DB_NAME_SIZE);
  self->ref_count = 1;
  self->create = database_create_function (dbbackend);

  if (self->create (self)) {
    xfree(self);
    return NULL;
  }

  if (database_init (self) == -1) {
    xfree (self);
    return NULL;
  }

  char *start_time_str = self->get_metadata (self, "start_time");

  if (start_time_str != NULL)
    {
      self->start_time = strtol (start_time_str, NULL, 0);
      xfree (start_time_str);
      logdebug("%s: Retrieved start-time = %lu\n", name, self->start_time);
    }

  // hook this one into the list of active databases
  self->next = first_db;
  first_db = self;

  return self;
}
/** One client no longer uses this database.
 * If this was the last client checking out, close database.
 * \param self the database to release
 * \see db_adapter_release
 */
void
database_release(Database* self)
{
  MString *hook_command = mstring_create();
  char dburi[PATH_MAX+1];

  if (self == NULL) {
    logerror("NONE: Trying to release a NULL database.\n");
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
    logerror("%s:  Trying to release an unknown database\n", self->name);
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

  loginfo ("%s: Closing database\n", self->name);
  self->release (self);

  if(hook_enabled()) {
    if(!self->get_uri(self, dburi, sizeof(dburi)))
      logwarn("%s: Unable to get full URI to database for hook\n", self->name);
    else if (mstring_sprintf(hook_command, "%s %s\n",
          HOOK_CMD_DBCLOSED, dburi) == -1) {
      logwarn("%s: Failed to construct command string for event hook\n", self->name);
    }
    if(hook_write(mstring_buf(hook_command), mstring_len(hook_command)) < (int)mstring_len(hook_command))
      logwarn("%s: Failed to send command string to event hook: %s\n", self->name, strerror(errno));
    mstring_delete(hook_command);
  }

  xfree(self);
}

/** Close all open databases
 *
 * Useful when exiting.
 */
void
database_cleanup()
{
  Database *next, *db = first_db;

  logdebug("Cleaning up databases\n");

  while (db) {
    next = db->next;
    database_release(db);
    db = next;
  }
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

/** Create the adapter structure for a table.
 *
 * Create a new table in the database, with given schema.  Register
 * the table with the database, so that database_find_table () will
 * find it. Return a pointer to the table, or NULL on error.
 *
 * The schema is deep copied, so the caller can safely free the
 * schema.
 *
 * Note: this function does NOT issue the SQL required to create the
 * table in the actual storage backend.
 *
 * \param database Database to add the DbTable to
 * \param schema schema structure for that tables
 * \return an xmalloc'd DbTable (already added to database), or NULL on error
 *
 * \see database_find_table, schema_copy, database_release
 */
DbTable*
database_create_table (Database *database, const struct schema *schema)
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

/** Search a Database's registered DbTables for one matchng the given schema.
 *
 * If none is found, the table is created. If one is found, but the schema
 * differs, try to append a number to the name (up to MAX_TABLE_RENAME), create
 * that table, and update the schema.
 *
 * If the table search/creation is successful, the returned DbTable is already
 * added to the list of the Database.
 *
 * \param database Database to search
 * \param schema schema structure for the table to add
 * \return a newly created DbTable for that table, or NULL on error
 *
 * \see MAX_TABLE_RENAME, database_create_table
 */
DbTable*
database_find_or_create_table(Database *database, struct schema *schema)
{
  if (database == NULL) return NULL;
  if (schema == NULL) return NULL;

  DbTable *table = NULL;
  struct schema *s = schema_copy(schema);
  int i = 1;
  int diff = 0, tnlen;

  tnlen = strlen(schema->name);

  do {
    table = database_find_table (database, s->name);

    if (table) {
      diff = schema_diff (s, table->schema);
      if (!diff) {
        schema_free(s);
        return table;

      } else if (diff == -1) {
        logerror ("%s: Schema error table '%s'\n", database->name, s->name);
        logdebug (" One of the server schema %p or the client schema %p is probably NULL\n", s, table->schema);

      } else if (diff > 0) {
        struct schema_field *client = &s->fields[diff-1];
        struct schema_field *stored = &table->schema->fields[diff-1];
        logdebug ("%s: Schema differ for table index '%s', at column %d: expected %s:%s, got %s:%s\n",
            database->name, s->name, diff,
            stored->name, oml_type_to_s (stored->type),
            client->name, oml_type_to_s (client->type));
      }

      if (i == 1) {
        /* First time we need to increase the size */
        /* Add space for 2 characters and null byte, that is up to '_9', with MAX_TABLE_RENAME = 10 */
        s->name = xrealloc(s->name, tnlen + 3);
        strncpy(s->name, schema->name, tnlen);
      }
      snprintf(&s->name[tnlen], 3, "_%d", ++i);
    }

  } while(table && diff && (i < MAX_TABLE_RENAME));

  if (table && diff) {
    logerror ("%s: Too many (>%d) tables named '%s_x', giving up. Please use the rename attribute of <mp /> tags.\n",
      database->name, MAX_TABLE_RENAME, schema->name);
    schema_free(s);
    return NULL;
  }

  if(i>1) {
    /* We had to change the table name*/
    logwarn("%s: Creating table '%s' for new stream '%s' with incompatible schema\n", database->name, s->name, schema->name);
    xfree(schema->name);
    schema->name = xstrndup(s->name, tnlen+3);
  }
  schema_free(s);

  /* No table by that name exists, so we create it */
  table = database_create_table (database, schema);
  if (!table)
    return NULL;
  if (database->table_create (database, table, 0)) {
    logerror ("%s: Couldn't create table '%s'\n", database->name, schema->name);
    /* Unlink the table from the experiment's list */
    DbTable* t = database->first_table;
    if (t == table)
      database->first_table = t->next;
    else {
      while (t && t->next != table)
        t = t->next;
      if (t && t->next)
        t->next = t->next->next;
    }
    database_table_free (database, table);
    return NULL;
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
    logdebug("%s: Freeing table '%s'\n", database->name, table->schema->name);
    schema_free (table->schema);
    xfree(table);
  } else {
    logwarn("%s: Tried to free a NULL table (or database was NULL).\n",
            (database ? database->name : "NONE"));
  }
}

/** Prepare an INSERT statement for a given table
 *
 * The returned value is to be destroyed by the caller.
 *
 * \param db Database
 * \param table DbTable adapter for the target SQLite3 table
 * \return an MString containing the prepared statement, or NULL on error
 *
 * \see mstring_create, mstring_delete
 */
MString*
database_make_sql_insert (Database *db, DbTable* table)
{
  MString* mstr = mstring_create ();
  int n = 0, i = 0;
  int max = table->schema->nfields;
  char *pvar;

  if (max <= 0) {
    logerror ("%d: Trying to insert 0 values into table %s\n",
        db->backend_name, table->schema->name);
    goto fail_exit;
  }

  if (mstr == NULL) {
    logerror("%d: Failed to create managed string for preparing SQL INSERT statement\n",
        db->backend_name);
    goto fail_exit;
  }

  /* Build SQL "INSERT INTO" statement */
  n += mstring_sprintf (mstr,
      "INSERT INTO \"%s\" (\"oml_sender_id\", \"oml_seq\", \"oml_ts_client\", \"oml_ts_server\"",
      table->schema->name);

  /* Add specific column names */
  for (i=0; i<max; i++) {
    n += mstring_sprintf (mstr, ", \"%s\"", table->schema->fields[i].name);
  }

  /* Close column names, and add variables for the prepared statement */
  pvar = db->prepared_var(db, 1);
  n += mstring_sprintf (mstr, ") VALUES (%s", pvar);
  xfree(pvar); /* XXX: Not really efficient, but we only do this rarely */

  max += 4; /* Number of metadata columns we want to be able to insert */
  for (i=2; i<=max; i++) {
    pvar = db->prepared_var(db, i);
    n += mstring_sprintf (mstr, ", %s", pvar);
    xfree(pvar);
  }

  /* We're done */
  n += mstring_cat (mstr, ");");

  logdebug("%s:%s: Prepared insert statement for tables %s: %s\n",
      db->backend_name, db->name, table->schema->name, mstring_buf(mstr));

  if (n != 0) {
    /* mstring_* return -1 on error, with no error, the sum in n should be 0 */
    goto fail_exit;
  }
  return mstr;

 fail_exit:
  if (mstr) mstring_delete (mstr);
  return NULL;
}

/** Initialise adapters for a new database
 *
 * If the database already has tables, initialise the adapters.
 *
 * \param database main Database object
 * \return 0 on success, -1 otherwise
 *
 * \see database_create_table, db_adapter_table_create
 */
int
database_init (Database *database)
{
  int num_tables;
  TableDescr* tables = database->get_table_list (database, &num_tables);
  TableDescr* td = tables;

  if (num_tables == -1)
    return -1;

  logdebug("%s: Got table list with %d tables in it\n", database->name, num_tables);
  int i = 0;
  for (i = 0; i < num_tables; i++, td = td->next) {
    if (td->schema) {
      struct schema *schema = schema_copy (td->schema);
      DbTable *table = database_create_table (database, schema);

      if (!table) {
        logwarn ("%s: Failed to create table '%s'\n",
                 database->name, td->name);
        continue;
      }
      /* Create the required table data structures, but don't do SQL CREATE TABLE */
      if (database->table_create (database, table, 1) == -1) {
        logwarn ("%s: Failed to create adapter structures for table '%s'\n",
                 database->name, td->name);
        database_table_free (database, table);
      }
    }
  }

  /* Create default tables if they are not already present in the 'tables' list */
  const char *meta_tables [] = { "_senders", "_experiment_metadata" };
  size_t j;
  for (j = 0; j < LENGTH(meta_tables); j++) {
    if (!table_descr_have_table (tables, meta_tables[j])) {
      if (database->table_create_meta (database, meta_tables[j])) {
        table_descr_list_free (tables);
        logerror("%s: Could not create default table %s\n",
            database->name, meta_tables[j]);
        return -1;
      }
    }
  }

  table_descr_list_free (tables);
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
