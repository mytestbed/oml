/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
#include <math.h>
#include <check.h>

#include "oml2/omlc.h"
#include "oml_value.h"
#include "oml2/oml_writer.h"
#include "check_util.h"


OmlValueU*
make_vector (void* v, OmlValueT type, int n)
{
  OmlValueU* result = (OmlValueU*)malloc (n * sizeof (OmlValueU));

  if (result)
    {
      int i = 0;
      omlc_zero_array(result, n);

      switch (type)
        {
        case OML_LONG_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_long(result[i], ((long*)v)[i]);
          break;
        case OML_DOUBLE_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_double(result[i], ((double*)v)[i]);
          break;
        case OML_INT32_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_int32(result[i], ((int32_t*)v)[i]);
          break;
        case OML_UINT32_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_uint32(result[i], ((uint32_t*)v)[i]);
          break;
        case OML_INT64_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_int64(result[i], ((int64_t*)v)[i]);
          break;
        case OML_UINT64_VALUE:
          for (i = 0; i < n; i++)
            omlc_set_uint64(result[i], ((uint64_t*)v)[i]);
          break;
        default:
          // Can't do non-numeric types yet
          return NULL;
        }

      return result;
    }
  else
    return NULL;
}

TestVector*
make_test_vector (void* v, OmlValueT type, int n)
{
  TestVector* result = (TestVector*)malloc (sizeof (TestVector));
  if (result)
    {
      OmlValueU* oml_v = make_vector (v, type, n);

      if (oml_v)
        {
          memset (result, 0, sizeof (TestVector));
          result->length = n;
          result->type = type;
          result->vector = oml_v;
          return result;
        }
      else
        {
          free (result);
          return NULL;
        }
    }
  else
    return NULL;
}

int
vector_type_check (OmlValue* values, OmlValueT type, int n)
{
  int i = 0;
  for (i = 0; i < n; i++)
      if (oml_value_get_type(&values[i]) != type)
        return 0;
  return 1;
}

int
vector_values_check (OmlValue* values, OmlValueU* expected, OmlValueT type, int n)
{
  double epsilon = 1e-9;
  int i = 0;
  for (i = 0; i < n; i++) {
      switch (type) {
        case OML_LONG_VALUE:
          if (omlc_get_long(*oml_value_get_value(&values[i])) != omlc_get_long(expected[i]))
            return 0;
          break;
        case OML_DOUBLE_VALUE:
          if (fabs(omlc_get_double(*oml_value_get_value(&values[i])) - omlc_get_double(expected[i])) > epsilon)
            return 0;
          break;
        case OML_STRING_VALUE:
          if (omlc_get_string_length(*oml_value_get_value(&values[i])) != omlc_get_string_length(expected[i]))
            return 0;
          if (strcmp (omlc_get_string_ptr(*oml_value_get_value(&values[i])), omlc_get_string_ptr(expected[i])) != 0)
            return 0;
          break;
        case OML_INT32_VALUE:
          if (omlc_get_int32(*oml_value_get_value(&values[i])) != omlc_get_int32(expected[i]))
            return 0;
          break;
        case OML_UINT32_VALUE:
          if (omlc_get_uint32(*oml_value_get_value(&values[i])) != omlc_get_uint32(expected[i]))
            return 0;
          break;
        case OML_INT64_VALUE:
          if (omlc_get_int64(*oml_value_get_value(&values[i])) != omlc_get_int64(expected[i]))
            return 0;
          break;
        case OML_UINT64_VALUE:
          if (omlc_get_uint64(*oml_value_get_value(&values[i])) != omlc_get_uint64(expected[i]))
            return 0;
          break;
        default:
          return 0; // Fail on unknown value types
        }
    }
  return 1;
}

char *vector_values_stringify(OmlValue *v, int n)
{
  static char str[256];
  int i = 0, used = 0;

  do {
    if (i>0) {
      strncat(str, " ", sizeof(str) - used);
      used = strlen(str);
    }
    oml_value_to_s(&v[i], &str[used], sizeof(str) - used);
    used = strlen(str);
  } while (++i<n && used < sizeof(str));

  return str;
}

char *vector_values_ut_stringify(OmlValueU* values, OmlValueT type, int n)
{
  OmlValue v;
  static char str[256];
  int i = 0, used = 0;

  oml_value_init(&v);

  do {
    if (i>0) {
      strncat(str, " ", sizeof(str) - used);
      used = strlen(str);
    }
    oml_value_set(&v, &values[i], type);
    oml_value_to_s(&v, &str[used], sizeof(str) - used);
    used = strlen(str);
  } while (++i<n && used < sizeof(str));


  oml_value_reset(&v);
  return str;
}


typedef struct _omlTestWriter
{
  oml_writer_meta meta;
  oml_writer_header_done header_done;

  //! Called before and after writing individual
  // filter results with 'out'
  oml_writer_row_start row_start;
  oml_writer_row_end row_end;

  //! Writing the results from a single filter.
  oml_writer_out out;

  oml_writer_close close;

  struct _omlWriter* next;

  OmlValue* values;
  int count;
} OmlTestWriter;


static int
test_writer_out(OmlWriter* writer, OmlValue* values, int value_count)
{
  ((OmlTestWriter*)writer)->values = values;
  ((OmlTestWriter*)writer)->count = value_count;
  return 0;
}

void
run_filter_test (TestData* test_data, OmlFilter* f)
{
  int i = 0, j = 0;
  int count = 0;
  OmlTestWriter w;
  OmlValue v;

  oml_value_init(&v);

  w.out = test_writer_out;

  for (i = 0; i < test_data->count; i++)
    {
      TestVector* input = test_data->inputs[i];
      TestVector* output = test_data->outputs[i];
      oml_value_set_type(&v, input->type);
      for (j = 0; j < input->length; j++, count++)
        {
          int res;
          /* XXX: is this really legal? */
          v.value = input->vector[j];
          res = f->input(f, &v);
          fail_unless (res == 0);
        }

      f->output (f, (OmlWriter*)&w);
      f->newwindow (f);

      fail_unless (w.count == output->length);
      fail_unless (vector_type_check (w.values, output->type, output->length));
      fail_unless (vector_values_check (w.values, output->vector, output->type, output->length),
          "Output mismatch in test vector %d [%s] for filter '%s': expected [%s] but got [%s]",
          i,
          vector_values_ut_stringify(input->vector, input->type, input->length),
          f->name,
          vector_values_ut_stringify(output->vector, output->type, output->length),
          vector_values_stringify(w.values, w.count));
    }

  oml_value_reset(&v);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
