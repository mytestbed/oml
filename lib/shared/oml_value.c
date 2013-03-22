/*
 * Copyright 2010-2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_value.c
 * \brief Support functions for manipulating OmlValue objects
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "mem.h"
#include "guid.h"
#include "oml_util.h"
#include "oml_value.h"
#include "base64.h"
#include "string_utils.h"

static char *oml_value_ut_to_s(OmlValueU* value, OmlValueT type, char *buf, size_t size);
static int oml_value_ut_from_s (OmlValueU *value, OmlValueT type, const char *value_s);

/** String representation of OML types.
 * \see oml_type_from_s, oml_type_to_s
 */
static struct {
  OmlValueT type;
  const char * const name;
} type_names[] = {
  { OML_LONG_VALUE,    "long"    },
  { OML_INT32_VALUE,   "int32"   },
  { OML_UINT32_VALUE,  "uint32"  },
  { OML_INT64_VALUE,   "int64"   },
  { OML_UINT64_VALUE,  "uint64"  },
  { OML_DOUBLE_VALUE,  "double"  },
  { OML_STRING_VALUE,  "string"  },
  { OML_BLOB_VALUE,    "blob"    },
  { OML_GUID_VALUE,    "guid"    },
  { OML_BOOL_VALUE,    "bool"    },
};

/** Initialise one OmlValues.
 *
 * It is *MANDATORY* to use this function before any OmlValue setting,
 * otherwise, uninitialised memory might lead to various issues with dynamic
 * allocation.
 *
 * \param v pointer to the OmlValue to manipulate
 * \see oml_value_array_init
 */
void
oml_value_init(OmlValue *v)
{
  oml_value_array_init(v, 1);
}
/** Initialise arrays of OmlValues.
 *
 * It is *MANDATORY* to use this function before any OmlValue setting,
 * otherwise, uninitialised memory might lead to various issues with dynamic
 * allocation.
 *
 * \param v pointer to the first OmlValue of the array to manipulate
 * \param n length of the array
 * \see oml_value_init
 */
void
oml_value_array_init(OmlValue *v, unsigned int n)
{
  memset(v, 0, n*sizeof(OmlValue));
}

/** Set the type of an OmlValue, cleaning up if need be.
 *
 * \param v pointer to the OmlValue to manipulate
 * \param type new type to set
 * \see oml_value_init, oml_value_reset
 */
void
oml_value_set_type(OmlValue* v, OmlValueT type)
{
  if (v->type != type) {
    oml_value_reset(v);
    v->type = type;
  }
}

/** Assign the content of an OmlValueU of the given OmlValueT to an OmlValue.
 *
 * This function copies value, which is assumed to be of the given type (no
 * check can be made), into the OmlValue pointed to by to.  The to object is
 * set to have the given type. If type is a simple numeric type, the copy
 * simply copies the value.
 *
 * If type is OML_STRING_VALUE, then the string contents are copied into new
 * storage in to.  If to was previously set to be a const string, then the
 * is_const flag is cleared and a new block of memory is allocated to store the
 * copy and its terminating null character. If to did not previously have the
 * is_const flag set, and its string pointer was NULL, then a new block of
 * memory is also allocated as well.  If the string pointer was not NULL, then
 * the string is copied into the previously allocated memory block if it is
 * large enough to fit; otherwise the block is freed and a new one allocated
 * large enough to hold the string (and its terminator).
 *
 * Blobs are handled in a similar fashion.
 *
 * If the source pointer is NULL then an error is returned and a warning
 * message is sent to the log.
 *
 * \param to pointer to OmlValue into which the value should be copied
 * \param value pointer to original OmlValueU to copy into to
 * \param type OmlValueT of value
 * \return 0 if successful, -1 otherwise
 * \see oml_value_init, omlc_copy_string, omlc_copy_blob
 */
int
oml_value_set(OmlValue *to, const OmlValueU *value, OmlValueT type)
{
  oml_value_set_type(to, type);
  if (omlc_is_numeric_type (type) ||
      omlc_is_guid_type (type) ||
      omlc_is_bool_type (type)) {
    to->value = *value;
  } else {
    switch (type) {
    case OML_STRING_VALUE:
      if (!omlc_get_string_ptr(*value)) {
        logwarn("Trying to copy OML_STRING_VALUE from a NULL source\n");
        return -1;
      }
      omlc_copy_string(*oml_value_get_value(to), *value);
      break;

    case OML_BLOB_VALUE:
      if (!omlc_get_blob_ptr(*value)) {
        logwarn("Trying to copy OML_BLOB_VALUE from a NULL source\n");
        return -1;
      }
      omlc_copy_blob(*oml_value_get_value(to), *value);
      break;

  default:
      logerror("%s() for type '%d' not implemented'\n", __FUNCTION__, type);
      return -1;
    }
  }
  return 0;
}

/** DEPRECATED \see oml_value_set */
int
oml_value_copy(OmlValueU *value, OmlValueT type, OmlValue *to)
{
 logwarn("%s() is deprecated, please use oml_value_set(to, value, type) instead\n", __FUNCTION__);
 return oml_value_set(to, value, type);
}

/** Reset one OmlValue, cleaning any allocated memory.
 *
 * The type of the value is also reset (to 0, i.e., OML_DOUBLE_VALUE).
 *
 * \param v pointer to OmlValue to reset
 * \return 0 if successful, -1 otherwise
 * \see oml_value_init, oml_value_array_reset, memset(3)
 */
int
oml_value_reset(OmlValue* v)
{
  switch(v->type) {
  case OML_LONG_VALUE:
    logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
  case OML_INT32_VALUE:
  case OML_UINT32_VALUE:
  case OML_INT64_VALUE:
  case OML_UINT64_VALUE:
  case OML_DOUBLE_VALUE:
  case OML_GUID_VALUE:
  case OML_BOOL_VALUE:
    /* No deep cleanup needed, memset(3) below is sufficient */
    break;

  case OML_STRING_VALUE:
    omlc_reset_string(v->value);
    break;

  case OML_BLOB_VALUE:
    omlc_reset_blob(v->value);
    break;

  default:
    logwarn("%s() for type '%d' not implemented, zeroing storage\n", __FUNCTION__, v->type);
  }
  memset(v, 0, sizeof(OmlValue));
  return 0;
}

/** Reset array of OmlValue, cleaning any allocated memory.
 *
 * \param v pointer to the first OmlValue of the array to manipulate
 * \param n length of the array
 * \see oml_value_init
 */
void
oml_value_array_reset(OmlValue* v, unsigned int n)
{
 int i;
 for (i=0; i<n; i++) {
   oml_value_reset(&v[i]);
 }
}

/** Deep copy an OmlValue.
 *
 * \param dst OmlValue to copy to
 * \param src OmlValue to copy
 * \return 0 on success, -1 otherwise
 * \see oml_value_init, oml_value_set, oml_value_get_value, oml_value_get_type
 */
int
oml_value_duplicate(OmlValue* dst, OmlValue* src)
{
  return oml_value_set(dst, oml_value_get_value(src), oml_value_get_type(src));
}

/** Get a string representing the given OmlValueT type.
 *
 * \param type OmlValueT type to get the representation for
 * \return a string that represent the type, or "UNKNOWN"
 *
 * \see oml_value_ut_from_s
 * \see type_names
 */
const char*
oml_type_to_s (OmlValueT type)
{
  int i = 0;
  int n = LENGTH(type_names);
  static const char unknown[] = "unknown OML type";
  const char *name = unknown;

  for (i = 0; i < n; i++) {
    if (type_names[i].type == type) {
      name = (const char *)type_names[i].name;
      break;
    }
  }

  return name;
}


/** Convert string to an OmlValueT type.
 *
 * \param s string representing the type to get
 * \return the OmlValueT corresponding to the string, or OML_UNKNOWN_VALUE
 *
 * \see oml_value_ut_to_s
 * \see type_names
 */
OmlValueT
oml_type_from_s (const char *s)
{
  int i = 0;
  int n = LENGTH(type_names);
  OmlValueT type = OML_UNKNOWN_VALUE;

  if (NULL == s) {
    logwarn("%s(): trying to resolve a NULL type string\n", __FUNCTION__);

  } else {
    for (i = 0; i < n; i++) {
      if (strcmp (s, type_names[i].name) == 0) {
        type = type_names[i].type;
        break;
      }
    }

    if (type == OML_LONG_VALUE) {
      logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
    }
  }

  return type;
}

/** Convert data stored in an OmlValue to a string representation.
 *
 * The given buffer will contain a nul-terminated C string.
 *
 * \param value pointer to OmlValue containing data
 * \param buf buffer to put the textual representation of the value in
 * \param size size of the buffer
 * \return a pointer to buf, or NULL on error
 * \see oml_value_ut_to_s
 */
char*
oml_value_to_s (OmlValue *value, char *buf, size_t size)
{
  OmlValueU* v = oml_value_get_value(value);
  OmlValueT type = oml_value_get_type(value);

  return oml_value_ut_to_s(v, type, buf, size);
}

/** Convert data stored in an OmlValueU of the given OmlValueT to a string representation.
 *
 * The given buffer will contain a nul-terminated C string.
 *
 * \param value pointer to OmlValue containing data
 * \param buf buffer to put the textual representation of the value in
 * \param size size of the buffer
 * \return a pointer to buf, or NULL on error
 */
static char*
oml_value_ut_to_s(OmlValueU* value, OmlValueT type, char *buf, size_t size)
{
  int i, n = 0;

  switch (type) {
  case OML_LONG_VALUE:
    logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
    n += snprintf(buf, size, "%ld", omlc_get_long(*value));
    break;
  case OML_INT32_VALUE:  n += snprintf(buf, size, "%" PRId32, omlc_get_int32(*value));  break;
  case OML_UINT32_VALUE: n += snprintf(buf, size, "%" PRIu32, omlc_get_uint32(*value)); break;
  case OML_INT64_VALUE:  n += snprintf(buf, size, "%" PRId64, omlc_get_int64(*value));  break;
  case OML_UINT64_VALUE: n += snprintf(buf, size, "%" PRIu64, omlc_get_uint64(*value)); break;
  case OML_DOUBLE_VALUE: n += snprintf(buf, size, "%f", omlc_get_double(*value)); break;
  case OML_STRING_VALUE: n += snprintf(buf, size, "%s", omlc_get_string_ptr(*value)); break;
  case OML_BLOB_VALUE:
    strncpy (buf, "0x", size);
    for (n = 2, i = 0; n < size && i < omlc_get_blob_length(*value); i++) /* n = 2 because of the 0x prefix */
      n += sprintf(buf + n, "%02x", *((uint8_t*)(omlc_get_blob_ptr(*value) + i)));
    if (n == size && i < omlc_get_blob_length(*value)) /* if there was more data than we could write */
      strncpy(buf+size-4, "...", 4);
    break;
  case OML_GUID_VALUE:
    n += omlc_guid_to_string(omlc_get_guid(*value), buf);
    break;
  case OML_BOOL_VALUE:
    n += snprintf(buf, size, "%s", (omlc_get_bool(*value)==OMLC_BOOL_FALSE)?"false":"true");
    break;
  default:
    logerror("%s() for type '%d' not implemented'\n", __FUNCTION__, type);
    return NULL;
  }

  /* It is not guaranteed that snprintf() nul-terminates... */
  if (n >= size)
    /* snprintf() returns the full length of what would have been written if there was room;
     * if larger than size, there wasn't, and we don't want to overshoot */
    buf[size] = '\0';
  else
    buf[n] = '\0';

  return buf;
}

/** Try to convert a string to the current type of OmlValue, and store it there.
 *
 * \param value pointer to output OmlValue
 * \param value_s input string
 * \return  0 on success, -1 otherwise
 * \see oml_value_init, oml_value_ut_from_s
 */
int
oml_value_from_s (OmlValue *value, const char *value_s)
{
  return oml_value_ut_from_s(oml_value_get_value(value), oml_value_get_type(value), value_s);
}

/** Try to convert a string to the type represented by the given string, and store it an OmlValue.
 *
 * \param value pointer to output OmlValue
 * \param type_s string representing an OmlValueT
 * \param value_s input string
 * \return  0 on success, -1 otherwise
 * \see oml_value_init, oml_value_ut_from_s, oml_type_from_s
 */
int
oml_value_from_typed_s (OmlValue *value, const char *type_s, const char *value_s)
{
  OmlValueT type = oml_type_from_s (type_s);
  oml_value_set_type(value, type);
  return oml_value_ut_from_s(oml_value_get_value(value), type, value_s);
}

/** Try to convert a string to the given OmlValueT and store it an OmlValueU.
 *
 * Storage for value should have already been cleared (e.g., with
 * oml_value_set_type(), oml_value_reset() omlc_reset_string() or
 * omlc_reset_blob() if appropriate).
 *
 * Assumes the destination OmlValueU has been properly reset.
 *
 * \param value pointer to output OmlValue
 * \param type type of data to get from the string
 * \param value_s input string
 * \return  0 on success, -1 otherwise (e.g., conversion error)
 * \see oml_value_from_s, oml_value_from_typed_s
 * \see oml_value_set_type, oml_value_reset, omlc_reset_string, omlc_reset_blob
 */
static int
oml_value_ut_from_s (OmlValueU *value, OmlValueT type, const char *value_s)
{
  char *s;
  ssize_t n;
  size_t s_sz, blob_sz;
  uint8_t *blob;
  oml_guid_t c;

  switch (type) {
  case OML_LONG_VALUE:
    logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
    omlc_set_long (*value, strtol (value_s, NULL, 0));
    break;

  case OML_INT32_VALUE:   omlc_set_int32 (*value, strtol (value_s, NULL, 0)); break;
  case OML_UINT32_VALUE:  omlc_set_uint32 (*value, strtoul (value_s, NULL, 0)); break;
  case OML_INT64_VALUE:   omlc_set_int64 (*value, strtoll (value_s, NULL, 0)); break;
  case OML_UINT64_VALUE:  omlc_set_uint64 (*value, strtoull (value_s, NULL, 0)); break;
  case OML_DOUBLE_VALUE:  omlc_set_double (*value, strtod (value_s, NULL)); break;
  case OML_STRING_VALUE:
    s = xmalloc(strlen(value_s)+1);
    n = backslash_decode(value_s, s);
    omlc_reset_string(*value);
    omlc_set_string(*value, s);
    omlc_set_string_size(*value,n+1);
    break;

  case OML_BLOB_VALUE:
    omlc_reset_blob(*value);
    s_sz = base64_validate_string(value_s);
    if(s_sz != -1) {
      blob_sz = base64_size_blob(s_sz);
      blob = xmalloc(blob_sz);
      base64_decode_string(s_sz, value_s, blob_sz, blob);
      omlc_set_blob_ptr(*value, blob);
      omlc_set_blob_length(*value, blob_sz);
      omlc_set_blob_size(*value, blob_sz);
    }
    break;

  case OML_GUID_VALUE:
    omlc_string_to_guid(value_s, &c);
    omlc_set_guid(*value, c);
    break;

  case OML_BOOL_VALUE:
    omlc_set_bool(*value, oml_value_string_to_bool(value_s));
    break;

  default:
    logerror("%s() for type '%d' not implemented to convert '%s'\n", __FUNCTION__, type, value_s);
    return -1;
  }

  if (errno == ERANGE) {
    logerror("%s(): underflow or overlow converting value from string '%s'\n", __FUNCTION__, value_s);
    return -1;
  }

  return 0;
}

/** Cast the data contained in an OmlValue to a double.
 *
 * \param value pointer to OmlValue containing the data to cast
 * \return a double, which defaults to 0. on error
 */
double
oml_value_to_double (OmlValue *value)
{
  OmlValueU *v = oml_value_get_value(value);
  OmlValueT type = oml_value_get_type(value);
  switch (type) {
  case OML_LONG_VALUE:
    logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
    return (double) omlc_get_long(*v);
  case OML_INT32_VALUE:  return (double) omlc_get_int32(*v);
  case OML_UINT32_VALUE: return (double) omlc_get_uint32(*v);
  case OML_INT64_VALUE:  return (double) omlc_get_int64(*v);
  case OML_UINT64_VALUE: return (double) omlc_get_uint64(*v);
  case OML_DOUBLE_VALUE: return (double) omlc_get_double(*v);
  default:
    logerror("%s() for type '%d' not implemented'\n", __FUNCTION__, type);
    return 0.0;
  }
}

/** Cast the data contained in an OmlValue to an int.
 *
 * \param value pointer to OmlValue containing the data to cast
 * \return an integer, which defaults to 0 on error
 */
int
oml_value_to_int (OmlValue *value)
{
  OmlValueU *v = oml_value_get_value(value);
  OmlValueT type = oml_value_get_type(value);
  switch (type) {
  case OML_LONG_VALUE:
    logwarn("%s(): OML_LONG_VALUE is deprecated, please use OML_INT32_VALUE instead\n", __FUNCTION__);
    return (int) omlc_get_long(*v);
  case OML_INT32_VALUE:  return (int) omlc_get_int32(*v);
  case OML_UINT32_VALUE: return (int) omlc_get_uint32(*v);
  case OML_INT64_VALUE:  return (int) omlc_get_int64(*v);
  case OML_UINT64_VALUE: return (int) omlc_get_uint64(*v);
  case OML_DOUBLE_VALUE: return (int) omlc_get_double(*v);
  default:
    logerror("%s() for type '%d' not implemented'\n", __FUNCTION__, type);
  return 0;
  }
}

/** Try to read a string as a boolean
 *
 * Anything is True except if clearly False (which includes a NULL string).
 *
 * Partial matches work too: 'f', 'Fa', 'fAl' or 'FAlS' are equivalents to
 * 'false'; longer strings starting with a variation of 'false' are true.
 *
 * \param value_s string to read as a boolean
 * \return a bool representation suitable for omlc_set_bool
 * \see omlc_set_bool
 */
uint8_t
oml_value_string_to_bool(const char* value_s)
{
  if (NULL != value_s) {
    return (uint8_t)(strncasecmp(value_s, "false", strlen(value_s)) != 0);
  }

  logwarn("%s: trying to convert NULL string to bool, assuming false\n", __FUNCTION__);
  return (uint8_t)0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
