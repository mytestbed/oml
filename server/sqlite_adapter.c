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
#include <stdio.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <log.h>
#include <time.h>
#include <sys/time.h>
#include <mstring.h>
#include <htonll.h>
#include <mem.h>
#include <oml_value.h>
#include "database.h"
#include "schema.h"
#include "util.h"
#include "table_descr.h"
#include "sqlite_adapter.h"
#include "hook.h"

extern char *sqlite_database_dir;

typedef struct _sq3DB {
  sqlite3*  conn;
  int       sender_cnt;
  time_t    last_commit;
} Sq3DB;

typedef struct _sq3Table {
  sqlite3_stmt* insert_stmt;  // prepared insert statement
} Sq3Table;

static int
sql_stmt(Sq3DB* self, const char* stmt);

char*
sq3_get_key_value (Database* database, const char* table,
                   const char* key_column, const char* value_column,
                   const char* key);

int
sq3_set_key_value (Database* database, const char* table,
                   const char* key_column, const char* value_column,
                   const char* key, const char* value);

int
sq3_get_max_value (Database* database, const char* table, const char* column_name,
                   const char* where_column, const char* where_value);

char*
sq3_get_sender_id (Database* database, const char* name);

int
sq3_set_sender_id (Database* database, const char* name, int id);

int
sq3_get_max_sender_id (Database* database);

long
sq3_get_max_seq_no (Database* database, DbTable* table, int sender_id);

static int
begin_transaction (Sq3DB *db)
{
  const char *sql = "BEGIN TRANSACTION;";
  return sql_stmt (db, sql);
}

static int
end_transaction (Sq3DB *db)
{
  const char *sql = "END TRANSACTION";
  return sql_stmt (db, sql);
}

static int
reopen_transaction (Sq3DB *db)
{
  if (end_transaction (db) == -1) return -1;
  if (begin_transaction (db) == -1) return -1;
  return 0;
}

//static int first_row;

static struct {
  const char *name;
  const char *sql;
} meta_tables [] = {
  { .name = "_experiment_metadata",
    .sql = "CREATE TABLE _experiment_metadata (key TEXT PRIMARY KEY, value TEXT);" },
  { .name = "_senders",
    .sql = "CREATE TABLE _senders (name TEXT PRIMARY KEY, id INTEGER UNIQUE);" },
};

int
sq3_table_create_meta (Database *db, const char *name)
{
  Sq3DB *self = (Sq3DB*)db->handle;
  size_t i = 0;
  for (i = 0; i < LENGTH (meta_tables); i++)
    if (strcmp (meta_tables[i].name, name) == 0)
      return sql_stmt (self, meta_tables[i].sql);
  return -1;
}

/**
 * \brief Release the sqlite3 database
 * \param db the database that contains the sqlite3 database
 */
void
sq3_release(Database* db)
{
  MString *hook_command = mstring_create();
  char* fullpath[PATH_MAX+1];
  Sq3DB* self = (Sq3DB*)db->handle;
  end_transaction (self);
  sqlite3_close(self->conn);
  // TODO: Release table specific data
  
  if(hook_enabled()) {
    if (mstring_sprintf(hook_command, "%s file:%s/%s.sq3\n",
          HOOK_CMD_DBCLOSED, realpath(sqlite_database_dir, fullpath), db->name) == -1) {
      logwarn("Failed to construct command string for event hook\n");
      return -1;
    }
    if(hook_write(mstring_buf(hook_command), mstring_len(hook_command)) < mstring_len(hook_command))
      logwarn("Failed to send command string to event hook: %s\n", strerror(errno));
    mstring_delete(hook_command);
  }

  xfree(self);
  db->handle = NULL;
}

/**
 * @brief Add sender a new sender to the database, returning its
 * index.
 *
 * If a sender with the given id already exists, its pre-existing
 * index is returned.  Otherwise, a new sender is added to the table
 * with a new sender id, unique to this experiment.
 *
 * @param db the experiment database to which the sender id is being added.
 * @param sender_id the sender id
 * @return the index of the new sender
 */
static int
sq3_add_sender_id(Database* database, char* sender_id)
{
  Sq3DB* self = (Sq3DB*)database->handle;
  int index = -1;
  char* id_str = sq3_get_sender_id (database, sender_id);

  if (id_str)
    {
      index = atoi (id_str);
      xfree (id_str);
    }
  else
    {
      if (self->sender_cnt == 0)
        {
          int max_sender_id = sq3_get_max_sender_id (database);
          if (max_sender_id < 0)
            {
              logerror("Could not determing maximum sender id for database %s; starting at 0.\n",
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

char*
sq3_get_sender_id (Database* database, const char* name)
{
  return sq3_get_key_value (database, "_senders", "name", "id", name);
}

int
sq3_set_sender_id (Database* database, const char* name, int id)
{
  char s[64];
  snprintf (s, LENGTH(s), "%d", id);
  return sq3_set_key_value (database, "_senders", "name", "id", name, s);
}

int
sq3_get_max_sender_id (Database* database)
{
  return sq3_get_max_value (database, "_senders", "id", NULL, NULL);
}

long
sq3_get_max_seq_no (Database* database, DbTable* table, int sender_id)
{
  char s[64];
  snprintf (s, LENGTH(s), "%u", sender_id);
  // SELECT MAX(oml_seq) FROM table WHERE oml_sender_id='sender_id';
  return sq3_get_max_value (database, table->schema->name, "oml_seq", "oml_sender_id", s);
}

MString*
sq3_make_sql_insert (DbTable* table)
{
  int n = 0;
  int max = table->schema->nfields;

  if (max <= 0) {
    logerror ("Trying to insert 0 values into table %s\n", table->schema->name);
    goto fail_exit;
  }

  MString* mstr = mstring_create ();

  if (mstr == NULL) {
    logerror("Failed to create managed string for preparing SQL INSERT statement\n");
    goto fail_exit;
  }

  /* Build SQL "INSERT INTO" statement */
  n += mstring_set (mstr, "INSERT INTO \"");
  n += mstring_cat (mstr, table->schema->name);
  n += mstring_cat (mstr, "\" VALUES (?, ?, ?, ?"); /* metadata columns */
  while (max-- > 0)
    mstring_cat (mstr, ", ?");
  mstring_cat (mstr, ");");

  if (n != 0) goto fail_exit;
  return mstr;

 fail_exit:
  if (mstr) mstring_delete (mstr);
  return NULL;
}

/*
 * Create the adapter data structures required to represent a database
 * table, and if "backend_create" is true, actually issue the SQL
 * CREATE TABLE statement to the libsqlite3 library to create the
 * table in the backend.  If "backend_create" is false, the CREATE
 * TABLE statement is not executed, but an INSERT INTO prepared
 * statement is created, and other associated required data structures
 * are built.
 *
 * Returns 0 on success, -1 on failure.
 */
static int
table_create (Database* db, DbTable* table, int backend_create)
{
  if (table == NULL) {
    logwarn("Tried to create a table from a NULL definition.\n");
    return -1;
  }
  if (db == NULL) {
      logwarn("Tried to create a table in a NULL database.\n");
      return -1;
  }
  if (table->schema == NULL) {
    logwarn("No schema defined for table, cannot create\n");
    return -1;
  }
  MString *insert = NULL, *create = NULL;
  Sq3DB* sq3db = (Sq3DB*)db->handle;

  if (backend_create) {
    create = schema_to_sql (table->schema, oml_to_sql_type);
    if (!create) {
      logwarn("Failed to build SQL CREATE TABLE statement string for table %s.\n",
              table->schema->name);
      goto fail_exit;
    }
    if (sql_stmt(sq3db, mstring_buf(create))) {
      logerror("Could not create table: (%s).\n", sqlite3_errmsg(sq3db->conn));
      goto fail_exit;
    }
  }

  insert = sq3_make_sql_insert (table);
  if (!insert) {
    logwarn ("Failed to build SQL INSERT INTO statement string for table %s.\n",
             table->schema->name);
    goto fail_exit;
  }
  Sq3Table *sq3table = xmalloc(sizeof(Sq3Table));
  table->handle = sq3table;
  if (sqlite3_prepare_v2(sq3db->conn, mstring_buf(insert), -1,
                         &sq3table->insert_stmt, 0) != SQLITE_OK) {
    logerror("Could not prepare statement (%s).\n", sqlite3_errmsg(sq3db->conn));
    goto fail_exit;
  }

  if (create) mstring_delete (create);
  if (insert) mstring_delete (insert);
  return 0;

 fail_exit:
  if (create) mstring_delete (create);
  if (insert) mstring_delete (insert);
  return -1;
}

/*
 * Create the adapter structures required for the SQLite3 adapter to
 * represent the table.  If thin is false, then also issue an SQL
 * CREATE TABLE statement to the libsqlite3 library to actually create
 * the table in the backend; otherwise don't do that (it's a "shallow"
 * creation of the wrapper data structures, not "deep" into the
 * database itself).
 *
 * Return 0 on success, -1 on failure.
 */
int
sq3_table_create (Database *database, DbTable *table, int shallow)
{
  return table_create (database, table, !shallow);
}

int
sq3_table_free (Database *database, DbTable* table)
{
  (void)database;
  Sq3Table* sq3table = (Sq3Table*)table->handle;
  int ret = 0;
  if (sq3table) {
    ret = sqlite3_finalize (sq3table->insert_stmt);
    if (ret != SQLITE_OK)
      logwarn("Error encountered trying to finalize SQLITE3 prepared statement\n");
    xfree (sq3table);
  }
  return ret;
}

/**
 * \brief Insert value in the sqlite3 database
 * \param db the database that contains the sqlite3 db
 * \param table the table to insert data in
 * \param sender_id the sender ID
 * \param seq_no the sequence number
 * \param time_stamp the timestamp of the receiving data
 * \param values the values to insert
 * \param value_count the number of values
 * \return 0 if successful, -1 otherwise
 */
static int
sq3_insert(Database *db, DbTable *table, int sender_id, int seq_no,
           double time_stamp, OmlValue *values, int value_count)
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
    if (reopen_transaction (sq3db) == -1)
      return -1;
    sq3db->last_commit = tv.tv_sec;
  }

  //  o_log(O_LOG_DEBUG2, "sq3_insert(%s): insert row %d \n",
  //        table->schema->name, seq_no);

  if (sqlite3_bind_int(stmt, 1, sender_id) != SQLITE_OK) {
    logerror("Could not bind 'oml_sender_id' (%s).\n",
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_int(stmt, 2, seq_no) != SQLITE_OK) {
    logerror("Could not bind 'oml_seq' (%s).\n",
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_double(stmt, 3, time_stamp) != SQLITE_OK) {
    logerror("Could not bind 'oml_ts_client' (%s).\n",
        sqlite3_errmsg(sq3db->conn));
  }
  if (sqlite3_bind_double(stmt, 4, time_stamp_server) != SQLITE_OK) {
    logerror("Could not bind 'oml_ts_server' (%s).\n",
        sqlite3_errmsg(sq3db->conn));
  }

  OmlValue* v = values;
  struct schema *schema = table->schema;
  if (schema->nfields != value_count) {
    logerror ("Trying to insert %d values into a table with %d columns\n",
              value_count, schema->nfields);
    sqlite3_reset (stmt);
    return -1;
  }
  for (i = 0; i < schema->nfields; i++, v++) {
    if (v->type != schema->fields[i].type) {
      if (v->type == OML_BLOB_VALUE && schema->fields[i].type == OML_UINT64_VALUE)
        schema->fields[i].type = OML_BLOB_VALUE; // Dodgy because we map UINT64 -> BLOB for SQLite
      else {
        char *expected = oml_type_to_s (schema->fields[i].type);
        char *received = oml_type_to_s (v->type);
        logerror("Mismatch in value type for column %d of table '%s'\n", i, table->schema->name);
        logerror("-> Column name='%s', type=%s, but trying to insert a %s\n",
                 schema->fields[i].name, expected, received);
        sqlite3_reset (stmt);
        return -1;
      }
    }
    int res;
    int idx = i + 5;
    switch (schema->fields[i].type) {
    case OML_DOUBLE_VALUE: res = sqlite3_bind_double(stmt, idx, v->value.doubleValue); break;
    case OML_LONG_VALUE:   res = sqlite3_bind_int(stmt, idx, (int)v->value.longValue); break;
    case OML_INT32_VALUE:  res = sqlite3_bind_int(stmt, idx, (int)v->value.int32Value); break;
    case OML_UINT32_VALUE: res = sqlite3_bind_int(stmt, idx, (int)v->value.uint32Value); break;
    case OML_INT64_VALUE:  res = sqlite3_bind_int64(stmt, idx, (int)v->value.int64Value); break;
      /* Unsigned 64-bit integer will lose precision in sqlite3, so use a BLOB instead */
    case OML_UINT64_VALUE:
      {
        uint64_t blob = htonll (v->value.uint64Value);
        res = sqlite3_bind_blob(stmt, idx, &blob, sizeof(uint64_t), SQLITE_TRANSIENT);
        break;
      }
    case OML_STRING_VALUE:
      {
        res = sqlite3_bind_text (stmt, idx, v->value.stringValue.ptr,
                                 -1, SQLITE_TRANSIENT);
        break;
      }
    case OML_BLOB_VALUE: {
      res = sqlite3_bind_blob (stmt, idx,
                               v->value.blobValue.data,
                               v->value.blobValue.fill,
                               SQLITE_TRANSIENT);
      break;
    }
    default:
      logerror("Unknown type %d in col '%s'\n", schema->fields[i].type, schema->fields[i].name);
      sqlite3_reset (stmt);
      return -1;
    }
    if (res != SQLITE_OK) {
      logerror("Could not bind column '%s' (%s).\n",
               schema->fields[i].name, sqlite3_errmsg(sq3db->conn));
      sqlite3_reset (stmt);
      return -1;
    }
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    logerror("Could not step (execute) SQL statement in sq3_insert().  (%s)\n",
             sqlite3_errmsg(sq3db->conn));
    sqlite3_reset(stmt);
    return -1;
  }
  return sqlite3_reset(stmt);
}

#if 0 // select_callback is not used anywhere in this translation unit, but it's static!
static int
select_callback(void* p_data, int num_fields, char** p_fields, char** p_col_names)
{
  int i;
  int* nof_records = (int*) p_data;
  (*nof_records)++;

  if (first_row) {
    first_row = 0;
    for (i=0; i < num_fields; i++)
      printf("%20s", p_col_names[i]);
    printf("\n");
    for (i=0; i< num_fields*20; i++)
      printf("=");
    printf("\n");
  }

  for(i=0; i < num_fields; i++) {
    if (p_fields[i])
      printf("%20s", p_fields[i]);
    else
      printf("%20s", " ");
  }
  printf("\n");
  return 0;
}
#endif

#if 0 // select_stmt is not used anywhere in this translation unit, but it's static!
static int
select_stmt(Sq3DB* self, const char* stmt)
{
  char* errmsg;
  int   ret;
  int   nrecs = 0;
  logdebug("prepare to exec 1 \n");
  first_row = 1;

  ret = sqlite3_exec(self->conn, stmt, select_callback, &nrecs, &errmsg);

  if (ret != SQLITE_OK) {
    logerror("Error in select statement %s [%s].\n", stmt, errmsg);
    return -1;
  }
  return nrecs;
}
#endif

/**
 * \brief Execute sqlite3 statement
 * \param self the sqlite3 database
 * \param stmt the statement to prepare
 * \return 0 if successfull, -1 otherwise
 */
static int
sql_stmt(Sq3DB* self, const char* stmt)
{
  char *errmsg;
  int   ret;
  logdebug("prepare to exec %s \n", stmt);
  ret = sqlite3_exec(self->conn, stmt, 0, 0, &errmsg);

  if (ret != SQLITE_OK) {
    logwarn("Error(%d) in statement: %s [%s].\n", ret, stmt, errmsg);
    return -1;
  }
  return 0;
}

TableDescr*
sq3_get_table_list (Database *database, int *num_tables)
{
  Sq3DB *self = database->handle;
  const char* stmt = "SELECT name,sql FROM sqlite_master WHERE type='table' ORDER BY name;";
  char* errmsg;
  char** result;
  int nrows;
  int ncols;
  int ret = sqlite3_get_table (self->conn, stmt, &result, &nrows, &ncols, &errmsg);

  *num_tables = -1;
  if (ret != SQLITE_OK) {
    logerror("Error in SELECT statement %s [%s].\n", stmt, errmsg);
    sqlite3_free (errmsg);
    return NULL;
  }

  if (ncols == 0 || nrows == 0) {
    logdebug("Database table list seems empty; need to create tables.\n");
    sqlite3_free_table (result);
    *num_tables = 0;
    return NULL;
  }

  int i = 0;
  int name_col = -1;
  int schema_col = -1;
  for (i = 0; i < ncols; i++) {
    if (strcasecmp(result[i], "name") == 0)
      name_col = i;
    if (strcasecmp (result[i], "sql") == 0)
      schema_col = i;
  }

  if (name_col == -1 || schema_col == -1) {
    logerror("Couldn't get the 'name' or 'schema' column index from the SQLITE database list of tables\n");
    sqlite3_free_table (result);
    *num_tables = 0;
    return NULL;
  }

  TableDescr* tables = NULL;

  int n = 0;
  int j = 0;
  for (i = name_col + ncols, j = schema_col + ncols;
       i < (nrows + 1) * ncols;
       i += ncols, j+= ncols, n++) {
    TableDescr *t;
    // Don't try to treat the metadata tables as measurement tables.
    if (strcmp (result[i], "_experiment_metadata") == 0 ||
        strcmp (result[i], "_senders") == 0) {
      t = table_descr_new (result[i], NULL);
    } else {
      struct schema *schema = schema_from_sql (result[j]);
      if (!schema) {
        logwarn ("Failed to create table '%s': error parsing schema (not created by OML?):\n%s\n",
                 result[i], result[j]);
        continue;
      }
      t = table_descr_new (result[i], schema);
    }
    t->next = tables;
    tables = t;
  }

  sqlite3_free_table (result);
  *num_tables = n;
  return tables;
}

char*
sq3_get_metadata (Database* database, const char* key)
{
  return sq3_get_key_value (database, "_experiment_metadata", "key", "value", key);
}

int
sq3_set_metadata (Database* database, const char* key, const char* value)
{
  return sq3_set_key_value (database, "_experiment_metadata", "key", "value", key, value);
}

/**
 *  @brief Do a key-value style select on a database table.
 *
 *  This function does a key lookup on a database table that is set up
 *  in key-value style.  The table can have more than two columns, but
 *  this function SELECT's two of them and returns the value of the
 *  value column.  It checks to make sure that the key returned is the
 *  one requested, then returns its corresponding value.
 *
 *  This function makes a lot of assumptions about the database and
 *  the table:
 *
 *  #- the database exists and is open
 *  #- the table exists in the database
 *  #- there is a column named key_column in the table
 *  #- there is a column named value_column in the table
 *
 *  The function does not check for any of these conditions, but just
 *  assumes they are true.  Be advised.
 *
 *  @param database the database to look up in.
 *  @param table the name of the table to look up in.
 *  @param key_column the name of the column holding the key strings.
 *  @param value_column the name of the column holding the value strings.
 *  @param key the key string to look up (i.e. WHERE key_column='key').
 *
 *  @return the string value corresponding to the given key, or NULL
 *  if an error occurred of if the key was not present in the table.
 */
char*
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
    logerror("Error in SELECT statement '%s' [%s].\n", mstring_buf (stmt), errmsg);
    sqlite3_free (errmsg);
    return NULL;
  }

  if (ncols == 0 || nrows == 0) {
    sqlite3_free_table (result);
    return NULL;
  }

  if (nrows > 1)
    logwarn("Key-value lookup for key '%s' in %s(%s, %s) returned more than one possible key.\n",
            key, table, key_column, value_column);

  char* value = NULL;

  if (strcmp (key, result[2]) == 0) {
    size_t len = strlen (result[3]) + 1;
    value = xmalloc (len);
    strncpy (value, result[3], len);
  }

  sqlite3_free_table (result);
  return value;
}

int
sq3_set_key_value (Database* database, const char* table,
                   const char* key_column, const char* value_column,
                   const char* key, const char* value)
{
  Sq3DB* sq3db = (Sq3DB*) database->handle;
  char stmt[512];
  size_t n;
  char* check_value = sq3_get_key_value (database, table, key_column, value_column, key);
  if (check_value == NULL)
    n = snprintf (stmt, LENGTH(stmt), "INSERT INTO \"%s\" (\"%s\", \"%s\") VALUES ('%s', '%s');",
                  table,
                  key_column, value_column,
                  key, value);
  else
    n = snprintf (stmt, LENGTH(stmt), "UPDATE \"%s\" SET \"%s\"='%s' WHERE \"%s\"='%s';",
                  table,
                  value_column, value,
                  key_column, key);

  if (check_value != NULL)
    xfree (check_value);

  if (n >= LENGTH (stmt)) {
    logwarn("SQL statement too long trying to update key-value pair %s='%s' in %s(%s, %s)\n",
            key, value, table, key_column, value_column);
    return -1;
  }

  if (sql_stmt (sq3db, stmt)) {
    logwarn("Key-value update failed for %s='%s' in %s(%s, %s) (database error)\n",
            key, value, table, key_column, value_column);
    return -1;
  }

  return 0;
}

int
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
      logwarn("SQL statement too long trying to find MAX of column %s in table %s\n",
              column_name, table);
      return -1;
    }

  int ret = sqlite3_get_table (sq3db->conn, stmt, &result, &nrows, &ncols, &errmsg);

  if (ret != SQLITE_OK)
    {
      logerror("Error in SELECT statement %s [%s].\n", stmt, errmsg);
      sqlite3_free (errmsg);
      return -1;
    }

  if (ncols == 0 || nrows == 0)
    {
      loginfo("Max-value lookup on table %s:  result set seems to be empty.\n", table);
      sqlite3_free_table (result);
      return 0;
    }

  if (nrows > 1)
    logwarn("Max-value lookup for column '%s' in table %s returned more than one possible value.\n",
            column_name, table);

  if (ncols > 1)
    logwarn("Max-value lookup for column '%s' in table %s returned more than one possible column.\n",
            column_name, table);

  int max = 0;
  if (result[1] == NULL)
    max = 0;
  else
    max = atoi (result[1]);

  sqlite3_free_table (result);
  return max;
}

/**
 * \brief Create a sqlite3 database
 * \param db the databse to associate with the sqlite3 database
 * \return 0 if successful, -1 otherwise
 */
int
sq3_create_database(Database* db)
{
  sqlite3* conn;
  int rc;
  MString *path = mstring_create ();
  if (mstring_sprintf (path, "%s/%s.sq3", sqlite_database_dir, db->name) == -1) {
    logerror ("Failed to construct database path string\n");
    return -1;
  }
  loginfo ("Opening SQLite database for experiment '%s', path is '%s'\n",
           db->name, mstring_buf (path));
  rc = sqlite3_open(mstring_buf (path), &conn);
  mstring_delete (path);
  if (rc) {
    logerror("Can't open database for experiment '%s': %s\n", db->name, sqlite3_errmsg(conn));
    sqlite3_close(conn);
    return -1;
  }

  Sq3DB* self = xmalloc(sizeof(Sq3DB));
  self->conn = conn;
  self->last_commit = time (NULL);
  db->table_create = sq3_table_create;
  db->table_create_meta = sq3_table_create_meta;
  db->table_free = sq3_table_free;
  db->release = sq3_release;
  db->insert = sq3_insert;
  db->add_sender_id = sq3_add_sender_id;
  db->set_metadata = sq3_set_metadata;
  db->get_metadata = sq3_get_metadata;
  db->get_table_list = sq3_get_table_list;

  db->handle = self;

  begin_transaction (self);
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
