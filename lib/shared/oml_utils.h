/*
 * Copyright 2007-2015 National ICT Australia (NICTA), Australia
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

#ifndef UTIL_H__
#define UTIL_H__

#include "string_utils.h"

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

char* to_octets (unsigned char* buf, int len);

int resolve_service(const char *service, int defport);

/** Bit field allowing to identify the type(s) of a URI or scheme */
typedef enum {
  OML_URI_UNKNOWN = 0,
  OML_URI_FILE = 1,
  OML_URI_FLUSH = 1<<1,
  OML_URI_NET = 1<<2,
  OML_URI_FILE_FLUSH = OML_URI_FILE | OML_URI_FLUSH, /* 1 + 1<<1 = 3 */
  OML_URI_TCP = 1<<3 | OML_URI_NET,
  OML_URI_UDP = 1<<4 | OML_URI_NET,
  OML_URI_ZLIB,
} OmlURIType;

#define DEF_PORT 3003
#define DEF_PORT_STRING  xstr(DEF_PORT)

OmlURIType oml_uri_type(const char* uri);
int oml_uri_is_file(OmlURIType t);
int oml_uri_is_network(OmlURIType t);
int parse_uri (const char *uri, const char **scheme, const char **host, const char **port, const char **path);
char *default_uri(const char *app_name, const char *name, const char *domain);

#endif // UTIL_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
