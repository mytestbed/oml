/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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
#include <stdlib.h>
#include <string.h>

#include <mem.h>
#include "table_descr.h"

TableDescr*
table_descr_new (const char* name, struct schema* schema)
{
  if (name == NULL)
    return NULL;

  /* schema == NULL means metadata table */

  char *new_name = xstrndup (name, strlen (name));

  TableDescr* t = xmalloc (sizeof(TableDescr));
  if (t == NULL) {
    xfree (new_name);
    return NULL;
  }
  t->name = new_name;
  t->schema = schema;
  t->next = NULL;

  return t;
}

int
table_descr_have_table (TableDescr* tables, const char* table_name)
{
  while (tables)
    {
      if (strcmp (tables->name, table_name) == 0)
        return 1;
      tables = tables->next;
    }
  return 0;
}

void
table_descr_array_free (TableDescr* tables, int n)
{
  int i = 0;
  for (i = 0; i < n; i++)
    {
      xfree (tables[i].name);
      if (tables[i].schema)
        schema_free (tables[i].schema);
    }

  xfree(tables);
}

void
table_descr_list_free (TableDescr* tables)
{
  TableDescr* t = tables;

  while (t)
    {
      TableDescr* next = t->next;
      xfree (t->name);
      if (t->schema)
        schema_free (t->schema);
      xfree (t);
      t = next;
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
