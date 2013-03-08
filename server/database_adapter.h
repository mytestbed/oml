/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file database_adapter.h
 * \brief Generic functions prototypes for database adapters.
 */
#ifndef DATABASE_ADAPTER_H_
#define DATABASE_ADAPTER_H_

int dba_table_create_from_meta (Database *db, const char *schema);
int dba_table_create_from_schema (Database *db, const struct schema *schema);

int dba_table_create_meta (Database *db, const char *name);

int dba_begin_transaction (Database *db);
int dba_end_transaction (Database *db);
int dba_reopen_transaction (Database *db);

#endif /* DATABASE_ADAPTER_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
