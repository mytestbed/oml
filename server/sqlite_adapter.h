/*
 * Copyright 2007-2016 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
#ifndef SQLITE_ADAPTER_H_
#define SQLITE_ADAPTER_H_

#include <sqlite3.h>
#include "database.h"

typedef struct Sq3DB {
  sqlite3*  conn;
  int       sender_cnt;
  time_t    last_commit;
} Sq3DB;

typedef struct Sq3Table {
  sqlite3_stmt* insert_stmt;  // prepared insert statement
} Sq3Table;

int sq3_backend_setup (void);
int sq3_create_database (Database* db);

#endif /*SQLITE_ADAPTER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
