/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
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

#include <stdint.h>
#include <limits.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"

static inline int32_t oml_value_clamp_long (long value)
{
#if LONG_MAX > INT_MAX
  if (value > INT_MAX) {
    logwarn("Deprecated OML_LONG_VALUE %ld clamped to %d, please use OML_INT64_VALUE instead for such large values\n", value, INT_MAX);
    return INT_MAX;
  } else if (value < INT_MIN) {
    logwarn("Deprecated OML_LONG_VALUE %ld clamped to %d, please use OML_INT64_VALUE instead for such large values\n", value, INT_MIN);
    return INT_MIN;
  }
#endif
  return (int32_t)value;
}

/** Get the OmlValueU of an OmlValue.
 *
 * \param v pointer to the OmlValue to manipulate
 * \return a pointer to the OmlValueU contained in v
 */
#define oml_value_get_value(v) \
  ((OmlValueU*)&(v)->value)

/** Get the type of an OmlValue.
 *
 * \param v pointer to the OmlValue to manipulate
 * \return the type of the data stored in theOmlValue
 */
#define oml_value_get_type(v) \
  ((OmlValueT)(v)->type)

int oml_value_set(OmlValue* to, const OmlValueU* value, OmlValueT type);
int oml_value_copy(OmlValueU* value, OmlValueT type, OmlValue* to) __attribute__ ((deprecated));

void oml_value_init(OmlValue* v);
void oml_value_array_init(OmlValue* v, unsigned int n);
void oml_value_set_type(OmlValue* v, OmlValueT type);

int oml_value_reset(OmlValue* v);
void oml_value_array_reset(OmlValue* v, unsigned int n);

int oml_value_duplicate(OmlValue* dst, OmlValue* src);

const char* oml_type_to_s(OmlValueT type);
OmlValueT oml_type_from_s (const char *s);

char *oml_value_to_s (OmlValue *value, char *buf, size_t size);
int oml_value_from_s (OmlValue *value, const char *value_s);
int oml_value_from_typed_s (OmlValue *value, const char *type_s, const char *value_s);

double oml_value_to_double (OmlValue *value);
int oml_value_to_int (OmlValue *value);

uint8_t oml_value_string_to_bool(const char* value_s);

#endif // OML_VALUE_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
