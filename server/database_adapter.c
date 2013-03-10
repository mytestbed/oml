/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file database_adapter.h
 * \brief Generic functions for database adapters.
 */
#include <string.h>

#include "schema.h"
#include "database.h"
#include "database_adapter.h"

/** Metadata tables */
static struct {
  const char *name;
  const char *sql;
  const char *schema;
} meta_tables[] = {
  { .name = "_experiment_metadata",
    .sql = "CREATE TABLE _experiment_metadata (key TEXT PRIMARY KEY, value TEXT);",
    .schema = NULL,
  },
  { .name = "_senders",
    .sql = "CREATE TABLE _senders (name TEXT PRIMARY KEY, id INTEGER UNIQUE);",
    .schema = NULL,
  },
};

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

/** Create a table in the specified database following the given schema string
 *
 * \param db Database to create table in
 * \param meta schema string (from metadata) to generate table from
 * \return 0 on success, -1 otherwise
 */
int dba_table_create_from_meta (Database *db, const char *meta)
{
  int ret;
  struct schema* s = NULL;

  s = schema_from_meta(meta);
  if (!s) {
    return -1;
  }

  ret = dba_table_create_from_schema(db, s);

  schema_free(s);

  return ret;
}

/** Create a table in the specified database following the given schema
 *
 * \param db Database to create table in
 * \param meta schema to generate table from
 * \return 0 on success, -1 otherwise
 */
int
dba_table_create_from_schema (Database *db, const struct schema *schema)
{
  DbTable *table;
  MString *create = NULL, *meta_skey = NULL;;
  char *meta_svalue = NULL;
  struct schema *schema_meta;
  create = schema_to_sql (schema, db->o2t);
  if (!create) {
    logerror("%s:%s: Failed to build SQL CREATE TABLE statement string for schema '%s'\n",
        db->backend_name, db->name, schema_to_meta(schema));
    goto fail_exit;
  }
  if (db->stmt(db, mstring_buf(create))) {
    goto fail_exit;
  }
  /* FIXME: Create prepared insertion statement here. See #1056. */
  if(!(table = database_create_table(db, schema))) {
    logerror("%s:%s: Failed to register generic adapter for newly created table for schema '%s'\n",
        db->backend_name, db->name, schema_to_meta(schema));
    goto fail_exit;

  } else if (db->table_create(db, table, 1)) { /* XXX: table_create calls us when shallow=0 */
    logerror("%s:%s: Failed to register specific adapter for newly created table for schema '%s'\n",
        db->backend_name, db->name, schema_to_meta(schema));
    goto fail_exit;
  }

  /* The schema index is irrelevant in the metadata, temporarily drop it */
  schema_meta = schema_copy(schema);
  schema_meta->index = -1;
  meta_skey = mstring_create ();
  mstring_sprintf (meta_skey, "table_%s", schema->name);
  meta_svalue = schema_to_meta (schema);
  db->set_metadata (db, mstring_buf (meta_skey), meta_svalue);
  xfree(meta_svalue);
  mstring_delete(meta_skey);
  schema_free(schema_meta);

  if (create) { mstring_delete(create); }
  return 0;

fail_exit:
  if (create) { mstring_delete(create); }
  return -1;
}

/** Create the metadata tables using this backend's SQL wrapper
 * \see db_adapter_table_create_meta
 */
int
dba_table_create_meta (Database *db, const char *name)
{
  int i = 0;
  for (i = 0; i < LENGTH (meta_tables); i++) {
    if (strcmp (meta_tables[i].name, name) == 0) {
      if (meta_tables[i].sql) {
        logdebug("%s:%s: Creating default table %s from SQL '%s'\n",
            db->backend_name, db->name,
            name, meta_tables[i].sql);
        return db->stmt (db, meta_tables[i].sql);
      } else if (meta_tables[i].schema) {
        logdebug("%s:%s: Creating default table %s from schema '%s'\n",
            db->backend_name, db->name,
            name, meta_tables[i].schema);
        return dba_table_create_from_meta(db, meta_tables[i].schema);
      } else {
        logwarn ("%s:%s: Default definition found for %s, but it is not complete\n", name);
      }
    }
  }
  return -1;
}

/** Open a transaction with the database server.
 * \param db Database to work with
 * \return the success value of running the statement
 * \see db_adapter_stmt
 */
int
dba_begin_transaction (Database *db)
{
  const char sql[] = "BEGIN TRANSACTION;";
  return db->stmt (db, sql);
}

/** Close the current transaction with the database server.
 * \param db Database to work with
 * \return the success value of running the statement
 * \see db_adapter_stmt
 */
int
dba_end_transaction (Database *db)
{
  const char sql[] = "END TRANSACTION;";
  return db->stmt (db, sql);
}

/** Close the current transaction and start a new one.
 * \param db Database to work with
 * \return 0 on success, -1 otherwise
 * \see dba_begin_transaction, dba_end_transaction, db_adapter_stmt
 */
int
dba_reopen_transaction (Database *db)
{
  if (dba_end_transaction (db)) { return -1; }
  if (dba_begin_transaction (db)) { return -1; }
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
