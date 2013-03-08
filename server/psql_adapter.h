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
#ifndef PSQL_ADAPTER_H_
#define PSQL_ADAPTER_H_

#include <libpq-fe.h>
#include "database.h"

#define DEFAULT_PG_HOST "localhost"
#define DEFAULT_PG_PORT "postgresql"
#define DEFAULT_PG_USER "oml"
#define DEFAULT_PG_PASS ""
#define DEFAULT_PG_CONNINFO ""

typedef struct PsqlDB {
  PGconn *conn;
  int sender_cnt;
  time_t last_commit;
} PsqlDB;

typedef struct PsqlTable {
  MString *insert_stmt; /* Named statement for inserting into this table */
} PsqlTable;

int psql_backend_setup ();
int psql_create_database (Database* db);

#endif /*PSQL_ADAPTER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
