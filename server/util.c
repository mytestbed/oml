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
#include <ctype.h>
#include <log.h>
#include "util.h"

void
chomp (char* str)
{
  char* p = str + strlen (str);

  while (p != str && isspace (*--p));

  *++p = '\0';
}


OmlValueT sql_to_oml_type (const char *s)
{
  static struct type_pair
  {
    OmlValueT type;
    const char * const name;
  } type_list [] =
    {
      { OML_INT32_VALUE,  "INTEGER"  },
      { OML_UINT32_VALUE, "UNSIGNED INTEGER" },
      { OML_INT64_VALUE,  "BIGINT"  },
      { OML_UINT64_VALUE, "UNSIGNED BIGINT" },
      { OML_DOUBLE_VALUE, "REAL" },
      { OML_STRING_VALUE, "TEXT" }
    };
  int i = 0;
  int n = sizeof (type_list) / sizeof (type_list[0]);
  OmlValueT type = OML_UNKNOWN_VALUE;

  for (i = 0; i < n; i++)
    if (strcmp (s, type_list[i].name) == 0) {
        type = type_list[i].type;
        break;
    }

  if (type == OML_UNKNOWN_VALUE)
    logwarn("Unknown SQL type '%s' --> OML_UNKNOWN_VALUE\n", s);

  return type;
}

const char*
oml_to_sql_type (OmlValueT type)
{
  switch (type) {
  case OML_LONG_VALUE:    return "INTEGER"; break;
  case OML_DOUBLE_VALUE:  return "REAL"; break;
  case OML_STRING_VALUE:  return "TEXT"; break;
  case OML_INT32_VALUE:   return "INTEGER"; break;
  case OML_UINT32_VALUE:  return "UNSIGNED INTEGER"; break;
  case OML_INT64_VALUE:   return "BIGINT"; break;
  case OML_UINT64_VALUE:  return "UNSIGNED BIGINT"; break; // FIXME:  UBIGINT not supported without loss of precision
  default:
    logerror("Unknown type %d\n", type);
    return NULL;
  }
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
