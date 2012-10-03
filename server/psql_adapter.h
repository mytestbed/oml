#ifndef PSQL_ADAPTER_H_
#define PSQL_ADAPTER_H_

#include "database.h"

#define DEFAULT_PG_HOST "localhost"
#define DEFAULT_PG_PORT "postgresql"
#define DEFAULT_PG_USER "oml"
#define DEFAULT_PG_PASS ""
#define DEFAULT_PG_CONNINFO ""

int psql_backend_setup ();

int
psql_create_database(
  Database* db
);

void
psql_release(
  Database* db
);

int
psql_create_table(
  Database* database,
  DbTable* table
);

#endif /*PSQL_ADAPTER_H_*/
