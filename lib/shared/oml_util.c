/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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

#include <stdio.h> // For snprintf
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "log.h"
#include "mem.h"
#include "oml_util.h"

/**
 * \brief Remove trailing space from a string
 * \param[in,out] str the string to chomp, with the first trailing space replaced by '\0'
 */
void
chomp (char* str)
{
  char* p = str + strlen (str);

  while (p != str && isspace (*--p));

  *++p = '\0';
}

const char *
skip_white (const char *p)
{
  if (p)
    while (*p && isspace (*p)) p++;
  return p;
}

const char *
find_white (const char *p)
{
  if (p)
    while (*p && !isspace (*p)) p++;
  return p;
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
      { OML_STRING_VALUE, "TEXT" },
      { OML_BLOB_VALUE,   "BLOB" }
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

char*
to_octets (unsigned char* buf, int len)
{
  const int octet_width = 3;
  const int columns = 16;
  const int rows = len / columns + 1;
  char* out = xmalloc (octet_width * len + 1 + rows);
  int i = 0;
  int col = 1;
  int count = 0;
  for (i = 0, col = 1; i < len; i++, col++) {
    int n = snprintf (&out[count], octet_width + 1, "%02x ", (unsigned int)buf[i]);
    count += n;

    if (col >= columns) {
      n = snprintf (&out[count], 3, "\n");
      count += n;
      col = 0;
    }
  }

  return out;
}

/**
 * Parse the scheme of na URI and return its type as an +OmlURIType+
 * \param [in] uri the URI to parse
 * \return an +OmlURIType+ indicating the type of the URI
 */
OmlURIType oml_uri_type(const char* uri) {
  int len = strlen(uri);
  
  if(len>5 && !strncmp(uri, "flush", 5)) 
    return OML_URI_FILE_FLUSH;
  else if(len>4 && !strncmp(uri, "file", 4))
    return OML_URI_FILE;
  else if(len>3 && !strncmp(uri, "tcp", 3))
      return OML_URI_TCP;
  else if(len>3 && !strncmp(uri, "udp", 3))
      return OML_URI_UDP;
  return OML_URI_UNKNOWN;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
