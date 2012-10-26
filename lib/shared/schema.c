/*
 * Copyright 2010-2012 National ICT Australia (NICTA), Australia
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
#include <ctype.h>

#include "log.h"
#include "mem.h"
#include "mstring.h"
#include "oml_value.h"
#include "schema.h"
#include "oml_util.h"

/*
 * Parse a schema field like '<name>:<type>' into a struct schema_field.
 * The storage for the schema_field must be provided by the caller.
 * "len" must give the number of characters in the field; the field does not
 * have to be zero-terminated, but the string pointed to by "meta" should.
 * Returns 0 on success, -1 on failure.
 */
int
schema_field_from_meta (char *meta, size_t len, struct schema_field *field)
{
  char *p = meta;
  char *q = memchr (p, ':', len);
  if (!q)
    return -1;
  field->name = xstrndup (p, q++ - p);
  char *type = xstrndup (q, len - (q - p));
  if (!field->name || !type)
    goto exit;
  field->type = oml_type_from_s (type);
  // OML_LONG_VALUE is deprecated, and converted to INT32 internally in server.
  field->type = (field->type == OML_LONG_VALUE) ? OML_INT32_VALUE : field->type;
  if (field->type == OML_UNKNOWN_VALUE)
    goto exit;
  xfree (type);
  return 0;
 exit:
  if (!field->name) xfree (field->name);
  if (!type) xfree (type);
  return -1;
}

/*
 * Parse a schema metadata line from client headers.  For a valid schema,
 * return a pointer to the schema.  Otherwise, return NULL.
 *
 * Schema meta lines look like:
 *
 *  schema: <n> <name> <field_name1>:<field_type1> <field_name2>:<field_type2> ...
 *
 *  We get everything from the first colon ':' onwards in "meta" parameter.
 */
struct schema*
schema_from_meta (char *meta)
{
  if (!meta) return NULL;

  int index;
  struct schema *schema = NULL;
  struct schema_field *fields = NULL;
  int nfields = -1;
  size_t fields_size = 0;
  char *name;
  char *p = meta, *q;
  index = strtol (p, &q, 0);
  if (p == q)
    return NULL; /* no digits found */
  p = q;

  p = (char*)skip_white (p);
  q = (char*)find_white (p);
  name = xstrndup (p, (q - p));
  p = q;

  while (q && *q) {
    p = (char*)skip_white (p);
    q = (char*)find_white (p);
    if (p != q) {
      nfields++;
      fields_size += sizeof (struct schema_field);
      struct schema_field *f = xrealloc (fields, fields_size);
      if (!f) goto exit;
      else fields = f;
      if (schema_field_from_meta (p, (q - p), &fields[nfields]) == -1)
        goto exit;
      p = q;
    }
  }
  schema = xmalloc (sizeof (struct schema));
  if (!schema) goto exit;

  schema->nfields = nfields + 1;
  schema->index = index;
  schema->fields = fields;
  schema->name = name;
  return schema;
 exit:
  if (name) xfree (name);
  if (fields) {
    int i = 0;
    for (; i < nfields; i++)
      if (fields[i].name)
        xfree (fields[i].name);
    xfree (fields);
  }
  if (schema) xfree (schema);
  return NULL;
}

const char *
schema_to_meta (struct schema *schema)
{
  if (!schema) return NULL;
  int i;
  MString *str = mstring_create ();
  mstring_sprintf (str, "%d %s", schema->index, schema->name);

  if (schema->fields)
    for (i = 0; i < schema->nfields; i++) {
      mstring_sprintf (str, " %s:%s",
                       schema->fields[i].name,
                       oml_type_to_s (schema->fields[i].type));
    }
  else
    goto fail_exit;

  const char *s = xstrndup (mstring_buf (str), mstring_len (str));
  mstring_delete (str);
  return s;

 fail_exit:
  if (str)
    mstring_delete (str);
  return NULL;
}

/** Parse an SQL name/type pair into a schema field object.
 *
 * \param sql pointer to the start of the column specifier can be part of a
 * longer string and does not have to be null terminated.  There must be no
 * leading whitespace.
 * \param len length of the column specifier, from the first character of the
 * name up to but not including the comma (or closing parenthesis) following
 * the type.
 * \param[out] field parse result, must be allocated by the caller.
 * \param reverse_typemap function pointer to the database adapter's SQL to OML type converter function
 * \return 0 on success, -1 on failure.
 *
 * Example input (SQLite3 types):
 *
 *  "rx_packets INTEGER, tx_packets INTEGER, ..."
 *   ^          ^
 *   |          |_type
 *   |            type_len=7
 *   |_ name
 *      name_len=10
 *
 *      len = 10 + 7 + 1 = 18
 *
 *  The first four fields are handled specially, because they
 *  represent OML metadata -- they must always be present.  They are:
 *
 *  oml_sender_id INTEGER
 *  oml_seq       INTEGER
 *  oml_ts_client REAL
 *  oml_ts_server REAL
 *
 */
static int
schema_field_from_sql (char *sql, size_t len, struct schema_field *field, OmlValueT (*reverse_typemap) (const char *s))
{
  char *p = sql, *q, *up, *uq;
  q = (char*)find_white (p);
  up = *p == '"' ? p+1 : p;
  uq = *(q-1) == '"' ? q-1 : q;
  field->name = xstrndup (up, uq - up);
  q = (char*)skip_white (q);
  char *type = xstrndup (q, len - (q - p));
  if (!field->name || !type)
    goto exit;
  field->type = reverse_typemap (type);
  // OML_LONG_VALUE is deprecated, and converted to INT32 internally in server.
  field->type = (field->type == OML_LONG_VALUE) ? OML_INT32_VALUE : field->type;
  if (field->type == OML_UNKNOWN_VALUE)
    goto exit;
  xfree (type);
  return 0;
 exit:
  if (!field->name) xfree (field->name);
  if (!type) xfree (type);
  return -1;
}

/*
 *  Check to see if "field" represents a metadata column.  If so,
 *  check the type matches the expected type.  If the type mismatches,
 *  return -1.  If the type matches, return 0.  If the field is not a
 *  metadata field, return 1.
 */
int
schema_check_metadata (struct schema_field *field)
{
  struct schema_field metadata [] =
    {
      { "oml_sender_id", OML_INT32_VALUE },
      { "oml_seq", OML_INT32_VALUE },
      { "oml_ts_client", OML_DOUBLE_VALUE },
      { "oml_ts_server", OML_DOUBLE_VALUE },
    };
  size_t i;
  for (i = 0; i < LENGTH (metadata); i++) {
    if (!strcmp (metadata[i].name, field->name)) {
      if (metadata[i].type != field->type) {
        char *expected = oml_type_to_s (metadata[i].type);
        char *received = oml_type_to_s (field->type);
        logerror ("Existing table metadata type mismatch (%s expected %s, got %s)\n",
                  metadata[i].name, expected, received);
        return -1;
      } else return 0;
    }
  }
  return 1;
}

/** Parse an SQL input into a schema object.
 *
 * \param sql pointer to the start of the column specifier can be part of a
 * longer string and does not have to be null terminated.  There must be no
 * leading whitespace.
 * \param reverse_typemap function pointer to the database adapter's SQL to OML type converter function
 * \return 0 on success, -1 on failure.
 *
 */
struct schema*
schema_from_sql (char *sql, OmlValueT (*reverse_typemap) (const char *s))
{
  const char * const command = "CREATE TABLE ";
  int command_len = strlen (command);
  /* Check that it's a CREATE TABLE statement */
  if (strncmp (sql, command, command_len)) return NULL;

  struct schema *schema = NULL;
  struct schema_field *fields = NULL;
  int nfields = 0; /* Different to schema_from_meta () */
  size_t fields_size = 0;
  char *name;
  char *p = sql + command_len, *up;
  char *q, *uq;

  p = (char*)skip_white (p);
  q = (char*)find_white (p);
  up = *p == '"' ? p+1 : p;
  uq = *(q-1) == '"' ? q-1 : q;
  name = xstrndup (up, uq - up);
  p = q;
  p = (char*)skip_white (p);

  if (*p++ != '(') /* Opening paren of colspec */
    goto exit;

  q = p;
  while (q && *q && *q != ';') {
    p = (char*)skip_white (p);
    q = memchr (p, ',', strlen (p));
    if (q == NULL) {
      q = memchr (p, ')', strlen (p));
    }
    if (q != NULL) {
      fields_size = (nfields + 1) * sizeof (struct schema_field);
      struct schema_field *f = xrealloc (fields, fields_size);
      if (!f)
        goto exit;
      fields = f;
      if (schema_field_from_sql (p, (q - p), &fields[nfields], reverse_typemap) == -1)
        goto exit;
      int n = schema_check_metadata (&fields[nfields]);
      if (n == -1)
        goto exit;
      /* n == 0 means metadata column (must be skipped here) */
      if (n == 0) {
        xfree (fields[nfields].name);
        fields[nfields].name = NULL;
      }
      nfields += n;
      p = ++q;
    }
  }

  schema = xmalloc (sizeof (struct schema));
  if (!schema) goto exit;

  schema->name = name;
  schema->index = -1;
  schema->fields = fields;
  schema->nfields = nfields;
  return schema;

 exit:
  if (name) xfree (name);
  if (fields) {
    int i = 0;
    for (; i < nfields; i++)
      if (fields[i].name)
        xfree (fields[i].name);
    xfree (fields);
  }
  if (schema) xfree (schema);
  return NULL;
}


struct schema *
schema_new (const char *name)
{
  struct schema *schema = xmalloc (sizeof (struct schema));
  if (!schema) goto exit;

  schema->name = xstrndup (name, strlen (name));
  if (!schema->name) goto exit;
  schema->index = -1;
  return schema;

 exit:
  if (schema) {
    if (schema->name) xfree (schema->name);
    xfree (schema);
  }
  return NULL;
}

void
schema_free (struct schema *schema)
{
  if (schema) {
    if (schema->name)
      xfree (schema->name);
    if (schema->fields) {
      int i;
      for (i = 0; i < schema->nfields; i++)
        xfree (schema->fields[i].name);
      xfree (schema->fields);
    }
    xfree (schema);
  }
}

int
schema_add_field (struct schema *schema, const char *name, OmlValueT type)
{
  if (!schema || !name) return -1;
  size_t fields_size = (schema->nfields + 1) * sizeof (struct schema_field);
  struct schema_field *new = xrealloc (schema->fields, fields_size);
  if (!new) return -1;
  schema->fields = new;
  schema->fields[schema->nfields].name = xstrndup (name, strlen (name));
  schema->fields[schema->nfields].type = type;
  if (!schema->fields[schema->nfields].name) return -1;
  schema->nfields += 1;
  return 0;
}

struct schema*
schema_copy (struct schema *schema)
{
  if (!schema) return NULL;
  struct schema *new = xmalloc (sizeof (struct schema));
  if (!new) return NULL;
  new->name = xstrndup (schema->name, strlen (schema->name));
  new->index = schema->index;
  new->nfields = schema->nfields;
  new->fields = xcalloc (new->nfields, sizeof (struct schema_field));
  if (!new->name || !new->fields)
    goto exit;
  int i;
  for (i = 0; i < new->nfields; i++) {
    new->fields[i].name = xstrndup (schema->fields[i].name, strlen (schema->fields[i].name));
    if (!new->fields[i].name)
      goto exit;
    new->fields[i].type = schema->fields[i].type;
  }
  return new;
 exit:
  if (new->name) xfree (new->name);
  if (new->fields) {
    for (i = 0; i < new->nfields; i++)
      if (new->fields[i].name)
        xfree (new->fields[i].name);
    xfree (new->fields);
  }
  if (new) xfree (new);
  return NULL;
}

/*
 * Check if two schema are different.  Schema are equal if they have the
 * same names and identical field names/numbers/types.  They may have a
 * different index and still be considered equal.
 *
 * If the schema are the same object, or if the schema are identical
 * in a deep compare sense as outlined above, return 0.
 *
 * If either s1 or s2 is NULL, or if their names differ, or if they
 * differ in number of fields, or if either's fields vector is NULL,
 * return -1.
 *
 * If s1 and s2 have differing field names and/or types, return the
 * column number of the field that differs (fields are numbered from 1
 * in this case to distinguish from return value 0, corresponding to
 * "schema are equal".).
 */
int
schema_diff (struct schema *s1, struct schema *s2)
{
  if (s1 == s2) return 0;
  if (!s1 || !s2) return -1;
  if (strcmp (s1->name, s2->name)) return -1;
  if (s1->fields && s2->fields) {
    if (s1->nfields != s2->nfields) return -1;
    int i;
    for (i = 0; i < s1->nfields; i++) {
      if (strcmp (s1->fields[i].name, s2->fields[i].name) ||
          s1->fields[i].type != s2->fields[i].type)
        return i+1;
    }
  } else
    return -1;
  return 0;
}

/* Convert a schema into an SQL table creation statement.
 *
 * \param schema schema structure as created by, e.g., schema_from_meta()
 * \param typemap function pointer to the database adapter's SQL to OML type converter function
 * \return an MString containing the table-creation statement
 */
MString*
schema_to_sql (struct schema* schema, const char *(*typemap) (OmlValueT))
{
  int n = 0;
  int max = schema->nfields;

  if (max <= 0) {
    logerror ("Tried to create SQL CREATE TABLE statement for schema with 0 columns\n");
    return NULL;
  }

  MString* mstr = mstring_create ();
  if (mstr == NULL) {
    logerror("Failed to create managed string for preparing SQL CREATE TABLE statement\n");
    return NULL;
  }

  /* Build SQL "CREATE TABLE" statement */
  n += mstring_set (mstr, "CREATE TABLE \"");
  n += mstring_cat (mstr, schema->name);
  n += mstring_sprintf (mstr, "\" (oml_sender_id %s, ", typemap (OML_INT32_VALUE));
  n += mstring_sprintf (mstr, "oml_seq %s, ", typemap (OML_INT32_VALUE));
  n += mstring_sprintf (mstr, "oml_ts_client %s, ", typemap (OML_DOUBLE_VALUE));
  n += mstring_sprintf (mstr, "oml_ts_server %s", typemap (OML_DOUBLE_VALUE));

  int i = 0;
  while (max > 0) {
    OmlValueT type = schema->fields[i].type;
    char *name = schema->fields[i].name;
    const char* t = typemap (type);
    if (!t) {
      logerror("Unknown type in column '%s'\n", name);
      goto fail_exit;
    }
    n += mstring_sprintf (mstr, ", \"%s\" %s", name, t);
    i++; max--;
  }
  n += mstring_cat (mstr, ");");
  if (n != 0) goto fail_exit;
  return mstr;

 fail_exit:
  if (mstr) mstring_delete (mstr);
  return NULL;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
