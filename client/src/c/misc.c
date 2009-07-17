/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
 * \fn int mp_lock(OmlMP* mp)
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
 * \fn void mp_unlock(OmlMP* mp)
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
 * \fn int oml_value_copy(OmlValueU* value, OmlValueT  type, OmlValue*  to)
 * \brief copy the value +value+ inside +to+
 * \param value the unique value
 * \param type the type of +value+
 * \param to the receiving enum of the value
 * \return 0 if successful, -1 otherwise
 */
int
oml_value_copy(
  OmlValueU* value,
  OmlValueT  type,
  OmlValue*  to
) {
  switch (type) {
    case OML_LONG_VALUE:
      to->value.longValue = value->longValue;
      to->type = OML_LONG_VALUE;
      break;
    case OML_DOUBLE_VALUE:
      to->value.doubleValue = value->doubleValue;
      to->type = OML_DOUBLE_VALUE;
      break;
    case OML_STRING_VALUE: {
      to->type = OML_STRING_VALUE;
      OmlString* str = &to->value.stringValue;
      str->is_const = value->stringValue.is_const;
      if (str->is_const) {
	str->ptr = value->stringValue.ptr;
      } else {
	char* fstr = value->stringValue.ptr;
	int size = strlen(fstr);
	if (str->length < size + 1) {
	  if (str->length > 0) {
	    free(str->ptr);
	  }
	  str->ptr = malloc(size + 1);
	  str->length = size + 1;
	}
	strncpy(str->ptr, fstr, size);
	str->size = size;
      }
      break;
    }
    default:
      o_log(O_LOG_ERROR, "Copy for type '%d' not implemented'\n", type);
      return -1;
  }
  return 0;
}
/**
 * \fn int oml_value_reset(OmlValue* v)
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
 * \fn char* oml_type_to_s(OmlValueT type)
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
