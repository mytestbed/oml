#ifndef PSQL_ADAPTER_H_
#define PSQL_ADAPTER_H_

#include "database.h"

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
