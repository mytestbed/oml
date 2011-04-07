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
#include <stdio.h>
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

typedef struct _sq3DB {
  sqlite3*  db_hdl;
  int       sender_cnt;
  time_t    last_commit;
} Sq3DB;

typedef struct _sq3Table {
  sqlite3_stmt* insert_stmt;  // prepared insert statement
} Sq3Table;

static int
sql_stmt(Sq3DB* self, const char* stmt);

TableDescr*
sq3_get_table_list (Sq3DB* self, int* num_tables);

char*
sq3_get_metadata (Database* self, const char* key);
int
sq3_set_metadata (Database* self, const char* key, const char* value);


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

const char* SQL_CREATE_EXPT_METADATA = "CREATE TABLE _experiment_metadata (key TEXT PRIMARY KEY, value TEXT);";
const char* SQL_CREATE_SENDERS       = "CREATE TABLE _senders (name TEXT PRIMARY KEY, id INTEGER UNIQUE);";

/**
 * \brief Release the sqlite3 database
 * \param db the database that contains the sqlite3 database
 */
void
sq3_release(Database* db)
{
  Sq3DB* self = (Sq3DB*)db->adapter_hdl;
  end_transaction (self);
  sqlite3_close(self->db_hdl);
  // TODO: Release table specific data

  xfree(self);
  db->adapter_hdl = NULL;
}

/**
 * \brief  Add sender ID to the table
 * \param db the database that contains the sqlite3 db
 * \param sender_id the sender ID
 * \return the index of the sender
 */
static int
sq3_add_sender_id(Database* database, char* sender_id)
{
  Sq3DB* self = (Sq3DB*)database->adapter_hdl;
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
    logwarn("No schema defined for table '%s'\n");
    return -1;
  }
  MString *insert = NULL, *create = NULL;
  Sq3DB* sq3db = (Sq3DB*)db->adapter_hdl;

  if (backend_create) {
    create = schema_to_sql (table->schema, oml_to_sql_type);
    if (!create) {
      logwarn("Failed to build SQL CREATE TABLE statement string for table %s.\n",
              table->schema->name);
      goto fail_exit;
    }
    if (sql_stmt(sq3db, mstring_buf(create))) {
      logerror("Could not create table: (%s).\n", sqlite3_errmsg(sq3db->db_hdl));
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
  table->adapter_hdl = sq3table;
  if (sqlite3_prepare_v2(sq3db->db_hdl, mstring_buf(insert), -1,
                         &sq3table->insert_stmt, 0) != SQLITE_OK) {
    logerror("Could not prepare statement (%s).\n", sqlite3_errmsg(sq3db->db_hdl));
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
 * represent the table, and then also issue an SQL CREATE TABLE
 * statement to the libsqlite3 library to actually create the table in
 * the backend.
 *
 * Return 0 on success, -1 on failure.
 */
int
sq3_table_create (Database *database, DbTable *table)
{
  return table_create (database, table, 1);
}

int
sq3_table_free (Database *database, DbTable* table)
{
  (void)database;
  Sq3Table* sq3table = (Sq3Table*)table->adapter_hdl;
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
  Sq3DB* sq3db = (Sq3DB*)db->adapter_hdl;
  Sq3Table* sq3table = (Sq3Table*)table->adapter_hdl;
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
        sqlite3_errmsg(sq3db->db_hdl));
  }
  if (sqlite3_bind_int(stmt, 2, seq_no) != SQLITE_OK) {
    logerror("Could not bind 'oml_seq' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }
  if (sqlite3_bind_double(stmt, 3, time_stamp) != SQLITE_OK) {
    logerror("Could not bind 'oml_ts_client' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
  }
  if (sqlite3_bind_double(stmt, 4, time_stamp_server) != SQLITE_OK) {
    logerror("Could not bind 'oml_ts_server' (%s).\n",
        sqlite3_errmsg(sq3db->db_hdl));
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
               schema->fields[i].name, sqlite3_errmsg(sq3db->db_hdl));
      sqlite3_reset (stmt);
      return -1;
    }
  }

  if (sqlite3_step(stmt) != SQLITE_DONE) {
    logerror("Could not step (execute) SQL statement in sq3_insert().  (%s)\n",
             sqlite3_errmsg(sq3db->db_hdl));
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

  ret = sqlite3_exec(self->db_hdl, stmt, select_callback, &nrecs, &errmsg);

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
  ret = sqlite3_exec(self->db_hdl, stmt, 0, 0, &errmsg);

  if (ret != SQLITE_OK) {
    logwarn("Error(%d) in statement: %s [%s].\n", ret, stmt, errmsg);
    return -1;
  }
  return 0;
}

TableDescr*
sq3_get_table_list (Sq3DB* self, int *num_tables)
{
  const char* stmt = "SELECT name,sql FROM sqlite_master WHERE type='table' ORDER BY name;";
  char* errmsg;
  char** result;
  int nrows;
  int ncols;
  int ret = sqlite3_get_table (self->db_hdl, stmt, &result, &nrows, &ncols, &errmsg);

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
  for (i = name_col + ncols,
         j = schema_col + ncols;
       i < (nrows + 1) * ncols;
       i += ncols,
         j+= ncols,
         n++)
    {
      TableDescr* t = table_descr_new (result[i], result[j]);
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

  Sq3DB* sq3db = (Sq3DB*) database->adapter_hdl;
  char s[512];
  const char* stmt_fmt = "SELECT %s,%s FROM %s WHERE %s='%s';";
  char* errmsg;
  char** result;
  int nrows;
  int ncols;

  size_t n = snprintf (s, LENGTH(s), stmt_fmt, key_column, value_column, table, key_column, key);

  if (n >= LENGTH(s))
    logwarn("Key-value lookup for key '%s' in %s(%s, %s): SELECT statement had to be truncated\n",
            key, table, key_column, value_column);

  int ret = sqlite3_get_table (sq3db->db_hdl, s, &result, &nrows, &ncols, &errmsg);

  if (ret != SQLITE_OK)
    {
      logerror("Error in SELECT statement %s [%s].\n", stmt_fmt, errmsg);
      sqlite3_free (errmsg);
      return NULL;
    }

  if (ncols == 0 || nrows == 0)
    {
      sqlite3_free_table (result);
      return NULL;
    }

  if (nrows > 1)
      logwarn("Key-value lookup for key '%s' in %s(%s, %s) returned more than one possible key.\n",
              key, table, key_column, value_column);

  char* value = NULL;

  if (strcmp (key, result[2]) == 0)
    {
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
  Sq3DB* sq3db = (Sq3DB*) database->adapter_hdl;
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

  if (n >= LENGTH (stmt))
    {
      logwarn("SQL statement too long trying to update key-value pair %s='%s' in %s(%s, %s)\n",
              key, value, table, key_column, value_column);
      return -1;
    }

  if (sql_stmt (sq3db, stmt))
    {
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

  Sq3DB* sq3db = (Sq3DB*) database->adapter_hdl;
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

  int ret = sqlite3_get_table (sq3db->db_hdl, stmt, &result, &nrows, &ncols, &errmsg);

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
  char fname[128];
  sqlite3* db_hdl;
  int rc;
  snprintf(fname, LENGTH (fname) - 1, "%s/%s.sq3", g_database_data_dir, db->name);
  rc = sqlite3_open(fname, &db_hdl);
  if (rc) {
    logerror("Can't open database: %s\n", sqlite3_errmsg(db_hdl));
    sqlite3_close(db_hdl);
    return -1;
  }

  Sq3DB* self = xmalloc(sizeof(Sq3DB));
  self->db_hdl = db_hdl;
  self->last_commit = time (NULL);
  db->table_create = sq3_table_create;
  db->table_free = sq3_table_free;
  db->release = sq3_release;
  db->insert = sq3_insert;
  db->add_sender_id = sq3_add_sender_id;
  db->set_metadata = sq3_set_metadata;
  db->get_metadata = sq3_get_metadata;
  db->get_max_seq_no = sq3_get_max_seq_no;

  db->adapter_hdl = self;

  int num_tables;
  TableDescr* tables = sq3_get_table_list (self, &num_tables);
  TableDescr* td = tables;

  logdebug("Got table list with %d tables in it\n", num_tables);
  int i = 0;
  for (i = 0; i < num_tables; i++, td = td->next) {
    // Don't try to treat the metadata tables as measurement tables.
    if (strcmp (td->name, "_experiment_metadata") == 0 ||
        strcmp (td->name, "_senders") == 0)
      continue;

    struct schema *schema = schema_from_sql (td->schema);
    if (!schema) {
      logwarn ("Failed to create table '%s': error parsing schema (not created by OML?):\n%s\n",
               td->name, td->schema);
      continue;
    }

    DbTable *table = database_create_table (db, schema);
    schema_free (schema);
    if (!table) {
      logwarn ("Failed to create table '%s': allocation failed\n",
               td->name);
      continue;
    }
    /* Create the required table data structures, but don't do SQL CREATE TABLE */
    if (table_create (db, table, 0) == -1) {
      logwarn ("Failed to create adapter structures for table '%s'\n",
               td->name);
      database_table_free (db, table);
    }
  }

  if (!table_descr_have_table (tables, "_experiment_metadata"))
    {
      if (sql_stmt (self, SQL_CREATE_EXPT_METADATA))
        {
          table_descr_list_free (tables);
          return -1;
        }
    }

  if (!table_descr_have_table (tables, "_senders"))
    {
      if (sql_stmt (self, SQL_CREATE_SENDERS))
        {
          table_descr_list_free (tables);
          return -1;
        }
    }

  table_descr_list_free (tables);
  begin_transaction (self);
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
