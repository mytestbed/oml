/*
 * Copyright 2007-2008 National ICT Australia (NICTA), Australia
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
    case OML_STRING_PTR_VALUE:
      to->value.stringPtrValue= value->stringPtrValue;
      to->type = OML_STRING_PTR_VALUE;
      break;
    case OML_STRING_VALUE:{
      char* str = type == OML_STRING_VALUE ? value->stringValue.text
          : value->stringPtrValue;
      int len = strlen(str);
      if (to->value.stringValue.size < len) {
        if (to->value.stringValue.size > 0) {
          free(to->value.stringValue.text);
        }
        to->value.stringValue.text = malloc(len + 1);
        to->value.stringValue.size = len;
      }
      strncpy(to->value.stringValue.text, str, to->value.stringValue.size);
      to->type = OML_STRING_VALUE;
      break;
    }
    default:
      o_log(O_LOG_ERROR, "Copy for type '%d' not implemented'\n", type);
      return -1;
  }
  return 0;
}

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
  case OML_STRING_PTR_VALUE:
    v->value.stringPtrValue = '\0';
    break;
  case OML_STRING_VALUE: {
    if (v->value.stringValue.size > 0) {
      v->value.stringValue.text[0] = '\0';
    }
    break;
  }
  default:
    o_log(O_LOG_ERROR, "Copy for type '%d' not implemented'\n", v->type);
    return -1;
  }
  return 0;
}

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
  }
  return "UNKNOWN";
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
