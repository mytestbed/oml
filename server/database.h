/*
 * Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file database.h
 * \brief Interface that storage backend need to implement to be usable by the server.
 */
#ifndef DATABASE_H_
#define DATABASE_H_

#include "oml2/omlc.h"
#include "table_descr.h"
#include "schema.h"

#define DEFAULT_DB_BACKEND "sqlite"

#define MAX_DB_NAME_SIZE 64
#define MAX_TABLE_NAME_SIZE 64
#define MAX_COL_NAME_SIZE 64

/**
 * Limit on the number of times the server tries to generate a table name for
 * a different schema.
 *
 * XXX: If this is changed, make sure enough memory is allocated in
 * database_find_or_create_table() to hold the new table name in the schema
 * structure.
 */
#define MAX_TABLE_RENAME 10

struct Database;
struct DbTable;
typedef struct DbTable DbTable;
typedef struct Database Database;

/** Mapping from native to OML types.
 *
 * \param type string describing the SQLite3 type
 * \return the corresponding OmlValueT or OML_UNKNOWN_VALUE if unknown
 */
typedef OmlValueT (*db_adapter_type_to_oml)(const char *type);

/** Mapping from OML to native types.
 *
 * \param type string describing the SQLite3 type
 * \return the corresponding OmlValueT or OML_UNKNOWN_VALUE if unknown
 */
typedef const char* (*db_adapter_oml_to_type)(OmlValueT type);

/** Execute an SQL statement with no data return.
 *
 * This function executes a statement with the assumption that the
 * result can be ignored; that is, it's not useful for SELECT
 * statements.
 *
 * \param self the database handle.
 * \param stmt the SQL statement to execute.
 * \return 0 if successful, -1 if the database reports an error.
 */
typedef int (*db_adapter_stmt)(Database *db, const char* stmt);

/** Create a database adapter structures
 *
 * \param db basic Database structure to associate with the database
 * \return 0 if successful, -1 otherwise
 */
typedef int (*db_adapter_create)(Database *db);

/** Release the database.
 *
 * This is called when the last active client leaves.
 *
 * \param db Database to release
 */
typedef void (*db_adapter_release)(Database *db);

/** Create the adapter data structures required to represent a database table.
 *
 * \param db Database to create the table adapter
 * \param table DbTable description of the table to create
 * \param shallow if 0, the table should be actively CREATEd in the database backend
 * \return 0 on success, -1 on failure
 */
typedef int (*db_adapter_table_create)(Database *db, DbTable *table, int shallow);

/** Create a metadata table 
 *
 * \param db pointer to Database
 * \param name name of the database table to create
 * \return 0 on success, -1 otherwise
 */
typedef int (*db_adapter_table_create_meta)(Database *db, const char *name);

/** Free an SQLite3 table
 *
 * \param database Database in which the table is
 * \param table DbTable structure to free
 * \return 0 on success, <0 otherwise
 */
typedef int (*db_adapter_table_free)(Database *db, DbTable *table);

/** Insert value in the a table of a database
 *
 * \param db Database to write in
 * \param table DbTable to insert data in
 * \param sender_id sender ID
 * \param seq_no sequence number
 * \param time_stamp timestamp of the receiving data
 * \param values OmlValue array to insert
 * \param value_count number of values
 * \return 0 if successful, -1 otherwise
 */
typedef int (*db_adapter_insert)(Database *db, DbTable* table, int sender_id, int seq_no, double time_stamp, OmlValue* values, int value_count);

/** Get data from the metadata table
 *
 * The returned string should be xfree'd by the caller when no longer needed.
 * 
 * \param key key to lookup
 * \return an xmalloc'd string containing the current value for that key
 *
 * \see xmalloc, xfree
 */
typedef char* (*db_adapter_get_metadata) (Database* db, const char* key);

/** Set data in the metadata table
 * 
 * \param key key to lookup
 * \param value a string to set for that key
 * \return 0 on success, -1 otherwise
 */
typedef int   (*db_adapter_set_metadata) (Database* db, const char* key, const char* value);

/** Get a URI to this database
 *
 * \param db Database object
 * \param[out] uri output string buffer
 * \param size length of uri
 * \return uri on success, NULL otherwise (e.g., uri buffer too small)
 */
typedef char* (*db_adapter_get_uri) (Database* db, char* uri, size_t size);

/** Add a new sender to the database, returning its index.
 *
 * If a sender with the given id already exists, its pre-existing
 * index should be returned. Otherwise, a new sender is added to the table
 * with a new sender id, unique to this experiment.
 *
 * \param db the Database to which the sender id is being added.
 * \param sender_id the sender ID
 * \return the index of the new sender
 */
typedef int (*db_add_sender_id)(Database* db, const char* sender_id);

/** Get a list of tables
 *
 * The caller should xfree the returned values when no longer needed.
 *
 * \param db Database to list tables of
 * \param[out] num_tables pointer to an integer where the number of tables will be written
 * \return a linked list of TableDescr of length *num_tables
 *
 * \see table_descr_new, table_descr_list_free
 */
typedef TableDescr* (*db_adapter_get_table_list) (Database* db, int *num_tables);

/** One measurement table in a Database */
struct DbTable {
  struct schema*  schema; /** Schema for that table */
  void*           handle; /** Opaque pointer to database implementation handle */
  struct DbTable* next;   /** Pointer to the next table in the linked list */
};

/** An open and active database, with manipulations functions from its backend */
struct Database{
  char       name[MAX_DB_NAME_SIZE];  /** Name of this database */
  char       *backend_name;           /** Name of the backend for this database */

  int        ref_count;               /** Number of active clients */
  DbTable*   first_table;             /** Pointer to the first data table */
  time_t     start_time;              /** Experiment start time */
  void*      handle;                  /** Opaque pointer to database implementation handle */

  db_adapter_oml_to_type o2t;                     /** Pointer to OML-to-native type conversion function */
  db_adapter_type_to_oml t2o;                     /** Pointer to native-to-OML type conversion function */
  db_adapter_stmt stmt;                           /** Pointer to low-level function to execute a given SQL statement \see db_adapter_stmt */
  db_adapter_create create;                       /** Pointer to function to create a new database \see db_adapter_create */
  db_adapter_release release;                     /** Pointer to function to release from one client \see db_adapter_release */
  db_adapter_table_create table_create;           /** Pointer to function to create a table \see db_adapter_table_create*/
  db_adapter_table_create_meta table_create_meta; /** Pointer to function to create a metadata table \see db_adapter_table_create_meta */
  db_adapter_table_free table_free;               /** Pointer to function to free a table \see db_adapter_table_free */
  db_adapter_insert  insert;                      /** Pointer to function to insert data in a table \see db_adapter_insert */
  db_adapter_get_metadata get_metadata;           /** Pointer to function to get data from the metadata table \see db_adapter_get_metadata */
  db_adapter_set_metadata set_metadata;           /** Pointer to function to set data in the metadata table \see db_adapter_set_metadata*/
  db_adapter_get_uri get_uri;                     /** Pointer to function to get a URI to this database \see db_adapter_get_uri */
  db_add_sender_id   add_sender_id;               /** Pointer to function to add a new sender to the _senders table \see db_add_sender_id */
  db_adapter_get_table_list get_table_list;       /** Pointer to function to get a list of tables \see db_adapter_get_table_list */

  struct Database* next;                          /** Pointer to the next database in the linked list */
};


int database_setup_backend (const char* backend);
db_adapter_create database_create_function (const char *backend);
Database *database_find(const char* name);
int database_init (Database *self);
void database_release(Database* database);
void database_cleanup();

DbTable *database_find_table(Database* database, const char* name);
DbTable *database_find_or_create_table(Database *database, struct schema *schema);
DbTable *database_create_table (Database *database, struct schema *schema);
void     database_table_free(Database *database, DbTable* table);

#endif /*DATABASE_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
