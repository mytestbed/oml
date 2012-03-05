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
/*!\file oml_value.c
  \brief Support functions for manipulating OmlValue objects.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <pthread.h>
#include <errno.h>
#include <oml2/omlc.h>
#include "oml_value.h"
#include "log.h"

/**
 * @brief Copy an OmlValueU into an OmlValueT.
 *
 * This function copies +value+, which must be of the given +type+,
 * into the OmlValue pointed to by +to+.  The +to+ object is set to
 * have the given +type+.  If +type+ is a simple numeric type, the
 * copy simply copies the value.
 *
 * If +type+ is OML_STRING_VALUE, then the string contents are copied
 * into new storage in +to+.  If +to+ was previously set to be a
 * "is_const" string, then the "is_const" flag is cleared and a new
 * block of memory is allocated to store the copy, sized to the exact
 * number of bytes required to store the string and its terminating
 * null character.  If +to+ did not previously have the "is_const"
 * flag set, and its string pointer was null, then a new block of
 * memory is also allocated.  If the string pointer was not null, then
 * the string is copied into the previously allocated memory block if
 * it is large enough to fit; otherwize the block is freed and a new
 * one allocated large enough to hold the string (and its terminator).
 *
 * If the source string pointer is NULL then an error is returned and
 * a warning message is sent to the log.
 *
 * @param value the original value
 * @param type the type of +value+
 * @param to OmlValue into which the value should be copied
 * @return 0 if successful, -1 otherwise
 */
int
oml_value_copy(OmlValueU *value, OmlValueT type, OmlValue *to)
{
  if (omlc_is_numeric_type (type))
    {
      to->type = type;
      to->value = *value;
    }
  else
    {
      /*
       * Currently the only non-numeric type is OML_STRING_VALUE, but
       * this will change in future.
       */
      switch (type)
        {
        case OML_STRING_VALUE:
          {
            char* fstr = value->stringValue.ptr;
            if (!fstr)
              {
                logwarn("Trying to copy OML_STRING_VALUE from a NULL source\n");
                return -1;
              }
            int length = strlen(fstr);

            if (to->type == OML_STRING_VALUE)
              {
                if (to->value.stringValue.is_const)
                  {
                    to->value.stringValue.is_const = 0;
                    to->value.stringValue.ptr = NULL;
                  }
                else if (to->value.stringValue.size < length + 1)
                  {
                    if (to->value.stringValue.ptr != NULL)
                      {
                        free (to->value.stringValue.ptr);
                        to->value.stringValue.ptr = NULL;
                      }
                    to->value.stringValue.length = 0;
                    to->value.stringValue.size = 0;
                  }
              }
            else
              {
                to->type = OML_STRING_VALUE;
                to->value.stringValue.ptr = NULL;
                to->value.stringValue.length = 0;
                to->value.stringValue.size = 0;
                to->value.stringValue.is_const = 0;
              }

            /*
             * At this point, if the dest string value's pointer is NULL,
             * we should allocate the right amount of memory for it.  If
             * not, then it should already have enough memory to store the
             * string correctly.
             *
             * This assumes correct accounting of to.value.stringValue.{length,size}
             */
            if (to->value.stringValue.ptr == NULL)
              {
                to->value.stringValue.ptr = (char*)malloc(length + 1);
                memset(to->value.stringValue.ptr, 0, length + 1);
                to->value.stringValue.size = length + 1;
              }

            strncpy (to->value.stringValue.ptr, fstr, length + 1);
            to->value.stringValue.length = length;
            break;
          }
        case OML_BLOB_VALUE: {
          if (value->blobValue.data == NULL || value->blobValue.size == 0) {
            logwarn("Trying to copy OML_BLOB_VALUE from a NULL source\n");
            to->value.blobValue.fill = 0;
            return -1;
          } else {
            if (to->value.blobValue.data == NULL) {
              void *new = malloc (value->blobValue.fill);
              if (new == NULL) {
                logerror ("Failed to allocate memory for new OML_BLOB_VALUE:  %s\n", strerror (errno));
                return -1;
              }
              to->value.blobValue.data = new;
              to->value.blobValue.size = value->blobValue.fill;
            } else if (to->value.blobValue.size < value->blobValue.fill) {
              void * new = realloc (to->value.blobValue.data, value->blobValue.fill);
              if (new == NULL) {
                logerror ("Failed to re-allocate memory for new OML_BLOB_VALUE:  %s\n", strerror (errno));
                return -1;
              }
              to->value.blobValue.data = new;
              to->value.blobValue.size = value->blobValue.fill;
            }
            memcpy (to->value.blobValue.data, value->blobValue.data, value->blobValue.fill);
            to->value.blobValue.fill = value->blobValue.fill;
          }
          break;
        }
        default:
          logerror("XCopy for type '%d' not implemented'\n", type);
          return -1;
        }
    }
  return 0;
}

/**
 * \brief reset the set of values in +v+
 * \param v the +OmlValue+ to reset
 * \return 0 if successful, -1 otherwise
 */
int
oml_value_reset(OmlValue* v)
{
  switch(v->type)
    {
    case OML_LONG_VALUE:   v->value.longValue   = 0;    break;
    case OML_INT32_VALUE:  v->value.int32Value  = 0;    break;
    case OML_UINT32_VALUE: v->value.uint32Value = 0;    break;
    case OML_INT64_VALUE:  v->value.int64Value  = 0LL;  break;
    case OML_UINT64_VALUE: v->value.uint64Value = 0ULL; break;
    case OML_DOUBLE_VALUE: v->value.doubleValue = 0;    break;
    case OML_STRING_VALUE: {
      if (v->value.stringValue.is_const) {
        v->value.stringValue.ptr = NULL;
      } else {
        v->value.stringValue.size = 0;
        if (v->value.stringValue.length > 0) {
          *v->value.stringValue.ptr = '\0';
        }
      }
      break;
    }
    case OML_BLOB_VALUE: {
      OmlBlob *blob = &v->value.blobValue;
      if (blob->data != NULL) {
        bzero (blob->data, blob->size);
      } else {
        blob->size = 0;
      }
      blob->fill = 0;
      break;
    }
    default:
      logerror("Reset for type '%d' not implemented'\n", v->type);
      return -1;
    }
  return 0;
}
/**
 * \brief give the type of the value
 * \param type the type to return
 * \return a string that represent the type +type+
 */
char*
oml_type_to_s (OmlValueT type)
{
  switch(type) {
  case OML_LONG_VALUE:   return "long";
  case OML_INT32_VALUE:  return "int32";
  case OML_UINT32_VALUE: return "uint32";
  case OML_INT64_VALUE:  return "int64";
  case OML_UINT64_VALUE: return "uint64";
  case OML_DOUBLE_VALUE: return "double";
  case OML_STRING_VALUE: return "string";
  case OML_BLOB_VALUE:   return "blob";
  default: return "UNKNOWN";
  }
}

OmlValueT
oml_type_from_s (const char *s)
{
  static struct type_pair
  {
    OmlValueT type;
    const char * const name;
  } type_list [] =
    {
      { OML_LONG_VALUE,   "long"   },
      { OML_INT32_VALUE,  "int32"  },
      { OML_UINT32_VALUE, "uint32" },
      { OML_INT64_VALUE,  "int64"  },
      { OML_UINT64_VALUE, "uint64" },
      { OML_DOUBLE_VALUE, "double" },
      { OML_STRING_VALUE, "string" },
      { OML_BLOB_VALUE, "blob" }
    };
  int i = 0;
  int n = sizeof (type_list) / sizeof (type_list[0]);
  OmlValueT type = OML_UNKNOWN_VALUE;

  for (i = 0; i < n; i++) {
    if (strcmp (s, type_list[i].name) == 0) {
      type = type_list[i].type;
      break;
    }
  }

  return type;
}

void
oml_value_to_s (OmlValueU *value, OmlValueT type, char *buf)
{
  switch (type) {
  case OML_LONG_VALUE: {
    sprintf(buf, "\t%" PRId32, oml_value_clamp_long (value->longValue));
    break;
  }
  case OML_INT32_VALUE:  sprintf(buf, "%" PRId32,  value->int32Value);  break;
  case OML_UINT32_VALUE: sprintf(buf, "%" PRIu32,  value->uint32Value); break;
  case OML_INT64_VALUE:  sprintf(buf, "%" PRId64,  value->int64Value);  break;
  case OML_UINT64_VALUE: sprintf(buf, "%" PRIu64,  value->uint64Value); break;
  case OML_DOUBLE_VALUE: sprintf(buf, "%f",  value->doubleValue); break;
  case OML_STRING_VALUE: sprintf(buf, "%s",  value->stringValue.ptr); break;
  case OML_BLOB_VALUE: {
    const unsigned int max_bytes = 6;
    int bytes = value->blobValue.fill < max_bytes ? value->blobValue.fill : max_bytes;
    int i = 0;
    int n = 5;
    strcpy (buf, "blob ");
    for (i = 0; i < bytes; i++) {
      n += sprintf(buf + n, "%02x", ((uint8_t*)value->blobValue.data)[i]);
    }
    strcat (buf, " ...");
    break;
  }
  default:
    logerror ("Unsupported value type '%d'\n", type);
  }
}

int
oml_value_from_s (OmlValue *value, const char *value_s)
{
  switch (value->type) {
  case OML_STRING_VALUE: {
    /* Make sure we do a deep copy */
    OmlValue v;
    omlc_set_string (v.value, (char*)value_s);
    oml_value_copy (&v.value, value->type, value);
    break;
  }
  case OML_BLOB_VALUE: {
    /* Can't retrieve a blob from a string (yet) */
    value->value.blobValue.fill = 0;
    break;
  }
  case OML_LONG_VALUE: omlc_set_long (value->value, strtol (value_s, NULL, 0)); break;
  case OML_INT32_VALUE: omlc_set_int32 (value->value, strtol (value_s, NULL, 0)); break;
  case OML_UINT32_VALUE: omlc_set_uint32 (value->value, strtoul (value_s, NULL, 0)); break;
  case OML_INT64_VALUE: omlc_set_int64 (value->value, strtoll (value_s, NULL, 0)); break;
  case OML_UINT64_VALUE: omlc_set_uint64 (value->value, strtoull (value_s, NULL, 0)); break;
  case OML_DOUBLE_VALUE: omlc_set_double (value->value, strtod (value_s, NULL)); break;
  default: {
    logerror("Unknown type for value converted from string '%s'.\n", value_s);
    return -1;
  }
  }
  if (errno == ERANGE) {
    logerror("Underflow or overlow converting value from string '%s'\n", value_s);
    return -1;
  }
  return 0;
}

int
oml_value_from_typed_s (OmlValue *value, const char *type_s, const char *value_s)
{
  value->type = oml_type_from_s (type_s);
  return oml_value_from_s (value, value_s);
}


double
oml_value_to_double (OmlValue *value)
{
  OmlValueU *v = &value->value;
  switch (value->type) {
  case OML_LONG_VALUE:   return (double) v->longValue;
  case OML_INT32_VALUE:  return (double) v->int32Value;
  case OML_UINT32_VALUE: return (double) v->uint32Value;
  case OML_INT64_VALUE:  return (double) v->int64Value;
  case OML_UINT64_VALUE: return (double) v->uint64Value;
  case OML_DOUBLE_VALUE: return (double) v->doubleValue;
  default: return 0.0;
  }
}

int
oml_value_to_int (OmlValue *value)
{
  OmlValueU *v = &value->value;
  switch (value->type) {
  case OML_LONG_VALUE:   return (int) v->longValue;
  case OML_INT32_VALUE:  return (int) v->int32Value;
  case OML_UINT32_VALUE: return (int) v->uint32Value;
  case OML_INT64_VALUE:  return (int) v->int64Value;
  case OML_UINT64_VALUE: return (int) v->uint64Value;
  case OML_DOUBLE_VALUE: return (int) v->doubleValue;
  default: return 0;
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
