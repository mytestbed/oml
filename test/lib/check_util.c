/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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
#include "check_util.h"
#include "oml2/omlc.h"
#include "oml2/oml_writer.h"


OmlValueU*
make_vector (void* v, OmlValueT type, int n)
{
  OmlValueU* result = (OmlValueU*)malloc (n * sizeof (OmlValueU));

  if (result)
    {
      int i = 0;
      memset (result, 0, n * sizeof (OmlValueU));

      switch (type)
        {
        case OML_LONG_VALUE:
          for (i = 0; i < n; i++)
            result[i].longValue = ((long*)v)[i];
          break;
        case OML_DOUBLE_VALUE:
          for (i = 0; i < n; i++)
            result[i].doubleValue = ((double*)v)[i];
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
      if (values[i].type != type)
        return 0;
  return 1;
}

int
vector_values_check (OmlValue* values, OmlValueU* expected, OmlValueT type, int n)
{
  double epsilon = 1e-9;
  int i = 0;
  for (i = 0; i < n; i++)
    {
      switch (type)
        {
        case OML_LONG_VALUE:
          if (values[i].value.longValue != expected[i].longValue)
            return 0;
          break;
        case OML_DOUBLE_VALUE:
          if (fabs(values[i].value.doubleValue - expected[i].doubleValue) > epsilon)
            return 0;
          break;
        case OML_STRING_VALUE:
          if (values[i].value.stringValue.size != expected[i].stringValue.size)
            return 0;
          if (strcmp (values[i].value.stringValue.ptr, expected[i].stringValue.ptr) != 0)
            return 0;
          break;
        default:
          return 0; // Fail on unknown value types
        }
    }
  return 1;
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
  oml_writer_cols out;

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

  w.out = test_writer_out;

  for (i = 0; i < test_data->count; i++)
    {
      TestVector* input = test_data->inputs[i];
      TestVector* output = test_data->outputs[i];
      OmlValue v;
      v.type = input->type;
      for (j = 0; j < input->length; j++, count++)
        {
          int res;
          v.value = input->vector[j];
          res = f->input(f, &v);
          fail_unless (res == 0);
        }

      f->output (f, (OmlWriter*)&w);

      fail_unless (w.count == output->length);
      fail_unless (vector_type_check (w.values, output->type, output->length));
      fail_unless (vector_values_check (w.values, output->vector, output->type, output->length));
    }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
