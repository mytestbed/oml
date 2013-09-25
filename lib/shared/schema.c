/** Copyright 2007-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of the MIT
 * license (License).  You should find a copy of the License in COPYING or at
 * http://opensource.org/licenses/MIT. By downloading or using this software
 * you accept the terms and the liability disclaimer in the License.
 */
/** \file schema.c \brief Manipulate schema structures, and convert to/from
 * string or SQL representation.
 *
 * \page omsp
 * \section omspschema OMSP Schema Specification
 *
 * Schemas describe the name, type and order of the values defining a sample in
 * a measurement stream.
 *
 * Schema declarations are a space-delimited concatenation sequence of
 * name/type pairs. The name and type in each pair are separated by a colon
 * `:`.
 *
 * Valid types in OMSP the following.
 * - `int32` (V>=1)
 * - `uint32` (V>=2)
 * - `int64` (V>=2)
 * - `uint64` (V>=2)
 * - `double` (V>=2)
 * - `string` (V>=1)
 * - `blob` (V>=3)
 * - `guid` (V>=4)
 * - `bool` (V>=4)
 * - `guid` (V>=4)
 *
 * Additionally, some deprecated values are kept for backwards compatibility,
 * and interpreted in the latest version as indicated. They should not be used
 * in new implementations.
 * - `int` (V<2, mapped to `int32` in V>=3)
 * - `integer` (V<2, mapped to `int32` in V>=3)
 * - `long` (V<2, clamped and mapped to `int32` in V>=3)
 * - `float` (V<2, mapped to `double` in V>=3)
 *
 * A full schema also has a name, prepended to its definition and separated by
 * a space. This must consist of only alpha-numeric characters and underscores
 * and must start with a letter or an underscore, i.e., matching
 * `/[_A-Za-z][_A-Za-z0-9]/`. The same rule applies to the names of the
 * elements of the schema. Each schema is also associated with a numeric MS
 * identifier, which is used to link it to all associated measurement tuples
 * later sent. In <a href="https://en.wikipedia.org/wiki/Augmented_Backus%E2%80%93Naur_Form">ABNF</a>,
 * a schema is defined as follows.
 *
 *     schema = ms-id ws schema-name ws field-definition 0*63(ws field-definition)
 *
 *     ms-id = integer
 *     schema-name = 1*letter-or-decimal-or-underscore
 *     field-definition = field-name ":" oml-type
 *
 *     field-name = 1*letter-or-decimal-or-underscore
 *     oml-type = current-oml-type / deprecated-oml-type
 *
 *     current-oml-type = "int32" / "uint32" / "int64" / "uint64" / "double" / "string" / "blob" / "guid" / "bool" / "guid"
 *     deprecated-oml-type = "int" / "integer" / "long" / "float"
 *
 *     integer = 1*decimal
 *     letter-or-decimal-or-underscore = letter / decimal / "_"
 *
 *     decimal = "0"-"9"
 *     letter = "a"-"z" / "A"-"Z"
 *     ws = " "
 *
 * Each client should number its measurement streams sequentially starting from
 * 1 (not 0), and prepend that number to their schema definition. It will later
 * be used to label tuples following this schema, and allow to group them
 * together in the storage backend.
 *
 * \subsection omspschemaexample Example
 *
 *    1 generator_sin label:string phase:double value:double
 *    2 generator_lin label:string counter:long
 *
 * \subsection schema0 Schema 0 (OMSP V>=4)
 *
 * Schema 0 is a specific hard-coded stream for metadata. Its core elements are
 * two fields, named `key` and `value`. Data from this stream is stored in the
 * same way as any other data, but its semantic is different in that it only
 * describes and adds information about other measurement streams. Metadata
 * follows an Subject-Key-Value model where the key/value pair is an attribute
 * of a specific subject. Subjects are expressed in dotted notation. The
 * default subject, `.`, is the experiment itself. At the second level are
 * schemas, and their fields at the third level (e.g., `.a` refers to all of
 * schema `a`, while `.a.f` refers only to its field `f`).
 *
 * To support this, schema 0 is therefore:
 *
 *     0 _experiment_metadata subject:string key:string value:string
 *
 * On the \ref oml2-server "server side", everything gets stored in the
 * `_experiment_metadata` table. However, additional processing might happen.
 * For example, if key `schema` is defined for subject `.` (the experiment
 * root), a new schema is defined at the collection point so new MSs can be
 * sent.
 *
 * In case of reconnection, it is up to the client MUST re-send the \ref
 * headers headers, as well as all schema0 metadata with key `schema` (see \ref
* api). Other metadata MAY be restransmitted as well. The server MAY store
* duplicate metadata if this happens.
 */
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mstring.h"
#include "oml_value.h"
#include "schema.h"
#include "oml_util.h"

/** Parse a schema field like '<name>:<type>' into a struct schema_field.
 *
 * \param meta nil-terminated string to parse
 * \param len length to parse in the string
 * \param schema_field client-provided memory to store the parsed field
 * \return 0 on success, -1 on failure.
 */
int
schema_field_from_meta (const char *meta, size_t len, struct schema_field *field)
{
  char *type = NULL;
  const char *p = meta, *q = (char*)find_charn (p, ':', len);
  if (!q) {
    return -1;
  }
  field->name = oml_strndup (p, q++ - p);
  type = oml_strndup (q, len - (q - p));
  if (!field->name || !type) {
    goto exit;
  }
  field->type = oml_type_from_s (type);
  // OML_LONG_VALUE is deprecated, and converted to INT32 internally in server.
  field->type = (field->type == OML_LONG_VALUE) ? OML_INT32_VALUE : field->type;
  if (field->type == OML_UNKNOWN_VALUE) {
    goto exit;
  }
  oml_free (type);
  return 0;
 exit:
  if (!field->name) {
    oml_free (field->name);
    field->name = NULL;
  }
  if (!type) { oml_free (type); }
  return -1;
}

/** Parse a schema metadata line from client headers.
 *
 * Schema meta lines look like:
 *  schema: <n> <name> <field_name1>:<field_type1> <field_name2>:<field_type2> ...
 * We get everything from the first colon ':' onwards in "meta" parameter.
 *
 * \param meta nil-terminated string to parse
 * \return a pointer to an oml_malloc'd struct schema, to be freed by the caller, or NULL on error (e.g., meta is NULL)
 *
 * \see schema_field_from_meta
 */
struct schema*
schema_from_meta (const char *meta)
{
  if (!meta) return NULL;

  struct schema *schema = NULL;
  struct schema_field *f = NULL;
  size_t fields_size = 0;
  const char *p = meta;
  char *q;

  schema = oml_malloc (sizeof (struct schema));
  if (!schema) { goto exit; }
  schema->nfields = 0;
  schema->fields = NULL;

  /* Parse the schema index */
  schema->index = strtol (p, &q, 0);
  if (p == q) {
    return NULL; /* no digits found */
  }
  p = q;

  /* Get the name of the schema */
  p = (char*)skip_white (p);
  q = (char*)find_white (p);
  schema->name = oml_strndup (p, (q - p));
  if (!schema->name) { goto exit; }
  p = q;

  /* Parse the field definitions */
  while (q && *q) {
    p = (char*)skip_white (p);
    q = (char*)find_white (p);
    if (p != q) {
      schema->nfields++;
      fields_size += sizeof (struct schema_field);
      /* We need this intermediary to be able to free the previous block on failure */
      if (!(f = oml_realloc (schema->fields, fields_size))) {
        goto exit;
      }
      schema->fields = f;
      if (schema_field_from_meta (p, (q - p), &(schema->fields[schema->nfields-1])) == -1) {
        goto exit;
      }
      p = q;
    }
  }
  return schema;
 exit:
  if (schema) {
    schema_free(schema);
  }
  return NULL;
}

/** Create a string representation of a schema
 *
 * \param schema a schema structure to trancribe
 * \return a pointer to an oml_malloc'd string containing the schema; the caller should free the memory
 */
char *
schema_to_meta (const struct schema *schema)
{
  int i;
  char *s = NULL;
  if (!schema) return NULL;
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

  s = oml_strndup (mstring_buf (str), mstring_len (str));
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
 * \param t2o function pointer to the database adapter's SQL to OML type converter function
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
schema_field_from_sql (const char *sql, size_t len, struct schema_field *field, typemap t2o)
{
  char *p = (char*)sql, *q, *up, *uq;
  q = (char*)find_white (p);
  up = *p == '"' ? p+1 : p;
  uq = *(q-1) == '"' ? q-1 : q;
  field->name = oml_strndup (up, uq - up);
  q = (char*)skip_white (q);
  char *type = oml_strndup (q, len - (q - p));
  if (!field->name || !type)
    goto exit;
  field->type = t2o (type);
  // OML_LONG_VALUE is deprecated, and converted to INT32 internally in server.
  field->type = (field->type == OML_LONG_VALUE) ? OML_INT32_VALUE : field->type;
  if (field->type == OML_UNKNOWN_VALUE)
    goto exit;
  oml_free (type);
  return 0;
 exit:
  if (!field->name) oml_free (field->name);
  if (!type) oml_free (type);
  return -1;
}

/* Convert a schema into an SQL table creation statement.
 *
 * \param schema schema structure as created by, e.g., schema_from_meta()
 * \param typemap function pointer to the database adapter's SQL to OML type converter function
 * \return an oml_malloc'd MString containing the table-creation statement; the caller is responsible of freeing it
 * \see oml_malloc, oml_free
 */
MString*
schema_to_sql (const struct schema* schema, reverse_typemap o2t)
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
  n += mstring_sprintf (mstr, "\" (oml_tuple_id %s, ", o2t (OML_DB_PRIMARY_KEY));
  n += mstring_sprintf (mstr, "oml_sender_id %s, ", o2t (OML_INT32_VALUE));
  n += mstring_sprintf (mstr, "oml_seq %s, ", o2t (OML_INT32_VALUE));
  n += mstring_sprintf (mstr, "oml_ts_client %s, ", o2t (OML_DOUBLE_VALUE));
  n += mstring_sprintf (mstr, "oml_ts_server %s", o2t (OML_DOUBLE_VALUE));

  int i = 0;
  while (max > 0) {
    OmlValueT type = schema->fields[i].type;
    char *name = schema->fields[i].name;
    const char* t = o2t (type);
    if (!t) {
      logerror("Unknown type in column '%s'\n", name);
      goto fail_exit;
    }
    n += mstring_sprintf (mstr, ", \"%s\" %s", name, t);
    i++; max--;
  }
  n += mstring_cat (mstr, ");");

  if (n != 0) {
    /* mstring_* return -1 on error, with no error, the sum in n should be 0 */
    goto fail_exit;
  }
  return mstr;

 fail_exit:
  if (mstr) mstring_delete (mstr);
  return NULL;
}

/*
 *  Check to see if "field" represents a metadata column.  If so,
 *  check the type matches the expected type.  If the type mismatches,
 *  return -1.  If the type matches, return 0.  If the field is not a
 *  metadata field, return 1.
 */
static int
schema_check_metadata (struct schema_field *field)
{
  struct schema_field metadata [] =
    {
      { "oml_tuple_id", OML_DB_PRIMARY_KEY },
      { "oml_sender_id", OML_INT32_VALUE },
      { "oml_seq", OML_INT32_VALUE },
      { "oml_ts_client", OML_DOUBLE_VALUE },
      { "oml_ts_server", OML_DOUBLE_VALUE },
    };
  size_t i;
  for (i = 0; i < LENGTH (metadata); i++) {
    if (!strcmp (metadata[i].name, field->name)) {
      if (metadata[i].type != field->type) {
        const char *expected = oml_type_to_s (metadata[i].type);
        const char *received = oml_type_to_s (field->type);
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
 * \param t2o function pointer to the database adapter's SQL to OML type converter function
 * \return 0 on success, -1 on failure.
 *
 */
struct schema*
schema_from_sql (const char *sql, OmlValueT (*t2o) (const char *s))
{
  const char * const command = "CREATE TABLE ";
  int command_len = strlen (command);
  /* Check that it's a CREATE TABLE statement */
  if (strncmp (sql, command, command_len)) return NULL;

  struct schema *schema = NULL;
  struct schema_field *fields = NULL;
  int nfields = 0;
  size_t fields_size = 0;
  char *name;
  char *p = (char *)sql + command_len, *up;
  char *q, *uq;

  p = (char*)skip_white (p);
  q = (char*)find_white (p);
  up = *p == '"' ? p+1 : p;
  uq = *(q-1) == '"' ? q-1 : q;
  name = oml_strndup (up, uq - up);
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
      struct schema_field *f = oml_realloc (fields, fields_size);
      if (!f)
        goto exit;
      fields = f;
      if (schema_field_from_sql (p, (q - p), &fields[nfields], t2o) == -1)
        goto exit;
      int n = schema_check_metadata (&fields[nfields]);
      if (n == -1)
        goto exit;
      /* n == 0 means metadata column (must be skipped here) */
      if (n == 0) {
        oml_free (fields[nfields].name);
        fields[nfields].name = NULL;
      }
      nfields += n;
      p = ++q;
    }
  }

  schema = oml_malloc (sizeof (struct schema));
  if (!schema) goto exit;

  schema->name = name;
  schema->index = -1;
  schema->fields = fields;
  schema->nfields = nfields;
  return schema;

 exit:
  if (name) oml_free (name);
  if (fields) {
    int i = 0;
    for (; i < nfields; i++)
      if (fields[i].name)
        oml_free (fields[i].name);
    oml_free (fields);
  }
  oml_free (schema);
  return NULL;
}

/** Create a new schema structure
 *
 * \param name name for that schema
 */
struct schema *
schema_new (const char *name)
{
  struct schema *schema = oml_malloc (sizeof (struct schema));
  if (!schema) { goto exit; }
  memset(schema, 0, sizeof(struct schema));

  schema->name = oml_strndup (name, strlen (name));
  if (!schema->name) { goto exit; }
  schema->index = -1;
  return schema;

 exit:
  schema_free(schema);
  return NULL;
}

/** Free an allocated schema structures
 * \param schema schema structure to free
 */
void
schema_free (struct schema *schema)
{
  if (schema) {
    if (schema->name) {
      oml_free (schema->name);
    }
    if (schema->fields) {
      int i;
      for (i = 0; i < schema->nfields; i++) {
        if (schema->fields[i].name) {
          oml_free (schema->fields[i].name);
        }
      }
      oml_free (schema->fields);
    }
    oml_free (schema);
  }
}


/** Add a field to an existing schema.
 * \param schema schema to add a field to
 * \param name name of that field
 * \param type OmlValueT representing the type of that field
 * \return 0 un success, -1 otherwise
 */
int
schema_add_field (struct schema *schema, const char *name, OmlValueT type)
{
  if (!schema || !name) return -1;
  size_t fields_size = (schema->nfields + 1) * sizeof (struct schema_field);
  struct schema_field *new = oml_realloc (schema->fields, fields_size);
  if (!new) return -1;
  schema->fields = new;
  schema->fields[schema->nfields].name = oml_strndup (name, strlen (name));
  schema->fields[schema->nfields].type = type;
  if (!schema->fields[schema->nfields].name) return -1;
  schema->nfields += 1;
  return 0;
}

/** Deep copy of an existing schema structure
 *
 * \param schema schema structur to copy
 * \return a new oml_malloc'd copy (to be cleaned by the caller), or NULL
 */
struct schema*
schema_copy (const struct schema *schema)
{
  struct schema *new;
  int i;
  if (!schema) { return NULL; }

  new = oml_malloc (sizeof (struct schema));
  if (!new) { return NULL; }

  new->name = oml_strndup (schema->name, strlen (schema->name));
  new->index = schema->index;
  new->nfields = schema->nfields;
  new->fields = oml_calloc (new->nfields, sizeof (struct schema_field));
  if (!new->name || !new->fields) {
    goto exit;
  }

  for (i = 0; i < new->nfields; i++) {
    new->fields[i].name = oml_strndup (schema->fields[i].name, strlen (schema->fields[i].name));
    if (!new->fields[i].name) {
      goto exit;
    }
    new->fields[i].type = schema->fields[i].type;
  }
  return new;
 exit:
  /* new has been successfully allocated if we reach here */
  if (new->name) { oml_free (new->name); }
  if (new->fields) {
    for (i = 0; i < new->nfields; i++) {
      if (new->fields[i].name) {
        oml_free (new->fields[i].name);
      }
    }
    oml_free (new->fields);
  }
  oml_free (new);
  return NULL;
}

/** Compare two schemas.
 *
 * Schema are equal if they have the
 * same names and identical field names/numbers/types. They may have a
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
 *
 * \param s1 first schema structure
 * \param s2 second schema structure
 * \return 0 on equality, -1 on error, or a positive index+1 of the first mismatching field
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
  } else {
    return -1;
  }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
