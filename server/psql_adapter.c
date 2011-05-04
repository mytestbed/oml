#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <libpq-fe.h>
#include <log.h>
#include <mem.h>
#include <util.h>
#include <mstring.h>
#include <time.h>
#include <sys/time.h>
#include "database.h"

#define CONN_BUFFER 512

typedef struct _psqlDB {
  PGconn *conn;
  int sender_cnt;
} PsqlDB;

typedef struct _pqlTable {
  MString *insert_stmt; /* Named statement for inserting into this table */
} PsqlTable;

static int sql_stmt(PsqlDB* self, const char* stmt);
char* psql_get_key_value (Database *database, const char *table,
                          const char *key_column, const char *value_column,
                          const char *key);
int psql_set_key_value (Database *database, const char *table,
                        const char *key_column, const char *value_column,
                        const char *key, const char *value);

static int first_row;

static MString*
psql_make_sql_insert (DbTable* table)
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
  n += mstring_cat (mstr, "\" VALUES ($1, $2, $3, $4"); /* metadata columns */
  while (max-- > 0)
    mstring_sprintf (mstr, ", $%d", 4 + table->schema->nfields - max);
  mstring_cat (mstr, ");");

  if (n != 0) goto fail_exit;
  return mstr;

 fail_exit:
  if (mstr) mstring_delete (mstr);
  return NULL;
}

/**
 * \fn void psql_release(Database* db)
 * \brief Release the psql database
 * \param db the database that contains the psql database
 */
void
psql_release(Database* db)
{
  PsqlDB* self = (PsqlDB*)db->adapter_hdl;
  PQfinish(self->conn);
  // TODO: Release table specific data

  xfree(self);
  db->adapter_hdl = NULL;
}
/**
 * \fn static int psql_add_sender_id(Database* db, char* sender_id)
 * \brief  Add sender ID to the table
 * \param db the database that contains the sqlite3 db
 * \param sender_id the sender ID
 * \return the index of the sender
 */
static int
psql_add_sender_id(Database* db, char *sender_id)
{
  (void)db;
  //PsqlDB* self = (PsqlDB*)db->adapter_hdl;
  //  int index = ++self->sender_cnt;

  //char s[512];  // don't check for length
  // TDEBUG
  //sprintf(s, "INSERT INTO _senders VALUES (%d, '%s');\0", index, sender_id);
  //if (sql_stmt(self, s)) return -1;
  //return index;
  return atoi(sender_id);
}

char*
psql_get_metadata (Database *db, const char *key)
{
  return psql_get_key_value (db, "_experiment_metadata", "key", "value", key);
}

int
psql_set_metadata (Database *db, const char *key, const char *value)
{
  return psql_set_key_value (db, "_experiment_metadata", "key", "value", key, value);
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
psql_get_key_value (Database *database, const char *table,
                    const char *key_column, const char *value_column,
                    const char *key)
{
  if (database == NULL || table == NULL || key_column == NULL ||
      value_column == NULL || key == NULL)
    return NULL;

  PGresult *res;
  PsqlDB *psqldb = (PsqlDB*) database->adapter_hdl;
  MString *stmt = mstring_create();
  mstring_sprintf (stmt, "SELECT %s FROM %s WHERE %s='%s';",
                   value_column, table, key_column, key);

  res = PQexec (psqldb->conn, mstring_buf (stmt));

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("Error trying to get %s[%s]; (%s).\n",
             table, key, PQerrorMessage(psqldb->conn));
    goto fail_exit;
  }

  if (PQntuples (res) == 0)
    goto fail_exit;
  if (PQnfields (res) < 1)
    goto fail_exit;

  if (PQntuples (res) > 1)
    logwarn ("Key-value lookup for key '%s' in %s(%s, %s) returned more than one possible key.\n",
             key, table, key_column, value_column);

  char *value = NULL;
  value = PQgetvalue (res, 0, 0);

  if (value != NULL)
    value = xstrndup (value, strlen (value));

  PQclear (res);
  mstring_delete (stmt);
  return value;

 fail_exit:
  PQclear (res);
  mstring_delete (stmt);
  return NULL;
}

int
psql_set_key_value (Database *database, const char *table,
                    const char *key_column, const char *value_column,
                    const char *key, const char *value)
{
  PsqlDB *psqldb = (PsqlDB*) database->adapter_hdl;
  MString *stmt = mstring_create ();
  char *check_value = psql_get_key_value (database, table, key_column, value_column, key);
  if (check_value == NULL)
    mstring_sprintf (stmt, "INSERT INTO \"%s\" (\"%s\", \"%s\") VALUES ('%s', '%s');",
                     table, key_column, value_column, key, value);
  else
    mstring_sprintf (stmt, "UPDATE \"%s\" SET \"%s\"='%s' WHERE \"%s\"='%s';",
                     table, value_column, value, key_column, key);

  if (sql_stmt (psqldb, mstring_buf (stmt))) {
    logwarn("Key-value update failed for %s='%s' in %s(%s, %s) (database error)\n",
            key, value, table, key_column, value_column);
    return -1;
  }

  return 0;
}

long
psql_get_time_offset (Database *db, long start_time)
{
  (void)db, (void)start_time;
  return 0;
}

long
psql_get_max_seq_no (Database *db, DbTable *table, int sender_id)
{
  (void)db, (void)table, (void)sender_id;
  return 0;
}

/**
 * @brief Create a sqlite3 table
 * @param db the database that contains the sqlite3 db
 * @param table the table to associate in sqlite3 database
 * @return 0 if successful, -1 otherwise
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
  PsqlDB* psqldb = (PsqlDB*)db->adapter_hdl;
  PGresult *res;

  if (backend_create) {
    create = schema_to_sql (table->schema, oml_to_postgresql_type);
    if (!create) {
      logwarn ("Failed to build SQL CREATE TABLE statement string for table %s.\n",
               table->schema->name);
      goto fail_exit;
    }
    if (sql_stmt (psqldb, mstring_buf (create))) {
      logerror ("Could not create table: (%s).\n", PQerrorMessage (psqldb->conn));
      goto fail_exit;
    }
  }

  insert = psql_make_sql_insert (table);
  if (!insert) {
    logwarn ("Failed to build SQL INSERT INTO statement for table '%s'.\n",
             table->schema->name);
    goto fail_exit;
  }
  /* Prepare the insert statement and update statement  */
  PsqlTable* psqltable = (PsqlTable*)xmalloc(sizeof(PsqlTable));
  table->adapter_hdl = psqltable;

  MString *insert_name = mstring_create();
  mstring_set (insert_name, "OMLInsert-");
  mstring_cat (insert_name, table->schema->name);
  res = PQprepare(psqldb->conn,
                  mstring_buf (insert_name),
                  mstring_buf (insert),
                  table->schema->nfields + 4, // FIXME:  magic number of metadata cols
                  NULL);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("Could not prepare statement (%s).\n", PQerrorMessage(psqldb->conn));
    PQclear(res);
    goto fail_exit;
    return -1;
  }
  PQclear(res);
  psqltable->insert_stmt = insert_name;

  if (create) mstring_delete (create);
  if (insert) mstring_delete (insert);
  return 0;

 fail_exit:
  if (create) mstring_delete (create);
  if (insert) mstring_delete (insert);
  return -1;
}

int
psql_create_table (Database *database, DbTable *table)
{
  return table_create (database, table, 1);
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
psql_insert(Database* db,
            DbTable*  table,
            int       sender_id,
            int       seq_no,
            double    time_stamp,
            OmlValue* values,
            int       value_count)
{
  PsqlDB* psqldb = (PsqlDB*)db->adapter_hdl;
  PsqlTable* psqltable = (PsqlTable*)table->adapter_hdl;
  PGresult* res;
  int i;
  double time_stamp_server;
  const char* insert_stmt = mstring_buf (psqltable->insert_stmt);

  char * paramValues[4+value_count];
  for (i=0;i<4+value_count;i++) {
    paramValues[i] = malloc(512*sizeof(char));
  }

  int paramLength[4+value_count];
  int paramFormat[4+value_count];

  //logdebug("insert");
  logdebug("TDEBUG - into psql_insert - %d \n", seq_no);

  sprintf(paramValues[0],"%i",sender_id);
  paramLength[0] = 0;
  paramFormat[0] = 0;

  sprintf(paramValues[1],"%i",seq_no);
  paramLength[1] = 0;
  paramFormat[1] = 0;

  sprintf(paramValues[2],"%.8f",time_stamp);
  paramLength[2] = 0;
  paramFormat[2] = 0;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_stamp_server = tv.tv_sec - db->start_time + 0.000001 * tv.tv_usec;

  sprintf(paramValues[3],"%.8f",time_stamp_server);
  paramLength[3] = 0;
  paramFormat[3] = 0;

  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    struct schema_field *field = &table->schema->fields[i];
    if (v->type != field->type) {
      logerror("Mismatch in %dth value type fo rtable '%s'\n", i, table->schema->name);
      return -1;
    }
    switch (field->type) {
    case OML_LONG_VALUE: sprintf(paramValues[4+i],"%i",(int)v->value.longValue); break;
    case OML_INT32_VALUE:  sprintf(paramValues[4+i],"%" PRId32,v->value.int32Value); break;
    case OML_UINT32_VALUE: sprintf(paramValues[4+i],"%" PRIu32,v->value.uint32Value); break;
    case OML_INT64_VALUE:  sprintf(paramValues[4+i],"%" PRId64,v->value.int64Value); break;
//    case OML_UINT64_VALUE: sprintf(paramValues[4+i],"%i" PRIu64,(int)v->value.uint64Value); break;
    case OML_DOUBLE_VALUE: sprintf(paramValues[4+i],"%.8f",v->value.doubleValue); break;
    case OML_STRING_VALUE: sprintf(paramValues[4+i],"%s",v->value.stringValue.ptr); break;
      //    case OML_BLOB_VALUE: sprintf(paramValues[4+i],"%s",v->value.blobValue.ptr); break;
    default:
      logerror("Bug: Unknown type %d in col '%s'\n", field->type, field->name);
      return -1;
    }
    paramLength[4+i] = 0;
    paramFormat[4+i] = 0;
  }
  logdebug("TDEBUG - into psql_insert - %d \n", seq_no);

  /* Use stuff from http://www.postgresql.org/docs/current/static/plpgsql-control-structures.html#PLPGSQL-ERROR-TRAPPING */

  res = PQexecPrepared(psqldb->conn, insert_stmt,
                       4+value_count, (const char**)paramValues,
                       (int*) &paramLength, (int*) &paramFormat, 0 );

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("Exec of prepared INSERT INTO failed: %s\n", PQerrorMessage(psqldb->conn));
    PQclear(res);
    return -1;
  }
  PQclear(res);

  for (i=0;i<4+value_count;i++) {
    free(paramValues[i]);
  }

  return 0;
}

/**
 * \fn static int select_callback(void*  p_data, int    num_fields, char** p_fields, char** p_col_names)
 * \brief
 * \param p_data
 * \param num_fields
 * \param p_fields
 * \param p_col_names
 * \return
 */
static int
select_callback(
  void*  p_data,
  int    num_fields,
  char** p_fields,
  char** p_col_names
) {

  int i;

  int* nof_records = (int*) p_data;
  (*nof_records)++;

  if (first_row) {
    first_row = 0;

    for (i=0; i < num_fields; i++) {
      printf("%20s", p_col_names[i]);
    }

    printf("\n");
    for (i=0; i< num_fields*20; i++) {
      printf("=");
    }
    printf("\n");
  }

  for(i=0; i < num_fields; i++) {
    if (p_fields[i]) {
      printf("%20s", p_fields[i]);
    }
    else {
      printf("%20s", " ");
    }
  }

  printf("\n");
  return 0;
}

/**
 * @brief Execute an SQL statement (using PQexec()).
 *
 * This function executes a statement with the assumption that the
 * result can be ignored; that is, it's not useful for SELECT
 * statements.
 *
 * @param self the database handle.
 * @param stmt the SQL statement to execute.
 * @return 0 if successful, -1 if the database reports an error.
 */
static int
sql_stmt(PsqlDB* self, const char* stmt)
{
  PGresult   *res;
  logdebug("prepare to exec %s \n", stmt);
  res = PQexec(self->conn, stmt);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("Error in statement: %s [%s].\n", stmt, PQerrorMessage(self->conn));
    PQclear(res);
    return -1;
  }
  /*
   * Should PQclear PGresult whenever it is no longer needed to avoid memory
   * leaks
   */
  PQclear(res);

  return 0;
}

/**
 * @brief Create a sqlite3 database
 * @param db the databse to associate with the sqlite3 database
 * @return 0 if successful, -1 otherwise
 */
int
psql_create_database(Database* db)
{
  char conninfo[CONN_BUFFER];
  PGconn     *conn;

  /* Setup parameters to connect to the database */
  /* Assumes we use a $HOME/.pgpass file:
     http://www.postgresql.org/docs/8.2/interactive/libpq-pgpass.html*/
  sprintf(conninfo,"host=%s user=%s dbname=%s",db->hostname,db->user,db->name);

  logdebug("Connection info: %s\n",conninfo);

  /* Make a connection to the database */
  conn = PQconnectdb(conninfo);

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK) {
    logerror("Connection to database failed: %s",
      PQerrorMessage(conn));
    return -1;
  }

  PsqlDB* self = (PsqlDB*)xmalloc(sizeof(PsqlDB));
  self->conn = conn;

  db->create = psql_create_database;
  db->release = psql_release;
  db->table_create = psql_create_table;
  db->insert = psql_insert;
  db->add_sender_id = psql_add_sender_id;
  db->get_metadata = psql_get_metadata;
  db->set_metadata = psql_set_metadata;
  db->get_max_seq_no = psql_get_max_seq_no;

  db->adapter_hdl = self;

  /* FIXME:  Do all the reconnect stuff with auto-detecting existing tables and so-on */

  return 0;
}
