/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
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

#ifndef SCHEMA_H__
#define SCHEMA_H__

#include "oml2/omlc.h"
#include "mstring.h"

/** Represent one field in a schema */
struct schema_field
{
  char *name;     /** Name of the field */
  OmlValueT type; /** Type of the field */
};

/** Represent the fields of a schema */
struct schema
{
  char *name;                   /** Name of the schema */
  struct schema_field *fields;  /** Array of struct schema_field */
  int nfields;                  /** Number of elements in nfields */
  int index;                    /** Schema index as set by the sender */
};

/* These types should match those in server/database.h */
/** \see db_adapter_type_to_oml */
typedef OmlValueT (*typemap)(const char *type);
/** \see db_adapter_oml_to_type */
typedef const char* (*reverse_typemap)(OmlValueT type);

struct schema* schema_from_meta (const char *meta);
char *schema_to_meta (const struct schema *schema);
struct schema* schema_from_sql (const char *sql, typemap t2o);
MString* schema_to_sql (const struct schema* schema, reverse_typemap o2t);
struct schema *schema_new (const char *name);
void schema_free (struct schema *schema);
int schema_add_field (struct schema *schema, const char *name, OmlValueT type);
struct schema* schema_copy (const struct schema *schema);
int schema_diff (struct schema *s1, struct schema *s2);


#endif /* SCHEMA_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
