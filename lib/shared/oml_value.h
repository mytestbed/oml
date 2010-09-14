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
#ifndef OML_VALUE_H__
#define OML_VALUE_H__

#include <oml2/omlc.h>
#include <stdint.h>
#include <limits.h>

static inline int32_t oml_value_clamp_long (long value)
{
#if LONG_MAX > INT_MAX
  if (value > INT_MAX)
    return INT_MAX;
  if (value < INT_MIN)
    return INT_MIN;
#endif
  return (int32_t)value;
}


extern char*
oml_type_to_s(OmlValueT type);

extern OmlValueT
oml_type_from_s (const char *s);

void
oml_value_to_s (OmlValueU *value, OmlValueT type, char *buf);

int
oml_value_from_s (OmlValue *value, const char *value_s);
int

oml_value_from_typed_s (OmlValue *value, const char *type_s, const char *value_s);

double
oml_value_to_double (OmlValue *value);

#endif // OML_VALUE_H__

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
