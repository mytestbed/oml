/*
 * Copyright 2010 TU Berlin, Germany (Ruben Merz)
 * Copyright 2010-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file psql_adapter.c
 * \brief Adapter code for the PostgreSQL database backend.
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <libpq-fe.h>
#include <time.h>
#include <sys/time.h>
#include <arpa/inet.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mstring.h"
#include "oml_value.h"
#include "oml_util.h"
#include "database.h"
#include "database_adapter.h"
#include "psql_adapter.h"

static char backend_name[] = "psql";
/* Cannot be static due to the way the server sets its parameters */
char *pg_host = DEFAULT_PG_HOST;
char *pg_port = DEFAULT_PG_PORT;
char *pg_user = DEFAULT_PG_USER;
char *pg_pass = DEFAULT_PG_PASS;
char *pg_conninfo = DEFAULT_PG_CONNINFO;

/** Mapping between OML and PostgreSQL data types
 * \see psql_type_to_oml, psql_oml_to_type
 */
static struct {
  OmlValueT type;           /** OML type */
  const char * const name;  /** PostgreSQL equivalent */
} psql_type_pair[] = {
  { OML_LONG_VALUE,    "INT4" },
  { OML_DOUBLE_VALUE,  "FLOAT8" },
  { OML_STRING_VALUE,  "TEXT" },
  { OML_BLOB_VALUE,    "BYTEA" },
  { OML_INT32_VALUE,   "INT4" },
  { OML_UINT32_VALUE,  "INT8" }, /* PG doesn't support unsigned types --> promote */
  { OML_INT64_VALUE,   "INT8" },
  { OML_UINT64_VALUE,  "BIGINT" },
};

static int sql_stmt(PsqlDB* self, const char* stmt);

/* Functions needed by the Database struct */
static OmlValueT psql_type_to_oml (const char *s);
static const char* psql_oml_to_type (OmlValueT type);
static int psql_stmt(Database* db, const char* stmt);
static void psql_release(Database* db);
static int psql_table_create (Database* db, DbTable* table, int shallow);
static int psql_table_free (Database *database, DbTable* table);
static int psql_insert(Database *db, DbTable *table, int sender_id, int seq_no, double time_stamp, OmlValue *values, int value_count);
static char* psql_get_key_value (Database* database, const char* table, const char* key_column, const char* value_column, const char* key);
static int psql_set_key_value (Database* database, const char* table, const char* key_column, const char* value_column, const char* key, const char* value);
static char* psql_get_metadata (Database* database, const char* key);
static int psql_set_metadata (Database* database, const char* key, const char* value);
static int psql_add_sender_id(Database* database, const char* sender_id);
static char* psql_get_uri(Database *db, char *uri, size_t size);
static TableDescr* psql_get_table_list (Database *database, int *num_tables);

static MString* psql_prepare_conninfo(const char *database, const char *host, const char *port, const char *user, const char *pass, const char *extra_conninfo);
static char* psql_get_sender_id (Database* database, const char* name);
static int psql_set_sender_id (Database* database, const char* name, int id);
static MString* psql_make_sql_insert (DbTable* table);
static void psql_receive_notice(void *arg, const PGresult *res);

/** Prepare the conninfo string to connect to the Postgresql server.
 *
 * \param host server hostname
 * \param port server port or service name
 * \param user username
 * \param pass password
 * \param extra_conninfo additional connection parameters
 * \return a dynamically allocated MString containing the connection information; the caller must take care of freeing it
 *
 * \see mstring_delete, PQconnectdb
 */
static MString*
psql_prepare_conninfo(const char *database, const char *host, const char *port, const char *user, const char *pass, const char *extra_conninfo)
{
  MString *conninfo;
  int portnum;

  portnum = resolve_service(port, 5432);
  conninfo = mstring_create();
  mstring_sprintf (conninfo, "host='%s' port='%d' user='%s' password='%s' dbname='%s' %s",
      host, portnum, user, pass, database, extra_conninfo);

  return conninfo;
}

/** Setup the PostgreSQL backend.
 *
 * \return 0 on success, -1 otherwise
 *
 * \see database_setup_backend
 */
int
psql_backend_setup (void)
{
  MString *str, *conninfo;

  loginfo ("psql: Sending experiment data to PostgreSQL server %s:%s as user '%s'\n",
           pg_host, pg_port, pg_user);

  conninfo = psql_prepare_conninfo("postgres", pg_host, pg_port, pg_user, pg_pass, pg_conninfo);
  PGconn *conn = PQconnectdb (mstring_buf (conninfo));

  if (PQstatus (conn) != CONNECTION_OK) {
    logerror ("psql: Could not connect to PostgreSQL database (conninfo \"%s\"): %s\n",
         mstring_buf(conninfo), PQerrorMessage (conn));
    mstring_delete(conninfo);
    return -1;
  }

  /* oml2-server must be able to create new databases, so check that
     our user has the required role attributes */
  str = mstring_create();
  mstring_sprintf (str, "SELECT rolcreatedb FROM pg_roles WHERE rolname='%s'", pg_user);
  PGresult *res = PQexec (conn, mstring_buf (str));
  mstring_delete(str);
  if (PQresultStatus (res) != PGRES_TUPLES_OK) {
    logerror ("psql: Failed to determine role privileges for role '%s': %s\n",
         pg_user, PQerrorMessage (conn));
    return -1;
  }
  char *has_create = PQgetvalue (res, 0, 0);
  if (strcmp (has_create, "t") == 0)
    logdebug ("psql: User '%s' has CREATE DATABASE privileges\n", pg_user);
  else {
    logerror ("psql: User '%s' does not have required role CREATE DATABASE\n", pg_user);
    return -1;
  }

  mstring_delete(conninfo);
  PQclear (res);
  PQfinish (conn);

  return 0;
}

/** Mapping from PostgreSQL to OML types.
 * \see db_adapter_type_to_oml
 */
static
OmlValueT psql_type_to_oml (const char *type)
{
  int i = 0;
  int n = LENGTH(psql_type_pair);

  for (i = 0; i < n; i++) {
    if (strcmp (type, psql_type_pair[i].name) == 0) {
        return psql_type_pair[i].type;
    }
  }
  logwarn("Unknown PostgreSQL type '%s', using OML_UNKNOWN_VALUE\n", type);
  return OML_UNKNOWN_VALUE;
}

/** Mapping from OML types to PostgreSQL types.
 * \see db_adapter_oml_to_type
 */
static const char*
psql_oml_to_type (OmlValueT type)
{
  int i = 0;
  int n = LENGTH(psql_type_pair);

  for (i = 0; i < n; i++) {
    if (psql_type_pair[i].type == type) {
        return psql_type_pair[i].name;
    }
  }
  logerror("Unknown OML type %d\n", type);
  return NULL;
}

/** Execute an SQL statement (using PQexec()).
 * \see db_adapter_stmt, PQexec
 */
static int
sql_stmt(PsqlDB* self, const char* stmt)
{
  PGresult   *res;
  logdebug("psql: Will execute '%s'\n", stmt);
  res = PQexec(self->conn, stmt);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("psql: Error executing '%s': %s\n", stmt, PQerrorMessage(self->conn));
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

/** Type-agnostic wrapper for sql_stmt */
static int
psql_stmt(Database* db, const char* stmt)
{
 return sql_stmt((PsqlDB*)db->handle, stmt);
}

/** Create or open an PostgreSQL database
 * \see db_adapter_create
 */
int
psql_create_database(Database* db)
{
  MString *conninfo;
  MString *str;
  PGconn  *conn;
  PGresult *res = NULL;
  int ret = -1;

  loginfo ("psql:%s: Accessing database\n", db->name);

  /*
   * Make a connection to the database server -- check if the
   * requested database exists or not by connecting to the 'postgres'
   * database and querying that.
   */
  conninfo = psql_prepare_conninfo("postgres", pg_host, pg_port, pg_user, pg_pass, pg_conninfo);
  conn = PQconnectdb(mstring_buf (conninfo));

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK) {
    logerror ("psql: Could not connect to PostgreSQL database (conninfo \"%s\"): %s\n",
         mstring_buf(conninfo), PQerrorMessage (conn));
    goto cleanup_exit;
  }
  PQsetNoticeReceiver(conn, psql_receive_notice, "postgres");

  str = mstring_create();
  mstring_sprintf (str, "SELECT datname from pg_database where datname='%s';", db->name);
  res = PQexec (conn, mstring_buf (str));

  if (PQresultStatus (res) != PGRES_TUPLES_OK) {
    logerror ("psql: Could not get list of existing databases\n");
    goto cleanup_exit;
  }

  /* No result rows means database doesn't exist, so create it instead */
  if (PQntuples (res) == 0) {
    loginfo ("psql:%s: Database does not exist, creating it\n", db->name);
    mstring_set (str, "");
    mstring_sprintf (str, "CREATE DATABASE \"%s\";", db->name);

    res = PQexec (conn, mstring_buf (str));
    if (PQresultStatus (res) != PGRES_COMMAND_OK) {
      logerror ("psql:%s: Could not create database: %s\n", db->name, PQerrorMessage (conn));
      goto cleanup_exit;
    }
  }

  PQfinish (conn);
  mstring_delete(conninfo);

  /* Now that the database should exist, make a connection to the it for real */
  conninfo = psql_prepare_conninfo(db->name, pg_host, pg_port, pg_user, pg_pass, pg_conninfo);
  conn = PQconnectdb(mstring_buf (conninfo));

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK) {
    logerror ("psql:%s: Could not connect to PostgreSQL database (conninfo \"%s\"): %s\n",
        db->name, mstring_buf(conninfo), PQerrorMessage (conn));
    goto cleanup_exit;
  }
  PQsetNoticeReceiver(conn, psql_receive_notice, db->name);

  PsqlDB* self = (PsqlDB*)xmalloc(sizeof(PsqlDB));
  self->conn = conn;
  self->last_commit = time (NULL);

  db->backend_name = backend_name;
  db->o2t = psql_oml_to_type;
  db->t2o = psql_type_to_oml;
  db->stmt = psql_stmt;
  db->create = psql_create_database;
  db->release = psql_release;
  db->table_create = psql_table_create;
  db->table_create_meta = dba_table_create_meta;
  db->table_free = psql_table_free;
  db->insert = psql_insert;
  db->add_sender_id = psql_add_sender_id;
  db->get_metadata = psql_get_metadata;
  db->set_metadata = psql_set_metadata;
  db->get_uri = psql_get_uri;
  db->get_table_list = psql_get_table_list;

  db->handle = self;

  dba_begin_transaction (db);

  /* Everything was successufl, prepare for cleanup */
  ret = 0;

cleanup_exit:
  if (res) { PQclear (res) ; }
  if (ret) { PQfinish (conn); } /* If return !=0, cleanup connection */

  mstring_delete (str);
  mstring_delete (conninfo);
  return ret;
}

/** Release the psql database.
 * \see db_adapter_release
 */
static void
psql_release(Database* db)
{
  PsqlDB* self = (PsqlDB*)db->handle;
  dba_end_transaction (db);
  PQfinish(self->conn);
  xfree(self);
  db->handle = NULL;
}

/** Create a PostgreSQL database and adapter structures
 * \see db_adapter_create
 */
/* This function is exposed to the rest of the code for backend initialisation */
int
psql_table_create (Database *db, DbTable *table, int shallow)
{
  MString *insert = NULL, *create = NULL, *meta_skey= NULL, *insert_name = NULL;
  char *meta_svalue = NULL;
  PsqlDB* psqldb = NULL;
  PGresult *res = NULL;
  PsqlTable* psqltable = NULL;

  logdebug("psql:%s: Creating table '%s' (shallow=%d)\n", db->name, table->schema->name, shallow);

  if (db == NULL) {
    logerror("psql: Tried to create a table in a NULL database\n");
    return -1;
  }
  if (table == NULL) {
    logerror("psql:%s: Tried to create a table from a NULL definition\n", db->name);
    return -1;
  }
  if (table->schema == NULL) {
    logerror("psql:%s: No schema defined for table, cannot create\n", db->name);
    return -1;
  }
  psqldb = (PsqlDB*)db->handle;

  if (!shallow) {
    create = schema_to_sql (table->schema, psql_oml_to_type);
    if (!create) {
      logerror("psql:%s: Failed to build SQL CREATE TABLE statement string for schema '%s'\n",
          db->name, schema_to_meta(table->schema));
      goto fail_exit;
    }
    if (sql_stmt (psqldb, mstring_buf (create))) {
      logerror("psql:%s: Could not create table '%s': %s\n",
          db->name, table->schema->name,
          PQerrorMessage (psqldb->conn));
      goto fail_exit;
    }

    /* The schema index is irrelevant in the metadata, temporarily drop it */
    int sindex = table->schema->index;
    table->schema->index = -1;
    meta_skey = mstring_create ();
    mstring_sprintf (meta_skey, "table_%s", table->schema->name);
    meta_svalue = schema_to_meta (table->schema);
    table->schema->index = sindex;
    psql_set_metadata (db, mstring_buf (meta_skey), meta_svalue);
    mstring_delete(meta_skey);
    xfree(meta_svalue);
  }

  insert = psql_make_sql_insert (table);
  if (!insert) {
    logerror("psql:%s: Failed to build SQL INSERT INTO statement for table '%s'\n",
          db->name, table->schema->name);
    goto fail_exit;
  }
  /* Prepare the insert statement and update statement  */
  psqltable = (PsqlTable*)xmalloc(sizeof(PsqlTable));
  table->handle = psqltable;

  insert_name = mstring_create();
  mstring_set (insert_name, "OMLInsert-");
  mstring_cat (insert_name, table->schema->name);
  res = PQprepare(psqldb->conn,
                  mstring_buf (insert_name),
                  mstring_buf (insert),
                  table->schema->nfields + 4, // FIXME:  magic number of metadata cols
                  NULL);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("psql:%s: Could not prepare statement: %s\n",
        db->name, PQerrorMessage(psqldb->conn));
    PQclear(res);
    goto fail_exit;
    return -1;
  }
  PQclear(res);
  psqltable->insert_stmt = insert_name;

  if (create) { mstring_delete (create); }
  if (insert) { mstring_delete (insert); }
  return 0;

fail_exit:
  if (create) { mstring_delete (create); }
  if (insert) { mstring_delete (insert); }
  if (insert_name) { mstring_delete (insert_name); }
  if (psqltable) { xfree (psqltable); }
  return -1;
}

/** Free a PostgreSQL table
 *
 * Parameter database is ignored in this implementation
 *
 * \see db_adapter_table_free
 */
static int
psql_table_free (Database *database, DbTable *table)
{
  (void)database;
  PsqlTable *psqltable = (PsqlTable*)table->handle;
  if (psqltable) {
    mstring_delete (psqltable->insert_stmt);
    xfree (psqltable);
  }
  return 0;
}

/** Insert value in the PostgreSQL database.
 * \see db_adapter_insert
 */
static int
psql_insert(Database* db, DbTable* table, int sender_id, int seq_no, double time_stamp, OmlValue* values, int value_count)
{
  PsqlDB* psqldb = (PsqlDB*)db->handle;
  PsqlTable* psqltable = (PsqlTable*)table->handle;
  PGresult* res;
  int i;
  double time_stamp_server;
  const char* insert_stmt = mstring_buf (psqltable->insert_stmt);
  unsigned char *escaped_blob;
  size_t eblob_len=-1;

  char * paramValues[4+value_count];
  for (i=0;i<4+value_count;i++) {
    paramValues[i] = xmalloc(512*sizeof(char));
  }

  int paramLength[4+value_count];
  int paramFormat[4+value_count];

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

  if (tv.tv_sec > psqldb->last_commit) {
    if (dba_reopen_transaction (db) == -1) {
      return -1;
    }
    psqldb->last_commit = tv.tv_sec;
  }

  sprintf(paramValues[3],"%.8f",time_stamp_server);
  paramLength[3] = 0;
  paramFormat[3] = 0;

  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    struct schema_field *field = &table->schema->fields[i];
    if (oml_value_get_type(v) != field->type) {
      logerror("psql:%s: Value %d type mismatch for table '%s'\n", db->name, i, table->schema->name);
      return -1;
    }
    switch (field->type) {
    case OML_LONG_VALUE: sprintf(paramValues[4+i],"%i",(int)v->value.longValue); break;
    case OML_INT32_VALUE:  sprintf(paramValues[4+i],"%" PRId32,v->value.int32Value); break;
    case OML_UINT32_VALUE: sprintf(paramValues[4+i],"%" PRIu32,v->value.uint32Value); break;
    case OML_INT64_VALUE:  sprintf(paramValues[4+i],"%" PRId64,v->value.int64Value); break;
    case OML_UINT64_VALUE: sprintf(paramValues[4+i],"%" PRIu64,v->value.uint64Value); break;
    case OML_DOUBLE_VALUE: sprintf(paramValues[4+i],"%.8f",v->value.doubleValue); break;
    case OML_STRING_VALUE: sprintf(paramValues[4+i],"%s", omlc_get_string_ptr(*oml_value_get_value(v))); break;
    case OML_BLOB_VALUE:
                           escaped_blob = PQescapeByteaConn(psqldb->conn,
                               v->value.blobValue.ptr, v->value.blobValue.length, &eblob_len);
                           if (!escaped_blob) {
                             logerror("psql:%s: Error escaping blob in field %d of table '%s': %s\n",
                                 db->name, i, table->schema->name, PQerrorMessage(psqldb->conn));
                           }
                           /* XXX: 512 char is the size allocated above. Nasty. */
                           if (eblob_len > 512) {
                             logdebug("psql:%s: Reallocating %d bytes for big blob\n", db->name, eblob_len);
                             paramValues[4+i] = xrealloc(paramValues[4+i], eblob_len);
                             if (!paramValues[4+i]) {
                               logerror("psql:%s: Could not realloc()at memory for escaped blob in field %d of table '%s'\n",
                                   db->name, i, table->schema->name);
                               return -1;
                             }
                           }
                           snprintf(paramValues[4+i], eblob_len, "%s", escaped_blob);
                           PQfreemem(escaped_blob);
                           break;
    default:
      logerror("psql:%s: Unknown type %d in col '%s' of table '%s'; this is probably a bug\n",
          db->name, field->type, field->name, table->schema->name);
      return -1;
    }
    paramLength[4+i] = 0;
    paramFormat[4+i] = 0;
  }
  /* Use stuff from http://www.postgresql.org/docs/current/static/plpgsql-control-structures.html#PLPGSQL-ERROR-TRAPPING */

  res = PQexecPrepared(psqldb->conn, insert_stmt,
                       4+value_count, (const char**)paramValues,
                       (int*) &paramLength, (int*) &paramFormat, 0 );

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    logerror("psql:%s: INSERT INTO '%s' failed: %s\n",
        db->name, table->schema->name, PQerrorMessage(psqldb->conn));
    PQclear(res);
    return -1;
  }
  PQclear(res);

  for (i=0;i<4+value_count;i++) {
    xfree(paramValues[i]);
  }

  return 0;
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
 * \see psql_set_key_value, xmalloc, xfree
 */
static char*
psql_get_key_value (Database *database, const char *table, const char *key_column, const char *value_column, const char *key)
{
  if (database == NULL || table == NULL || key_column == NULL ||
      value_column == NULL || key == NULL)
    return NULL;

  PGresult *res;
  PsqlDB *psqldb = (PsqlDB*) database->handle;
  MString *stmt = mstring_create();
  mstring_sprintf (stmt, "SELECT %s FROM %s WHERE %s='%s';",
                   value_column, table, key_column, key);

  res = PQexec (psqldb->conn, mstring_buf (stmt));

  if (PQresultStatus(res) != PGRES_TUPLES_OK) {
    logerror("psql:%s: Error trying to get %s[%s]; (%s).\n",
             database->name, table, key, PQerrorMessage(psqldb->conn));
    goto fail_exit;
  }

  if (PQntuples (res) == 0)
    goto fail_exit;
  if (PQnfields (res) < 1)
    goto fail_exit;

  if (PQntuples (res) > 1)
    logwarn("psql:%s: Key-value lookup for key '%s' in %s(%s, %s) returned more than one possible key.\n",
             database->name, key, table, key_column, value_column);

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
 * \see psql_get_key_value
 */
static int
psql_set_key_value (Database *database, const char *table, const char *key_column, const char *value_column, const char *key, const char *value)
{
  PsqlDB *psqldb = (PsqlDB*) database->handle;
  MString *stmt = mstring_create ();
  char *check_value = psql_get_key_value (database, table, key_column, value_column, key);
  if (check_value == NULL) {
    mstring_sprintf (stmt, "INSERT INTO \"%s\" (\"%s\", \"%s\") VALUES ('%s', '%s');",
                     table, key_column, value_column, key, value);
  } else {
    mstring_sprintf (stmt, "UPDATE \"%s\" SET \"%s\"='%s' WHERE \"%s\"='%s';",
                     table, value_column, value, key_column, key);
  }

  if (sql_stmt (psqldb, mstring_buf (stmt))) {
    logwarn("psql:%s: Key-value update failed for %s='%s' in %s(%s, %s) (database error)\n",
            database->name, key, value, table, key_column, value_column);
    return -1;
  }

  return 0;
}

/** Get data from the metadata table
 * \see db_adapter_get_metadata, sq3_get_key_value
 */
static char*
psql_get_metadata (Database *db, const char *key)
{
  return psql_get_key_value (db, "_experiment_metadata", "key", "value", key);
}

/** Set data in the metadata table
 * \see db_adapter_set_metadata, sq3_set_key_value
 */
static int
psql_set_metadata (Database *db, const char *key, const char *value)
{
  return psql_set_key_value (db, "_experiment_metadata", "key", "value", key, value);
}

/** Add a new sender to the database, returning its index.
 * \see db_add_sender_id
 */
static int
psql_add_sender_id(Database *db, const char *sender_id)
{
  PsqlDB *self = (PsqlDB*)db->handle;
  int index = -1;
  char *id_str = psql_get_sender_id (db, sender_id);

  if (id_str) {
    index = atoi (id_str);
    xfree (id_str);

  } else {
    PGresult *res = PQexec (self->conn, "SELECT MAX(id) FROM _senders;");

    if (PQresultStatus (res) != PGRES_TUPLES_OK) {
      logwarn("psql:%s: Failed to get maximum sender id from database: %s; starting at 0\n",
          db->name, PQerrorMessage (self->conn));
      PQclear (res);
      index = 0;
    } else {
      int rows = PQntuples (res);
      if (rows == 0) {
        logwarn("psql:%s: Failed to get maximum sender id from database: empty result; starting at 0\n",
            db->name);
        PQclear (res);
        index = 0;
      } else {
        index = atoi (PQgetvalue (res, 0, 0)) + 1;
        PQclear (res);
      }
    }

    psql_set_sender_id (db, sender_id, index);

  }

  return index;
}

/** Build a URI for this database.
 *
 * URI is of the form postgresql://USER@SERVER:PORT/DATABASE
 *
 * \see db_adapter_get_uri
 */
static char*
psql_get_uri(Database *db, char *uri, size_t size)
{
  if(snprintf(uri, size, "postgresql://%s@%s:%d/%s", pg_user, pg_host, resolve_service(pg_port, 5432), db->name) >= size) {
    return NULL;
  }
  return uri;
}

/** Get a list of tables in a PostgreSQL database
 * \see db_adapter_get_table_list
 */
static TableDescr*
psql_get_table_list (Database *database, int *num_tables)
{
  PsqlDB *self = database->handle;
  const char *stmt_tablename =
    "SELECT tablename FROM pg_tables WHERE tablename NOT LIKE 'pg%' AND tablename NOT LIKE 'sql%';";
  PGresult *res;
  TableDescr *tables = NULL;
  int rows, cols, i;

  *num_tables = -1;
  res = PQexec (self->conn, stmt_tablename);
  if (PQresultStatus (res) != PGRES_TUPLES_OK) {
    logerror("psql:%s: Couldn't get list of tables: %s\n",
        database->name, PQerrorMessage (self->conn));
    PQclear (res);
    return NULL;
  }
  rows = PQntuples (res);
  cols = PQnfields (res);

  if (cols < 1) {
    return NULL;
  }

  int have_meta = 0;
  for (i = 0; i < rows && !have_meta; i++) {
    if (strcmp (PQgetvalue (res, i, 0), "_experiment_metadata") == 0) {
      have_meta = 1;
    }
  }

  if(!have_meta) {
    logdebug("psql:%s: No metadata found\n", database->name);
  }

  *num_tables = 0;

  for (i = 0; i < rows; i++) {
    char *val = PQgetvalue (res, i, 0);
    logdebug("psql:%s: Found table '%s'\n", database->name, val);
    MString *str = mstring_create ();
    TableDescr *t = NULL;

    if (have_meta) {
      mstring_sprintf (str, "SELECT value FROM _experiment_metadata WHERE key='table_%s';", val);
      PGresult *schema_res = PQexec (self->conn, mstring_buf (str));
      if (PQresultStatus (schema_res) != PGRES_TUPLES_OK) {
        logdebug("psql:%s: Couldn't get schema for table '%s': %s; skipping\n",
            database->name, val, PQerrorMessage (self->conn));
        mstring_delete (str);
        continue;
      }
      int rows = PQntuples (schema_res);
      if (rows == 0) {
        logdebug("psql:%s: Metadata for table '%s' found but empty\n", database->name, val);
        t = table_descr_new (val, NULL); // Don't know the schema for this table
      } else {
        logdebug("psql:%s: Stored schema for table '%s': %s\n", database->name, val, PQgetvalue (schema_res, 0, 0));
        struct schema *schema = schema_from_meta (PQgetvalue (schema_res, 0, 0));
        t = table_descr_new (val, schema);
      }
      PQclear (schema_res);
      mstring_delete (str);
    } else {
      t = table_descr_new (val, NULL);
    }

    if (t) {
      t->next = tables;
      tables = t;
      (*num_tables)++;
    }
  }

  return tables;
}

/** Get the sender_id for a given name in the _senders table.
 *
 * \param name name of the sender
 * \return the sender ID
 *
 * \see psql_get_key_value
 */
static char*
psql_get_sender_id (Database *database, const char *name)
{
  return psql_get_key_value (database, "_senders", "name", "id", name);
}

/** Set the sender_id for a given name in the _senders table.
 *
 * \param name name of the sender
 * \param id the ID to set
 * \return the sender ID
 *
 * \see psql_set_key_value
 */
static int
psql_set_sender_id (Database *database, const char *name, int id)
{
  MString *mstr = mstring_create();
  mstring_sprintf (mstr, "%d", id);
  int ret = psql_set_key_value (database, "_senders", "name", "id", name, mstring_buf (mstr));
  mstring_delete (mstr);
  return ret;
}

/** Prepare an INSERT statement for a given PostgreSQL table
 *
 * The returned value is to be destroyed by the caller.
 *
 * \param table DbTable adapter for the target PostgreSQL table
 * \return an xmalloc'd MString containing the prepared statement, or NULL on error
 *
 * \see mstring_create, mstring_delete
 */
static MString*
psql_make_sql_insert (DbTable* table)
{
  int n = 0;
  int max = table->schema->nfields;

  if (max <= 0) {
    logerror ("psql: Trying to insert 0 values into table '%s'\n", table->schema->name);
    goto fail_exit;
  }

  MString* mstr = mstring_create ();

  if (mstr == NULL) {
    logerror("psql: Failed to create managed string for preparing SQL INSERT statement\n");
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

/** Receive notices from PostgreSQL and post them as an OML log message
 * \param arg application-specific state (in our case, the table name)
 * \param res a PGRES_NONFATAL_ERROR PGresult which can be used with PQresultErrorField and PQresultErrorMessage
 */
static void
psql_receive_notice(void *arg, const PGresult *res)
{
  switch(*PQresultErrorField(res, PG_DIAG_SEVERITY)) {
  case 'E': /*RROR*/
  case 'F': /*ATAL*/
  case 'P': /*ANIC*/
    logerror("psql:%s': %s", (char*)arg, PQresultErrorMessage(res));
    break;
  case 'W': /*ARNING*/
    logwarn("psql:%s': %s", (char*)arg, PQresultErrorMessage(res));
    break;
  case 'N': /*OTICE*/
  case 'I': /*NFO*/
    /* Infos and notice from Postgre are not the primary purpose of OML.
     * We only display them as debug messages. */
  case 'L': /*OG*/
  case 'D': /*EBUG*/
    logdebug("psql:%s': %s", (char*)arg, PQresultErrorMessage(res));
    break;
  default:
    logwarn("'psql:%s': Unknown notice: %s", (char*)arg, PQresultErrorMessage(res));
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
