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

#include "database.h"
#include "database_adapter.h"

/** Metadata tables */
static struct {
  const char *name;
  const char *sql;
} meta_tables[] = {
  { .name = "_experiment_metadata",
    .sql = "CREATE TABLE _experiment_metadata (key TEXT PRIMARY KEY, value TEXT);",
  },
  { .name = "_senders",
    .sql = "CREATE TABLE _senders (name TEXT PRIMARY KEY, id INTEGER UNIQUE);",
  },
};

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

/** Create the metadata tables using this backend's SQL wrapper
 * \see db_adapter_table_create_meta
 */
int
dba_table_create_meta (Database *db, const char *name)
{
  int i = 0;
  for (i = 0; i < LENGTH (meta_tables); i++) {
    if (strcmp (meta_tables[i].name, name) == 0) {
      return db->stmt (db, meta_tables[i].sql);
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
