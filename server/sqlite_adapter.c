/*
 * Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file sqlite_adapter.c
 * \brief Adapter code for the SQLite3 database backend.
 */
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <sqlite3.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mstring.h"
#include "htonll.h"
#include "guid.h"
#include "oml_value.h"
#include "oml_util.h"
#include "schema.h"
#include "database.h"
#include "table_descr.h"
#include "database_adapter.h"
#include "sqlite_adapter.h"

static char backend_name[] = "sqlite";
/* Cannot be static due to testsuite */
char *sqlite_database_dir = NULL;

/** Mapping between OML and SQLite3 data types
 * \see sq3_type_to_oml, sq3_oml_to_type
 */
static struct {
  OmlValueT type;           /** OML type */
  const char * const name;  /** SQLite3 equivalent */
} sq3_type_pair [] = {
  { OML_DB_PRIMARY_KEY, "INTEGER PRIMARY KEY"},
  { OML_DB_PRIMARY_KEY, "INTEGER PRIMARY KEY"},
  { OML_INT32_VALUE,    "INTEGER"  },
  { OML_UINT32_VALUE,   "UNSIGNED INTEGER" },
  { OML_INT64_VALUE,    "BIGINT"  },
  { OML_UINT64_VALUE,   "UNSIGNED BIGINT" },
  { OML_DOUBLE_VALUE,   "REAL" },
  { OML_STRING_VALUE,   "TEXT" },
  { OML_BLOB_VALUE,     "BLOB" },
  { OML_GUID_VALUE,     "UNSIGNED BIGINT" },
};

static int sql_stmt(Sq3DB* self, const char* stmt);

/* Functions needed by the Database struct */
static OmlValueT sq3_type_to_oml (const char *s);
static const char *sq3_oml_to_type (OmlValueT T);
static int sq3_stmt(Database* db, const char* stmt);
static void sq3_release(Database* db);
static int sq3_table_create (Database* db, DbTable* table, int shallow);
static int sq3_table_free (Database *database, DbTable* table);
static char *sq3_prepared_var(Database *db, unsigned int order);
static int sq3_insert(Database *db, DbTable *table, int sender_id, int seq_no, double time_stamp, OmlValue *values, int value_count);
static char* sq3_get_key_value (Database* database, const char* table, const char* key_column, const char* value_column, const char* key);
static int sq3_set_key_value (Database* database, const char* table, const char* key_column, const char* value_column, const char* key, const char* value);
static char* sq3_get_metadata (Database* database, const char* key);
static int sq3_set_metadata (Database* database, const char* key, const char* value);
static int sq3_add_sender_id(Database* database, const char* sender_id);
static char* sq3_get_uri(Database *db, char *uri, size_t size);
static TableDescr* sq3_get_table_list (Database *database, int *num_tables);

static char* sq3_get_sender_id (Database* database, const char* name);
static int sq3_set_sender_id (Database* database, const char* name, int id);
static int sq3_get_max_value (Database* database, const char* table, const char* column_name, const char* where_column, const char* where_value);
static int sq3_get_max_sender_id (Database* database);

/** Work out which directory to put sqlite databases in, and set
 * sqlite_database_dir to that directory.
 *
 * This works as follows: if the user specified --data-dir on the
 * command line, we use that value.  Otherwise, if OML_SQLITE_DIR
 * environment variable is set, use that dir.  Otherwise, use
 * PKG_LOCAL_STATE_DIR, which is a preprocessor macro set by the build
 * system (under Autotools defaults this should be
 * ${prefix}/var/oml2-server, but on a distro it should be something
 * like /var/lib/oml2-server).
 */
void
sq3_dbdir_setup (void)
{
  if (!sqlite_database_dir) {
    const char *oml_sqlite_dir = getenv ("OML_SQLITE_DIR");
    if (oml_sqlite_dir) {
      sqlite_database_dir = xstrndup (oml_sqlite_dir, strlen (oml_sqlite_dir));
    } else {
      sqlite_database_dir = PKG_LOCAL_STATE_DIR;
    }
  }
}

/** Setup the SQLite3 backend.
 *
 * \return 0 on success, -1 otherwise
 *
 * \see database_setup_backend
 */
int
sq3_backend_setup (void)
{
  sq3_dbdir_setup ();

  /*
   * The man page says access(2) should be avoided because it creates
   * a race condition between calls to access(2) and open(2) where an
   * attacker could swap the underlying file for a link to a file that
   * the unprivileged user does not have permissions to access, which
   * the effective user does.  We don't use the access/open sequence
   * here. We check that we can create files in the
   * sqlite_database_dir directory, and fail if not (as the server
   * won't be able to do useful work otherwise).
   *
   * An attacker could potentially change the underlying file to a
   * link and still cause problems if oml2-server is run as root or
   * setuid root, but this check must happen after drop_privileges()
   * is called, so if oml2-server is not run with superuser privileges
   * such an attack would still fail.
   *
   * oml2-server should not be run as root or setuid root in any case.
   */
  if (access (sqlite_database_dir, R_OK | W_OK | X_OK) == -1) {
    logerror ("sqlite: Can't access SQLite database directory %s: %s\n",
         sqlite_database_dir, strerror (errno));
    return -1;
  }

  loginfo ("sqlite: Creating SQLite3 databases in %s\n", sqlite_database_dir);

  return 0;
}

/** Mapping from SQLite3 to OML types.
 * \see db_adapter_type_to_oml
 */
static
OmlValueT sq3_type_to_oml (const char *type)
{
  int i = 0;
  int n = LENGTH(sq3_type_pair);

  for (i = 0; i < n; i++) {
    if (strcmp (type, sq3_type_pair[i].name) == 0) {
        return sq3_type_pair[i].type;
    }
  }
  logwarn("Unknown SQLite3 type '%s', using OML_UNKNOWN_VALUE\n", type);
  return OML_UNKNOWN_VALUE;
}

/** Mapping from OML types to SQLite3 types.
 * \see db_adapter_oml_to_type
 */
static const char*
sq3_oml_to_type (OmlValueT type)
{
  int i = 0;
  int n = LENGTH(sq3_type_pair);

  for (i = 0; i < n; i++) {
    if (sq3_type_pair[i].type == type) {
        return sq3_type_pair[i].name;
    }
  }
  logerror("Unknown OML type %d\n", type);
  return NULL;
}

/** Execute an SQL statement (using sqlite3_exec()).
 * FIXME: Clearly not prepared statements (#168)
 * FIXME: Wrap into sq3_stmt and adapt code (s/db->conn/db/)
 * \see db_adapter_stmt, sqlite3_exec
 */
static int
sql_stmt(Sq3DB* self, const char* stmt)
{
  char *errmsg;
  int   ret;
  logdebug("sqlite: Will execute '%s'\n", stmt);
  ret = sqlite3_exec(self->conn, stmt, 0, 0, &errmsg);

  if (ret != SQLITE_OK) {
    logerror("sqlite: Error executing '%s': %s\n", stmt, errmsg);
    return -1;
  }
  return 0;
}

/** Type-agnostic wrapper for sql_stmt */
static int
sq3_stmt(Database* db, const char* stmt)
{
 return sql_stmt((Sq3DB*)db->handle, stmt);
}

/** Create an SQLite3 database and adapter structures
 * \see db_adapter_create
 */
/* This function is exposed to the rest of the code for backend initialisation */
int
sq3_create_database(Database* db)
{
  sqlite3* conn;
  int rc;
  MString *path = mstring_create ();
  if (mstring_sprintf (path, "%s/%s.sq3", sqlite_database_dir, db->name) == -1) {
    logerror ("sqlite:%s: Failed to construct database path string\n", db->name);
    return -1;
  }
  loginfo ("sqlite:%s: Opening database at '%s'\n",
           db->name, mstring_buf (path));
  rc = sqlite3_open(mstring_buf (path), &conn);
  mstring_delete (path);
  if (rc) {
    logerror("sqlite:%s: Can't open database: %s\n", db->name, sqlite3_errmsg(conn));
    sqlite3_close(conn);
    return -1;
  }

  Sq3DB* self = xmalloc(sizeof(Sq3DB));
  self->conn = conn;
  self->last_commit = time (NULL);
  db->backend_name = backend_name;
  db->o2t = sq3_oml_to_type;
  db->t2o = sq3_type_to_oml;
  db->stmt = sq3_stmt;
  db->table_create = sq3_table_create;
  db->table_create_meta = dba_table_create_meta;
  db->table_free = sq3_table_free;
  db->release = sq3_release;
  db->prepared_var = sq3_prepared_var;
  db->insert = sq3_insert;
  db->add_sender_id = sq3_add_sender_id;
  db->set_metadata = sq3_set_metadata;
  db->get_metadata = sq3_get_metadata;
  db->get_uri = sq3_get_uri;
  db->get_table_list = sq3_get_table_list;

  db->handle = self;

  dba_begin_transaction (db);
  return 0;
}

/** Release the SQLite3 database.
 * \see db_adapter_release
 */
static void
sq3_release(Database* db)
{
  Sq3DB* self = (Sq3DB*)db->handle;
  dba_end_transaction (db);
  sqlite3_close(self->conn);
  xfree(self);
  db->handle = NULL;
}

/** Create the adapter structures required for the SQLite3 adapter
 * \see db_adapter_table_create
 */
static int
sq3_table_create (Database* db, DbTable* table, int shallow)
{
  MString *insert = NULL;
  Sq3DB* sq3db = NULL;
  Sq3Table *sq3table = NULL;
  if (db == NULL) {
      logwarn("sqlite: Tried to create a table in a NULL database\n");
      return -1;
  }
  if (table == NULL) {
    logwarn("sqlite:%s: Tried to create a table from a NULL definition\n", db->name);
    return -1;
  }
  if (table->schema == NULL) {
    logwarn("sqlite:%s: No schema defined for table, cannot create\n", db->name);
    return -1;
  }
  sq3db = (Sq3DB*)db->handle;

  if (!shallow) {
    if (dba_table_create_from_schema(db, table->schema)) {
      logerror("sqlite:%s: Could not create table '%s': %s\n",
          db->name, table->schema->name,
          sqlite3_errmsg(sq3db->conn));
      goto fail_exit;
    }
  }

  /* Related to #1056. */
  if (table->handle != NULL) {
    logwarn("sqlite:%s: BUG: Recreating Sq3Table handle for table %s\n",
        table->schema->name);
  }
  sq3table = (Sq3Table*)xmalloc(sizeof(Sq3Table));
  table->handle = sq3table;

  /* XXX: Should not be done here, see #1056 */
  insert = database_make_sql_insert (db, table);
  if (!insert) {
    logerror ("sqlite:%s: Failed to build SQL INSERT INTO statement string for table '%s'\n",
             db->name, table->schema->name);
    goto fail_exit;
  }

  if (sqlite3_prepare_v2(sq3db->conn, mstring_buf(insert), -1,
                         &sq3table->insert_stmt, 0) != SQLITE_OK) {
    logerror("sqlite:%s: Could not prepare statement '%s': %s\n",
        db->name, mstring_buf(insert), sqlite3_errmsg(sq3db->conn));
    goto fail_exit;
  }

  if (insert) { mstring_delete (insert); }
  return 0;

 fail_exit:
  if (insert) { mstring_delete (insert); }
  if (sq3table) { xfree (sq3table); }
  return -1;
}

/** Free an SQLite3 table
 *
 * Parameter database is ignored in this implementation
 *
 * \see db_adapter_table_free, sqlite3_finalize
 */
static int
sq3_table_free (Database *database, DbTable* table)
{
  (void)database;
  Sq3Table* sq3table = (Sq3Table*)table->handle;
  int ret = 0;
  if (sq3table) {
    ret = sqlite3_finalize (sq3table->insert_stmt);
    if (ret != SQLITE_OK) {
      logwarn("sqlite:%s: Couldn't finalise statement for table '%s' (database error)\n",
          database->name, table->schema->name);
    }
    xfree (sq3table);
  }
  return ret;
}

/** Return a string suitable for an unbound variable is SQLite3.
 *
 * This is always "?"
 *
 * \see db_adapter_prepared_var
 */
static char*
sq3_prepared_var(Database *db, unsigned int order)
{
  char *s = xmalloc(2);

  (void)db;

  if (NULL != s) {
    *s = '?';
    *(s+1) = 0;
  }

  return s;
}

/** Insert value in the SQLite3 database.
 * \see db_adapter_insert
 */
static int
sq3_insert(Database *db, DbTable *table, int sender_id, int seq_no, double time_stamp, OmlValue *values, int value_count)
{
  Sq3DB* sq3db = (Sq3DB*)db->handle;
  Sq3Table* sq3table = (Sq3Table*)table->handle;
  int i;
  double time_stamp_server;
  sqlite3_stmt* stmt = sq3table->insert_stmt;
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_stamp_server = tv.tv_sec - db->start_time + 0.000001 * tv.tv_usec;

  if (tv.tv_sec > sq3db->last_commit) {
    if (dba_reopen_transaction (db) == -1) {
      return -1;
    }
    sq3db->last_commit = tv.tv_sec;
  }

  //  o_log(O_LOG_DEBUG2, "sq3_insert(%s): insert row %d \n",
  //        table->schema->name, seq_no);

  if (sqlite3_bind_int(stmt, 1, sender_id) != SQLITE_OK) {
    logerror("sqlite:%s: Could not bind 'oml_sender_id' in table '%s': %s\n",
        db->name, table->schema->name,
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_int(stmt, 2, seq_no) != SQLITE_OK) {
    logerror("sqlite:%s: Could not bind 'oml_seq' in table '%s': %s\n",
        db->name, table->schema->name,
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_double(stmt, 3, time_stamp) != SQLITE_OK) {
    logerror("sqlite:%s: Could not bind 'oml_ts_client' in table '%s': %s\n",
        db->name, table->schema->name,
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_double(stmt, 4, time_stamp_server) != SQLITE_OK) {
    logerror("sqlite:%s: Could not bind 'oml_ts_server' in table '%s': %s\n",
        db->name, table->schema->name,
        sqlite3_errmsg(sq3db->conn));
  }

  OmlValue* v = values;
  struct schema *schema = table->schema;
  if (schema->nfields != value_count) {
    logerror ("sqlite:%s: Trying to insert %d values into table '%s' with %d columns\n",
        db->name, value_count, table->schema->name, schema->nfields);
    sqlite3_reset (stmt);
    return -1;
  }
  for (i = 0; i < schema->nfields; i++, v++) {
    if (oml_value_get_type(v) != schema->fields[i].type) {
      const char *expected = oml_type_to_s (schema->fields[i].type);
      const char *received = oml_type_to_s (oml_value_get_type(v));
      logerror("sqlite:%s: Value %d type mismatch for table '%s'\n", db->name, i, table->schema->name);
      logdebug("sqlite:%s: -> Column name='%s', type=%s, but trying to insert a %s\n",
          db->name, schema->fields[i].name, expected, received);
      sqlite3_reset (stmt);
      return -1;
    }
    int res;
    int idx = i + 5;
    switch (schema->fields[i].type) {
    case OML_DOUBLE_VALUE: res = sqlite3_bind_double(stmt, idx, v->value.doubleValue); break;
    case OML_LONG_VALUE:   res = sqlite3_bind_int(stmt, idx, (int)v->value.longValue); break;
    case OML_INT32_VALUE:  res = sqlite3_bind_int(stmt, idx, (int32_t)v->value.int32Value); break;
    case OML_UINT32_VALUE: res = sqlite3_bind_int(stmt, idx, (uint32_t)v->value.uint32Value); break;
    case OML_INT64_VALUE:  res = sqlite3_bind_int64(stmt, idx, (int64_t)v->value.int64Value); break;
    case OML_UINT64_VALUE:
      {
        if (v->value.uint64Value > (uint64_t)9223372036854775808u)
          logwarn("sqlite:%s: Trying to store value %" PRIu64 " (>2^63) in column '%s' of table '%s', this might lead to a loss of resolution\n", db->name, (uint64_t)v->value.uint64Value, schema->fields[i].name, table->schema->name);
        res = sqlite3_bind_int64(stmt, idx, (uint64_t)v->value.uint64Value);
        break;
      }
    case OML_STRING_VALUE:
      {
        res = sqlite3_bind_text (stmt, idx, omlc_get_string_ptr(*oml_value_get_value(v)),
                                 -1, SQLITE_TRANSIENT);
        break;
      }
    case OML_BLOB_VALUE: {
      res = sqlite3_bind_blob (stmt, idx,
                               v->value.blobValue.ptr,
                               v->value.blobValue.length,
                               SQLITE_TRANSIENT);
      break;
    }
    case OML_GUID_VALUE: {
      if(v->value.uint64Value != UINT64_C(0)) {
        res = sqlite3_bind_int64(stmt, idx, (int64_t)(v->value.guidValue));
      } else {
        res = sqlite3_bind_null(stmt, idx);
      }
      break;
    }
    default:
      logerror("sqlite:%s: Unknown type %d in col '%s' of table '%s; this is probably a bug'\n",
          db->name, schema->fields[i].type, schema->fields[i].name, table->schema->name);
      sqlite3_reset (stmt);
      return -1;
    }
    if (res != SQLITE_OK) {
      logerror("sqlite:%s: Could not bind column '%s': %s\n",
          db->name, schema->fields[i].name, sqlite3_errmsg(sq3db->conn));
      sqlite3_reset (stmt);
      return -1;
    }
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    logerror("sqlite:%s: Could not step SQL statement: %s\n",
        db->name, sqlite3_errmsg(sq3db->conn));
    sqlite3_reset(stmt);
    return -1;
  }
  return sqlite3_reset(stmt);
}

/** Do a key-value style select on a database table.
 *
 * FIXME: Not using prepared statements (#168)
 *
 * The caller is responsible for xfree()ing the returned value when no longer
 * needed.
 *
 * This function does a key lookup on a database table that is set up
 * in key-value style.  The table can have more than two columns, but
 * this function SELECT's two of them and returns the value of the
 * value column.  It checks to make sure that the key returned is the
 * one requested, then returns its corresponding value.
 *
 * This function makes a lot of assumptions about the database and
 * the table:
 *
 * #- the database exists and is open
 * #- the table exists in the database
 * #- there is a column named key_column in the table
 * #- there is a column named value_column in the table
 *
 * The function does not check for any of these conditions, but just
 * assumes they are true.  Be advised.
 *
 * \param database Database to use
 * \param table name of the table to work in
 * \param key_column name of the column to use as key
 * \param value_column name of the column to set the value in
 * \param key key to look for in key_column
 *
 * \return an xmalloc'd string value for the given key, or NULL
 *
 * \see sq3_set_key_value, xmalloc, xfree
 */
static char*
sq3_get_key_value (Database* database, const char* table, const char* key_column,
                   const char* value_column, const char* key)
{
  if (database == NULL || table == NULL || key_column == NULL ||
      value_column == NULL || key == NULL)
    return NULL;

  Sq3DB* sq3db = (Sq3DB*) database->handle;
  MString *stmt = mstring_create ();
  mstring_sprintf (stmt, "SELECT %s,%s FROM %s WHERE %s='%s';",
                   key_column, value_column, table, key_column, key);
  char* errmsg;
  char** result;
  int nrows;
  int ncols;

  int ret = sqlite3_get_table (sq3db->conn, mstring_buf(stmt), &result,
                               &nrows, &ncols, &errmsg);

  if (ret != SQLITE_OK) {
    logerror("sqlite:%s: Error in SELECT statement '%s': %s\n", database->name, mstring_buf (stmt), errmsg);
    sqlite3_free (errmsg);
    return NULL;
  }

  if (ncols == 0 || nrows == 0) {
    sqlite3_free_table (result);
    return NULL;
  }

  if (nrows > 1)
    logwarn("sqlite:%s: Key-value lookup for key '%s' in %s(%s, %s) returned more than one possible key.\n",
             database->name, key, table, key_column, value_column);

  char* value = NULL;

  if (strcmp (key, result[2]) == 0) {
    size_t len = strlen (result[3]);
    value = xstrndup (result[3], len);
  }

  sqlite3_free_table (result);
  return value;
}

/** Set a value for the given key in the given table
 *
 * FIXME: Not using prepared statements (#168)
 *
 * \param database Database to use
 * \param table name of the table to work in
 * \param key_column name of the column to use as key
 * \param value_column name of the column to set the value in
 * \param key key to look for in key_column
 * \param value value to look for in value_column
 * \return 0 on success, -1 otherwise
 *
 * \see sq3_get_key_value
 */
static int
sq3_set_key_value (Database* database, const char* table,
                   const char* key_column, const char* value_column,
                   const char* key, const char* value)
{
  Sq3DB* sq3db = (Sq3DB*) database->handle;
  char stmt[512];
  size_t n;
  char* check_value = sq3_get_key_value (database, table, key_column, value_column, key);
  if (check_value == NULL) {
    n = snprintf (stmt, LENGTH(stmt), "INSERT INTO \"%s\" (\"%s\", \"%s\") VALUES ('%s', '%s');",
                  table,
                  key_column, value_column,
                  key, value);
  } else {
    n = snprintf (stmt, LENGTH(stmt), "UPDATE \"%s\" SET \"%s\"='%s' WHERE \"%s\"='%s';",
                  table,
                  value_column, value,
                  key_column, key);

  }
  if (check_value != NULL) {
    xfree (check_value);
  }

  if (n >= LENGTH (stmt)) {
    logwarn("sqlite:%s: SQL statement too long trying to update key-value pair %s='%s' in %s(%s, %s)\n",
        database->name, key, value, table, key_column, value_column);
    return -1;
  }

  if (sql_stmt (sq3db, stmt)) {
    logwarn("sqlite:%s: Key-value update failed for %s='%s' in %s(%s, %s) (database error)\n",
            database->name, key, value, table, key_column, value_column);
    return -1;
  }

  return 0;
}

/** Get data from the metadata table
 * \see db_adapter_get_metadata, sq3_get_key_value
 */
static char*
sq3_get_metadata (Database* database, const char* key)
{
  return sq3_get_key_value (database, "_experiment_metadata", "key", "value", key);
}

/** Set data in the metadata table
 * \see db_adapter_set_metadata, sq3_set_key_value
 */
static int
sq3_set_metadata (Database* database, const char* key, const char* value)
{
  return sq3_set_key_value (database, "_experiment_metadata", "key", "value", key, value);
}

/** Add a new sender to the database, returning its index.
 * \see db_add_sender_id
 */
static int
sq3_add_sender_id(Database* database, const char* sender_id)
{
  Sq3DB* self = (Sq3DB*)database->handle;
  int index = -1;
  char* id_str = sq3_get_sender_id (database, sender_id);

  if (id_str) {
    index = atoi (id_str);
    xfree (id_str);

  } else {
    if (self->sender_cnt == 0) {
      int max_sender_id = sq3_get_max_sender_id (database);
      if (max_sender_id < 0) {
        logwarn("sqlite:%s: Could not determine maximum sender id; starting at 0\n",
            database->name);
        max_sender_id = 0;
      }
      self->sender_cnt = max_sender_id;
    }
    index = ++self->sender_cnt;

    sq3_set_sender_id (database, sender_id, index);

  }

  return index;
}

/** Build a URI for this database.
 *
 * URI is of the form file:PATH/DATABASE.sq3
 *
 * \see db_adapter_get_uri
 */
static char*
sq3_get_uri(Database *db, char *uri, size_t size)
{
  char fullpath[PATH_MAX+1];
  if(snprintf(uri, size, "file:%s/%s.sq3\n", realpath(sqlite_database_dir, fullpath), db->name) >= size) {
    return NULL;
  }
  return uri;
}

/** Get a list of tables in an SQLite3 database
 * \see db_adapter_get_table_list
 */
static TableDescr*
sq3_get_table_list (Database *database, int *num_tables)
{
  Sq3DB *self = database->handle;
  TableDescr *tables = NULL, *t = NULL;
  const char *table_stmt = "SELECT name FROM sqlite_master WHERE type='table' ORDER BY name;";
  const char *tablename, *meta;
  const char *schema_stmt = "SELECT value FROM _experiment_metadata WHERE key='table_%s';";
  MString *schema_stmt_ms = NULL;
  sqlite3_stmt *ptable_stmt = NULL, *pschema_stmt = NULL;
  struct schema *schema;
  int res, have_meta = 0;

  /* Get a list of table names */
  if (sqlite3_prepare_v2(self->conn, table_stmt, -1, &ptable_stmt, NULL) != SQLITE_OK) {
    logerror("sqlite:%s: Could not prepare statement '%s': %s\n",
        database->name, table_stmt, sqlite3_errmsg(self->conn));
    goto fail_exit;
  }

  /* Check if the _experiment_metadata table exists */
  do {
    res = sqlite3_step(ptable_stmt);
    if (res == SQLITE_ROW) {
      if(strcmp ((const char*)sqlite3_column_text(ptable_stmt, 0), "_experiment_metadata") == 0) {
        logdebug("sqlite:%s: Found table %s\n",
            database->name, sqlite3_column_text(ptable_stmt, 0));
        have_meta = 1;
      }
    }
  } while (res == SQLITE_ROW && !have_meta);

  *num_tables = 0; /* In case !have_meta, we want it to be 0; also, we need it that way for later */

  if(!have_meta) {
    logdebug("sqlite:%s: _experiment_metadata table not found\n", database->name);
    /* XXX: This is probably a new database, don't exit in error */
    sqlite3_finalize(ptable_stmt);
    return NULL;
  }

  /* Get schema for all tables */
  if(sqlite3_reset(ptable_stmt) != SQLITE_OK) {
    logwarn("sqlite:%s: Could not reset statement '%s' to traverse results again: %s\n",
        database->name, table_stmt, sqlite3_errmsg(self->conn));
    /* The _experiment_metadata table is often (always?) at the beginning of the list,
     * so we try to recover gracefully from this issue by ignoring it */
  }

  schema_stmt_ms = mstring_create();
  do {
    t = NULL;
    mstring_set(schema_stmt_ms, "");

    res = sqlite3_step(ptable_stmt);
    if (res == SQLITE_ROW) {
      tablename = (const char*)sqlite3_column_text(ptable_stmt, 0);

      if(!strcmp (tablename, "_senders")) {
        /* Create a phony entry for the _senders table some
         * server/database.c:database_init() doesn't try to create it */
        t = table_descr_new (tablename, NULL);

      } else {
        mstring_sprintf(schema_stmt_ms, schema_stmt, tablename); /* XXX: Could a prepared statement do this concatenation? */
        /* If it's *not* the _senders table, get its schema from the metadata table */
        logdebug("sqlite:%s:%s: Trying to find schema for table %s: %s\n",
            database->name, __FUNCTION__, tablename,
            mstring_buf(schema_stmt_ms));

        if (sqlite3_prepare_v2(self->conn, mstring_buf(schema_stmt_ms),
              -1, &pschema_stmt, NULL) != SQLITE_OK) {
          logerror("sqlite:%s: Could not prepare statement '%s': %s\n",
              database->name, mstring_buf(schema_stmt_ms), sqlite3_errmsg(self->conn));
          goto fail_exit;
        }

        if(sqlite3_step(pschema_stmt) != SQLITE_ROW) {
          logerror("sqlite:%s: Could not get schema for table %s: %s\n",
              database->name, tablename, sqlite3_errmsg(self->conn));
          goto fail_exit;
        }

        meta = (const char*)sqlite3_column_text(pschema_stmt, 0); /* We should only have one result row */
        schema = schema_from_meta(meta);
        sqlite3_finalize(pschema_stmt);
        pschema_stmt = NULL; /* Avoid double-finalisation in case of error */

        if (!schema) {
          logerror("sqlite:%s: Could not parse schema '%s' (stored in DB) for table %s\n",
              database->name, meta, tablename);
          goto fail_exit;
        }

        t = table_descr_new (tablename, schema);
        if (!t) {
          logerror("sqlite:%s: Could create table descrition for table %s\n",
              database->name, tablename);
          goto fail_exit;
        }
        schema = NULL; /* The pointer has been copied in t (see table_descr_new);
                          we don't want to free it twice in case of error */
      }

      t->next = tables;
      tables = t;
      (*num_tables)++;
    }
  } while (res == SQLITE_ROW);

  mstring_delete(schema_stmt_ms);
  sqlite3_finalize(pschema_stmt);

  return tables;

fail_exit:
  if (tables) {
    table_descr_list_free(tables);
  }

  if (schema) {
    schema_free(schema);
  }
  if (pschema_stmt) {
   sqlite3_finalize(pschema_stmt);
  }
  if (schema_stmt_ms) {
    mstring_delete(schema_stmt_ms);
  }

  if (ptable_stmt) {
   sqlite3_finalize(ptable_stmt);
  }

  *num_tables = -1;
  return NULL;
}

/** Get the sender_id for a given name in the _senders table.
 *
 * \param name name of the sender
 * \return the sender ID
 *
 * \see sq3_get_key_value
 */
static char*
sq3_get_sender_id (Database* database, const char* name)
{
  return sq3_get_key_value (database, "_senders", "name", "id", name);
}

/** Set the sender_id for a given name in the _senders table.
 *
 * \param name name of the sender
 * \param id the ID to set
 * \return the sender ID
 *
 * \see sq3_set_key_value
 */
static int
sq3_set_sender_id (Database* database, const char* name, int id)
{
  char s[64];
  snprintf (s, LENGTH(s), "%d", id);
  return sq3_set_key_value (database, "_senders", "name", "id", name, s);
}

/** Find the largest value in a given column of a table, matching some conditions
 *
 * FIXME: Not using prepared statements (#168)
 *
 * \param database Database to use
 * \param table name of the table to work in
 * \param column_name name of the column to lookup
 * \param where_column column to look for specific condition
 * \param where_value condition on where_column
 * \return the maximal value found in column_name when where_column==where_value is matched, or -1 on error
 */
static int
sq3_get_max_value (Database* database, const char* table, const char* column_name,
                   const char* where_column, const char* where_value)
{
  if (database == NULL)
    return -1;

  Sq3DB* sq3db = (Sq3DB*) database->handle;
  char* errmsg;
  char** result;
  int nrows;
  int ncols;
  char stmt[512];
  size_t n;

  if (where_column && where_value)
    n = snprintf (stmt, LENGTH(stmt), "SELECT MAX(%s) FROM %s WHERE %s='%s'",
                  column_name, table, where_column, where_value);
  else
    n = snprintf (stmt, LENGTH(stmt), "SELECT MAX(%s) FROM %s",
                  column_name, table);


  if (n >= LENGTH (stmt))
    {
      logwarn("sqlite:%s: SQL statement too long trying to find MAX of column '%s' in table '%s'\n",
          database->name, column_name, table);
      return -1;
    }

  int ret = sqlite3_get_table (sq3db->conn, stmt, &result, &nrows, &ncols, &errmsg);

  if (ret != SQLITE_OK)
    {
      logerror("sqlite:%s: Error in SELECT statement '%s': %s\n", database->name, stmt, errmsg);
      sqlite3_free (errmsg);
      return -1;
    }

  if (ncols == 0 || nrows == 0)
    {
      logdebug("sqlite:%s: Max-value lookup for column '%s' in table '%s' result set empty\n",
          database->name, column_name, table);
      sqlite3_free_table (result);
      return 0;
    }

  if (nrows > 1)
    logwarn("sqlite:%s: Max-value lookup for column '%s' in table '%s' returned more than one possible value\n",
        database->name, column_name, table);

  if (ncols > 1)
    logwarn("sqlite:%s: Max-value lookup for column '%s' in table '%s' returned more than one possible column\n",
        database->name, column_name, table);

  int max = 0;
  if (result[1] == NULL)
    max = 0;
  else
    max = atoi (result[1]);

  sqlite3_free_table (result);
  return max;
}

/** Get the highest sender ID in this database.
 * \param db Database to use
 * \return the highest sender ID, or -1 on error
 * \see sq3_get_max_value
 */
static int
sq3_get_max_sender_id (Database* database)
{
  return sq3_get_max_value (database, "_senders", "id", NULL, NULL);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
