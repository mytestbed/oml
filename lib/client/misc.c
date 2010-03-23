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
/*!\file misc.c
  \brief Implements various common utility functions
*/

#include <stdlib.h>
#include <pthread.h>
#include <ocomm/o_log.h>
#include <errno.h>
#include <string.h>
#include <oml2/omlc.h>

#include "client.h"
/**
 * \brief lock of the measurement point mutex
 * \param mp the measurement point
 * \return 0 if successful, -1 otherwise
 */
int
mp_lock(
  OmlMP* mp
) {
  if (mp->mutexP) {
    if (pthread_mutex_lock(mp->mutexP)) {
      o_log(O_LOG_WARN, "%s: Couldn't get mutex lock (%s)\n",
          mp->name, strerror(errno));
      return -1;
    }
  }
  return 0;
}
/**
 * \brief unlock of the measurement point mutex
 * \param mp the measurement point
 */
void
mp_unlock(
  OmlMP* mp
) {
  if (mp->mutexP) {
    if (pthread_mutex_unlock(mp->mutexP)) {
      o_log(O_LOG_WARN, "%s: Couldn't unlock mutex (%s)\n",
          mp->name, strerror(errno));
    }
  }
}

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
  switch (type)
    {
    case OML_LONG_VALUE:
      to->value.longValue = value->longValue;
      to->type = OML_LONG_VALUE;
      break;
    case OML_DOUBLE_VALUE:
      to->value.doubleValue = value->doubleValue;
      to->type = OML_DOUBLE_VALUE;
      break;
    case OML_STRING_VALUE:
      {
        char* fstr = value->stringValue.ptr;
        if (!fstr)
          {
            o_log (O_LOG_WARN, "Trying to copy OML_STRING_VALUE from a NULL source\n");
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

        strncpy (to->value.stringValue.ptr, fstr, length);
        to->value.stringValue.length = length;
        break;
      }
    default:
      o_log(O_LOG_ERROR, "Copy for type '%d' not implemented'\n", type);
      return -1;
    }
  return 0;
}

/**
 * \brief reset the set of values in +v+
 * \param v the +OmlValue+ to reset
 * \return 0 if successful, -1 otherwise
 */
int
oml_value_reset(
  OmlValue* v
) {
  switch(v->type) {
  case OML_LONG_VALUE:
    v->value.longValue = 0;
    break;
  case OML_DOUBLE_VALUE:
    v->value.doubleValue = 0;
    break;
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
  default:
    o_log(O_LOG_ERROR, "Copy for type '%d' not implemented'\n", v->type);
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
oml_type_to_s(
    OmlValueT type
) {
  switch(type) {
  case OML_LONG_VALUE:
    return "long";
  case OML_DOUBLE_VALUE:
    return "double";
  case OML_STRING_PTR_VALUE:
  case OML_STRING_VALUE:
    return "string";
  default:
    return "UNKNOWN";
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
