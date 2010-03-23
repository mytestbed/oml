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
#include "util.h"

void
chomp (char* str)
{
  char* p = str + strlen (str);

  while (p != str && isspace (*--p));

  *++p = '\0';
}


OmlValueT sql_to_oml_type (const char* type)
{
  if (strcmp (type, "INTEGER") == 0)
	return OML_LONG_VALUE;
  else if (strcmp (type, "REAL") == 0)
	return OML_DOUBLE_VALUE;
  else if (strcmp (type, "TEXT") == 0)
	return OML_STRING_VALUE;
  else
	{
	  o_log (O_LOG_WARN, "Unknown SQL type %s --> OML_UNKNOWN_VALUE\n", type);
	  return OML_UNKNOWN_VALUE; // error
	}
}

const char*
oml_to_sql_type (OmlValueT type)
{
  switch (type) {
  case OML_LONG_VALUE:    return "INTEGER"; break;
  case OML_DOUBLE_VALUE:  return "REAL"; break;
  case OML_STRING_VALUE:  return "TEXT"; break;
  default:
	o_log(O_LOG_ERROR, "Unknown type %d\n", type);
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
