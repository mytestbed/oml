/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
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

#ifndef SCHEMA_H__
#define SCHEMA_H__

#include <oml2/omlc.h>

struct schema_field
{
  char *name;
  OmlValueT type;
};

struct schema
{
  char *name;
  struct schema_field *fields;
  int nfields;
  int index;
};

struct schema* schema_from_meta (char *meta);
struct schema* schema_from_sql (char *sql);
struct schema* schema_copy (struct schema *schema);
void schema_free (struct schema *schema);
int schema_diff (struct schema *s1, struct schema *s2);


#endif /* SCHEMA_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
