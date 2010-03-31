/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
#include "table_descr.h"

TableDescr*
table_descr_new (const char* name, const char* schema)
{
  if (name == NULL || schema == NULL)
	return NULL;

  size_t len = strlen (name) + 1;
  char* new_name = (char*) malloc (len * sizeof (char));
  if (new_name == NULL)
	{
	  return NULL;
	}
  strncpy (new_name, name, len);

  len = strlen (schema) + 1;
  char* new_schema = (char*) malloc (len * sizeof (char));
  if (new_schema == NULL)
	{
	  free (new_name);
	  return NULL;
	}
  strncpy (new_schema, schema, len);

  TableDescr* t = (TableDescr*) malloc (sizeof(TableDescr));
  if (t == NULL)
	{
	  free (new_name);
	  free (new_schema);
	  return NULL;

	}
  memset (t, 0, sizeof (TableDescr));
  t->name = new_name;
  t->schema = new_schema;
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
	  free (tables[i].name);
	  free (tables[i].schema);
	}

  free(tables);
}

void
table_descr_list_free (TableDescr* tables)
{
  TableDescr* t = tables;

  while (t)
	{
	  TableDescr* next = t->next;
	  free (t->name);
	  free (t->schema);
	  free (t);
	  t = next;
	}
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
