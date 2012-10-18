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

#ifndef UTIL_H__
#define UTIL_H__

#include <oml2/omlc.h>

#define LENGTH(a) ((sizeof (a)) / (sizeof ((a)[0])))

void chomp (char* str);

const char *skip_white (const char *p);
const char *find_white (const char *p);

char* to_octets (unsigned char* buf, int len);

int resolve_service(const char *service, int defport);

typedef enum {
  OML_URI_UNKNOWN = -1,
  OML_URI_FILE = 0,
  OML_URI_FILE_FLUSH,
  OML_URI_TCP,
  OML_URI_UDP,
} OmlURIType;

OmlURIType oml_uri_type(const char* uri);
#define oml_uri_is_file(t) (t>=OML_URI_FILE && t<=OML_URI_FILE_FLUSH)
#define oml_uri_is_network(t) (t>=OML_URI_TCP && t<=OML_URI_UDP)

#endif // UTIL_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
