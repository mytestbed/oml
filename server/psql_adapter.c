#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <postgresql/libpq-fe.h>
#include <ocomm/o_log.h>
#include <time.h>
#include <sys/time.h>
#include "database.h"

#define CONN_BUFFER 512

typedef struct _psqlDB {
  PGconn *conn;
  int sender_cnt;
} PsqlDB;

typedef struct _pqlTable {
  char insert_stmt[512];  // prepared insert statement
  char update_stmt[512];  // prepared update statement
} PsqlTable;

static void
exit_nicely(PGconn *conn)
{
  PQfinish(conn);
  exit(1);
}

static int
sql_stmt(PsqlDB* self, const char* stmt);

static int
psql_insert(
  Database* db, DbTable*  table, 
  int sender_id, int seq_no, double ts,
  OmlValue* values, int value_count
);
static int
psql_add_sender_id(Database* db, char* sender_id);

//static sqlite3* db;
static int first_row;
/**
 * \fn int psql_create_database(Database* db)
 * \brief Create a sqlite3 database
 * \param db the databse to associate with the sqlite3 database
 * \return 0 if successful, -1 otherwise
 */
int
psql_create_database(
  Database* db
) {

  char conninfo[CONN_BUFFER];
  PGconn     *conn;
  PGresult   *res;

  /* Setup parameters to connect to the database */
  /* Assumes we use a $HOME/.pgpass file:
     http://www.postgresql.org/docs/8.2/interactive/libpq-pgpass.html*/
  sprintf(conninfo,"host=%s user=%s dbname=%s",db->hostname,db->user,db->name);

  o_log(O_LOG_DEBUG, "Connection info: %s",conninfo);

  /* Make a connection to the database */
  conn = PQconnectdb(conninfo);

  /* Check to see that the backend connection was successfully made */
  if (PQstatus(conn) != CONNECTION_OK) {
    o_log(O_LOG_ERROR, "Connection to database failed: %s",
	  PQerrorMessage(conn));
    exit_nicely(conn);
    return -1;
  }

  PsqlDB* self = (PsqlDB*)malloc(sizeof(PsqlDB));
  memset(self, 0, sizeof(PsqlDB));

  self->conn = conn;

  db->insert = psql_insert;
  db->add_sender_id = psql_add_sender_id;
  db->adapter_hdl = self;
  
  return 0;
}
/**
 * \fn void psql_release(Database* db)
 * \brief Release the psql database
 * \param db the database that contains the psql database
 */
void
psql_release(
  Database* db
) {
  PsqlDB* self = (PsqlDB*)db->adapter_hdl;
  PQfinish(self->conn);

  // TODO: Release table specific data
  
  free(self);
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
psql_add_sender_id(
  Database* db,
  char*     sender_id
) {
  PsqlDB* self = (PsqlDB*)db->adapter_hdl;
  int index = ++self->sender_cnt;
  
  //char s[512];  // don't check for length
  // TDEBUG
  //sprintf(s, "INSERT INTO _senders VALUES (%d, '%s');\0", index, sender_id); 
  //if (sql_stmt(self, s)) return -1;
  //return index;
  return atoi(sender_id);
}
/**
 * \fn int psql_create_table(Database* db, DbTable* table)
 * \brief Create a sqlite3 table
 * \param db the database that contains the sqlite3 db
 * \param table the table to associate in sqlite3 database
 * \return 0 if successful, -1 otherwise
 */
int
psql_create_table(
  Database* db,
  DbTable* table
) {
  if (table->columns == NULL) {
    o_log(O_LOG_WARN, "No cols defined for table '%s'\n", table->name);
    return -1;
  }

  char *errmsg;
  PGresult   *res;

  int max = table->col_size;
  
//  BEGIN TRANSACTION;
//       CREATE TABLE t1 (t1key INTEGER
//                    PRIMARY KEY,data TEXT,num double,timeEnter DATE);
//       INSERT INTO "t1" VALUES(1, 'This is sample data', 3, NULL);
//       INSERT INTO "t1" VALUES(2, 'More sample data', 6, NULL);
//       INSERT INTO "t1" VALUES(3, 'And a little more', 9, NULL);
//       COMMIT;
//       
      
  char create[512];  // don't check for length
  char index[512];  // don't check for length
  char index_name[512];
  char insert[512];  // don't check for length
  char update[512];  // don't check for length
  char update_key[512];  // don't check for length
  
  char* cs = create;
  sprintf(cs, "CREATE TABLE %s (oml_sender_id INT4, oml_seq INT4, oml_ts_client FLOAT8, oml_ts_server FLOAT8", table->name);
  cs += strlen(cs);

  char* id = index;

  sprintf(index_name, "idx_%s",table->name);
  sprintf(id, "CREATE UNIQUE INDEX %s ON %s(", index_name,table->name);
  /* sprintf(id, "CREATE INDEX %s ON %s(", index_name,table->name); */
  id += strlen(id);

  char* is = insert;
  sprintf(is, "INSERT INTO %s VALUES ($1, $2, $3, $4", table->name);
  is += strlen(is);

  /* rmz: this needs to be updated with a psql function or dealt with directly in here */
  /* Example: UPDATE bowl_trace_libtrace SET delta = 2 , hist = 'toto'
     where mac_src = '00:0e:0c:af:cb:48' and mac_dst =
     '00:1b:21:05:e7:92'; */
  char* up = update;
  sprintf(up, "UPDATE %s SET oml_sender_id = $1, oml_seq = $2, oml_ts_client = $3, oml_ts_server = $4", table->name);
  up += strlen(up);
  char* upk = update_key;
  sprintf(upk, " WHERE ");
  upk += strlen(upk);

  int first = 0;
  DbColumn* col = table->columns[0];
  int i = 0;
  /* ruben: Index of fields who will be indexes, hard coded for now */
  int idx[4] = {0,1,2,3};
  int idx_len = 4;
  int idx_i = 0;
  while (col != NULL && max > 0) {
    char* t;
    col = table->columns[i];
    switch (col->type) {
      case OML_LONG_VALUE: t = "INT4"; break;
      case OML_DOUBLE_VALUE: t = "FLOAT8"; break;
      case OML_STRING_VALUE: t = "TEXT"; break;
      default:
        o_log(O_LOG_ERROR, "Bug: Unknown type %d in col '%s'\n", col->type, col->name);
    }
    if (first) {
      sprintf(cs, "%s %s", col->name, t);
      sprintf(is, "$%d",i+5);
      first = 0;
    } else { 
      sprintf(cs, ", %s %s", col->name, t);
      sprintf(is, ", $%d",i+5);  
    }
    /* Checking for indexes */
    if (idx_i < idx_len && idx[idx_i] == i) {
      /* This column will be an index */
      if (idx_i == 0) {
	sprintf(id, "%s", col->name);
	sprintf(upk," %s = $%d", col->name,i+5);
      } else {
	sprintf(id, ", %s", col->name);
	sprintf(upk," and %s = $%d", col->name,i+5);
      }
      id += strlen(id);
      upk += strlen(upk);
      idx_i++;
    } else {
      /* No index */
      sprintf(up, ", %s = $%d",col->name, i+5);
      up += strlen(up);
    }
    cs += strlen(cs);
    is += strlen(is);
    i++;
    max--;
  }
  sprintf(cs, ");");
  sprintf(id, ");");  
  sprintf(is, ");");
  sprintf(up, " %s;",update_key);

  o_log(O_LOG_INFO, "Number of parameters = %d\n", i+4);
  o_log(O_LOG_INFO, "schema: %s\n", create);
  o_log(O_LOG_INFO, "index: %s\n", index);
  o_log(O_LOG_INFO, "insert: %s\n", insert);
  o_log(O_LOG_INFO, "update: %s\n", update);
  
  /* Get a hook to the database */
  PsqlDB* psqldb = (PsqlDB*)db->adapter_hdl;

  /* Create the table */
  res = PQexec(psqldb->conn,create);
  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    o_log(O_LOG_ERROR, "Could not create table: %s [%s].\n", create, PQerrorMessage(psqldb->conn));
    PQclear(res);
    /* exit_nicely(psqldb->conn); /\* Not sure this is a good idea *\/ */
/*     return -1; */
  } else {
    /* The table was created, let's apply the index */
    res = PQexec(psqldb->conn,index);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      o_log(O_LOG_ERROR, "Could not create index: %s [%s].\n", index, PQerrorMessage(psqldb->conn));
      PQclear(res);
      /* exit_nicely(psqldb->conn); /\* Not sure this is a good idea *\/ */
      /*     return -1; */
    }
  }
  /* Original code */
  /* if (sql_stmt(psqldb, create)) return -1; */
  /* if (sql_stmt(psqldb, index)) return -1; */

  /* Prepare the insert statement and update statement  */
  PsqlTable* psqltable = (PsqlTable*)malloc(sizeof(PsqlTable));
  memset(psqltable, 0, sizeof(PsqlTable));
  table->adapter_hdl = psqltable;

  strcpy(psqltable->insert_stmt,"InsertOMLResults");
  strcat(psqltable->insert_stmt,"-");
  strcat(psqltable->insert_stmt,table->name);
  res = PQprepare(psqldb->conn,psqltable->insert_stmt,insert,i+4,NULL);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    o_log(O_LOG_ERROR, "Could not prepare statement (%s).\n", PQerrorMessage(psqldb->conn));
    PQclear(res);
    exit_nicely(psqldb->conn); /* Not sure this is a good idea */
    return -1;
  } else {
    o_log(O_LOG_DEBUG, "Prepare statement (%s) ok.\n",psqltable->insert_stmt);
  }
  PQclear(res);

  strcpy(psqltable->update_stmt,"UpdateOMLResults");
  strcat(psqltable->update_stmt,"-");
  strcat(psqltable->update_stmt,table->name);
  res = PQprepare(psqldb->conn,psqltable->update_stmt,update,i+4,NULL);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    o_log(O_LOG_ERROR, "Could not prepare statement (%s).\n", PQerrorMessage(psqldb->conn));
    PQclear(res);
    exit_nicely(psqldb->conn); /* Not sure this is a good idea */
    return -1;
  } else {
    o_log(O_LOG_DEBUG, "Prepare statement (%s) ok.\n",psqltable->update_stmt);
  }
  PQclear(res);

/*   if (sqlite3_prepare_v2(sq3db->db_hdl, insert, -1, &sq3table->insert_stmt, 0) != SQLITE_OK) { */

/*     return -1; */
/*   } */
  return 0;
}

/**
 * \fn static int psql_insert(Database* db, DbTable*  table, int sender_id, int seq_no, double time_stamp, OmlValue* values, int value_count)
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
psql_insert(
  Database* db,
  DbTable*  table,
  int       sender_id,
  int       seq_no,
  double    time_stamp,
  OmlValue* values,
  int       value_count
) {
  PsqlDB* psqldb = (PsqlDB*)db->adapter_hdl;
  PsqlTable* psqltable = (PsqlTable*)table->adapter_hdl;
  PGresult* res;
  int i;
  double time_stamp_server;
  char* insert_stmt = psqltable->insert_stmt;

  char * paramValues[4+value_count];
  for (i=0;i<4+value_count;i++) {
    paramValues[i] = malloc(512*sizeof(char));
  }

  int paramLength[4+value_count];
  int paramFormat[4+value_count];

  //o_log(O_LOG_DEBUG, "insert");
  o_log(O_LOG_DEBUG, "TDEBUG - into psql_insert - %d \n", seq_no);

  sprintf(paramValues[0],"%i",sender_id);
  paramLength[0] = 0;
  paramFormat[0] = 0;

  sprintf(paramValues[1],"%i",seq_no);
  paramLength[1] = 0;
  paramFormat[1] = 0;

  sprintf(paramValues[2],"%.8f",time_stamp);
  paramLength[2] = 0;
  paramFormat[2] = 0;

  /* if (sqlite3_bind_int(stmt, 1, sender_id) != SQLITE_OK) { */
/*     o_log(O_LOG_ERROR, "Could not bind 'oml_sender_id' (%s).\n",  */
/*         sqlite3_errmsg(sq3db->db_hdl)); */
/*   } */
/*   if (sqlite3_bind_int(stmt, 2, seq_no) != SQLITE_OK) { */
/*     o_log(O_LOG_ERROR, "Could not bind 'oml_seq' (%s).\n",  */
/*         sqlite3_errmsg(sq3db->db_hdl)); */
/*   } */
/*   if (sqlite3_bind_double(stmt, 3, time_stamp) != SQLITE_OK) { */
/*     o_log(O_LOG_ERROR, "Could not bind 'oml_ts_client' (%s).\n",  */
/*         sqlite3_errmsg(sq3db->db_hdl)); */
/*   } */
  struct timeval tv;
  gettimeofday(&tv, NULL);
  time_stamp_server = tv.tv_sec - db->start_time + 0.000001 * tv.tv_usec;

  sprintf(paramValues[3],"%.8f",time_stamp_server);
  paramLength[3] = 0;
  paramFormat[3] = 0;

/*   if (sqlite3_bind_double(stmt, 4, time_stamp_server) != SQLITE_OK) { */
/*     o_log(O_LOG_ERROR, "Could not bind 'oml_ts_server' (%s).\n",  */
/*         sqlite3_errmsg(sq3db->db_hdl)); */
/*   } */
  
  OmlValue* v = values;
  for (i = 0; i < value_count; i++, v++) {
    DbColumn* col = table->columns[i]; 
    if (v->type != col->type) {
      o_log(O_LOG_ERROR, "Mismatch in %dth value type fo rtable '%s'\n", i, table->name);
      return -1;
    }
    switch (col->type) {
      case OML_LONG_VALUE:
	sprintf(paramValues[4+i],"%i",(int)v->value.longValue);
	paramLength[4+i] = 0;
	paramFormat[4+i] = 0;
        /* res = sqlite3_bind_int(stmt, i + 5, (int)v->value.longValue); */
        break;
      case OML_DOUBLE_VALUE:
	sprintf(paramValues[4+i],"%.8f",v->value.doubleValue);
	paramLength[4+i] = 0;
	paramFormat[4+i] = 0;
        /* res = sqlite3_bind_double(stmt, i + 5, v->value.doubleValue); */
        break;
      case OML_STRING_VALUE:
	sprintf(paramValues[4+i],"%s",v->value.stringValue.ptr);
	paramLength[4+i] = 0;
	paramFormat[4+i] = 0;
/*         res = sqlite3_bind_text (stmt, i + 5, v->value.stringValue.ptr, */
/*                 -1, SQLITE_TRANSIENT); */
        break;
      default:
        o_log(O_LOG_ERROR, "Bug: Unknown type %d in col '%s'\n", col->type, col->name);
        return -1;
    }
/*     if (res != SQLITE_OK) { */
/*       o_log(O_LOG_ERROR, "Could not bind column '%s' (%s).\n",  */
/*           col->name, sqlite3_errmsg(sq3db->db_hdl));  */
/*     } */
  }
  o_log(O_LOG_DEBUG, "TDEBUG - into psql_insert - %d \n", seq_no);

  /* Use stuff from http://www.postgresql.org/docs/current/static/plpgsql-control-structures.html#PLPGSQL-ERROR-TRAPPING */

  res = PQexecPrepared(psqldb->conn,psqltable->update_stmt, 4+value_count, 
		       (const char**)paramValues, (int *) &paramLength, (int *) &paramFormat, 0 ); 

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    o_log(O_LOG_ERROR, "Exec of prepared UPDATE failed: %s\n", PQerrorMessage(psqldb->conn));
    PQclear(res);
    exit_nicely(psqldb->conn);
    return -1;
  } else if (!strcmp(PQcmdTuples(res),"0")) {
    /* No result found, try an insert instead */
    o_log(O_LOG_DEBUG, "UPDATE status: %s (%s rows affected)\n", PQcmdStatus(res),PQcmdTuples(res));
    o_log(O_LOG_DEBUG, "Try INSERT\n");
    /* Clean up the old one */
    PQclear(res);

    res = PQexecPrepared(psqldb->conn,psqltable->insert_stmt, 4+value_count, 
			 (const char**)paramValues, (int *) &paramLength, (int *) &paramFormat, 0 ); 

    /* This one might failed if there is a concurrent key insert,
       might have to check for another exception in the future. See
       http://www.postgresql.org/docs/current/static/plpgsql-control-structures.html#PLPGSQL-ERROR-TRAPPING */
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
      fprintf(stderr, "Exec of prepared INSERT failed: %s\n", PQerrorMessage(psqldb->conn));
      PQclear(res);
      exit_nicely(psqldb->conn);
      return -1;
    }
    PQclear(res);
  } else {
    PQclear(res);
  }

  for (i=0;i<4+value_count;i++) {
    free(paramValues[i]);
  }

/*   if (sqlite3_step(stmt) != SQLITE_DONE) { */
/*     o_log(O_LOG_ERROR, "Could not step (execute) stmt.\n"); */
/*     return -1; */
/*   } */
  /* return sqlite3_reset(stmt); */
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
 * \fn static int select_stmt(Sq3DB* self, const char* stmt)
 * \brief Prespare sqlite statement
 * \param self the sqlite3 database
 * \param stmt the statement to prepare
 * \return 0 if successfull, -1 otherwise
 */
/* This one does not seem to be used anymore */
/* static int  */
/* select_stmt( */
/* 	    PsqlDB* self, */
/*   const char* stmt */
/* ) { */
/*   char* errmsg; */
/*   int   ret; */
/*   int   nrecs = 0; */
/*   o_log(O_LOG_DEBUG, "prepare to exec 1 \n"); */
/*   first_row = 1; */

/*   ret = sqlite3_exec(self->db_hdl, stmt, select_callback, &nrecs, &errmsg); */

/*   if (ret != SQLITE_OK) { */
/*     o_log(O_LOG_ERROR, "Error in select statement %s [%s].\n", stmt, errmsg); */
/*     return -1; */
/*   } */
/*   return nrecs; */
/* } */
/**
 * \fn static int sql_stmt(Sq3DB* self, const char* stmt)
 * \brief Execute sqlite3 statement
 * \param self the sqlite3 database
 * \param stmt the statement to prepare
 * \return 0 if successfull, -1 otherwise
 */
static int
sql_stmt(
  PsqlDB* self,
  const char* stmt
) {
  char *errmsg;
  PGresult   *res;
  o_log(O_LOG_DEBUG, "prepare to exec %s \n", stmt);
  //ret = sqlite3_exec(self->db_hdl, stmt, 0, 0, &errmsg);
  res = PQexec(self->conn,stmt);

  if (PQresultStatus(res) != PGRES_COMMAND_OK) {
    o_log(O_LOG_ERROR, "Error in statement: %s [%s].\n", stmt, PQerrorMessage(self->conn));
    PQclear(res);
    exit_nicely(self->conn); /* Not sure this is a good idea */
    return -1;
  }
  /*
   * Should PQclear PGresult whenever it is no longer needed to avoid memory
   * leaks
   */
  PQclear(res);

  return 0;
}
