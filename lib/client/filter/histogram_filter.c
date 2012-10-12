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
/*!\file histogram_filter.c
  \brief Implements a filter which calculates
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include "oml_value.h"
#include "filter/histogram_filter.h"

typedef struct _omlHistFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* value);

/*
static int
meta(OmlFilter* f, int param_index, char** namePtr, OmlValueT* type);
*/

void*
omlf_histogram_new(
  OmlValueT type,
  OmlValue* result
) {
  if (! (type == OML_LONG_VALUE || type == OML_DOUBLE_VALUE)) {
    logerror("Can only handle number parameters\n");
    return NULL;
  }

  InstanceData* self = (InstanceData *)malloc(sizeof(InstanceData));
  memset(self, 0, sizeof(InstanceData));

  self->sample_sum = 0;
  self->sample_count = 0;
  self->sample_min = HUGE;
  self->sample_max = -1 * HUGE;
  self->result = result;

  return self;
}

void
omlf_register_filter_histogram (void)
{
  OmlFilterDef def [] =
    {
      { "avg", OML_DOUBLE_VALUE },
      { "min", OML_DOUBLE_VALUE },
      { "max", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter ("histogram",
            omlf_histogram_new,
            NULL,
            sample,
            process,
            NULL,
            def);
}

static int
sample(
    OmlFilter* f,
    OmlValue*  value  //! values of sample
) {
  InstanceData* self = (InstanceData*)f->instance_data;
  OmlValueU* v = oml_value_get_value(value);;
  OmlValueT type = oml_value_get_type(value);;
  double val;

  switch (type) {
  case OML_LONG_VALUE:
    val = v->longValue;
    break;
  case OML_DOUBLE_VALUE:
    val = v->doubleValue;
    break;
  default:
    // raise error;
    return -1;
  }
  self->sample_sum += val;
  if (val < self->sample_min) self->sample_min = val;
  if (val > self->sample_max) self->sample_max = val;
  self->sample_count++;
  return 0;
}

static int
process(
  OmlFilter* f,
  OmlWriter*  writer //! Write results of filter to this function
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  if (self->sample_count > 0) {
    omlc_set_double(*oml_value_get_value(&self->result[0]), 1.0 * self->sample_sum / self->sample_count);
    omlc_set_double(*oml_value_get_value(&self->result[1]), self->sample_min);
    omlc_set_double(*oml_value_get_value(&self->result[2]), self->sample_max);
  } else {
    omlc_set_double(*oml_value_get_value(&self->result[0]), 0);
    omlc_set_double(*oml_value_get_value(&self->result[1]), 0);
    omlc_set_double(*oml_value_get_value(&self->result[2]), 0);
  }

  writer->out(writer, self->result, 3);


  return 0;
}

/*
static int
meta(
  OmlFilter* f,
  int param_index,
  char** namePtr,
  OmlValueT* type
) {

  if (param_index > 2) return -1;

  switch (param_index) {
  case 0:
    *namePtr = "avg";
    break;
  case 1:
    *namePtr = "min";
    break;
  case 2:
    *namePtr = "max";
    break;
  }
  *type = OML_DOUBLE_VALUE;
  return 0;
}
*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
