/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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

#include <malloc.h>
#include <string.h>
#include <math.h>
#include <ocomm/o_log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
//#include "filter/factory.h"

typedef struct _omlHistFilter {
  //! Name used for debugging
  char name[64];

  //! Number of output value created
  int output_cnt;

  //! Set filter parameters
  oml_filter_set set;
  //! Process a new sample.
  oml_filter_input input;
  //! Calculate output, send it, and get ready for new sample period.
  oml_filter_output output;
  oml_filter_meta meta;

  OmlFilter* next;

  /* ------------------ */

  int index;

  OmlValue result[3];

  // Keep the sum and sample count to calculate average
  double  sample_sum;
  int     sample_count;

  double  sample_min;
  double  sample_max;
} OmlHistFilter;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValueU* values, OmlMP* mp);

static int
meta(OmlFilter* f, int param_index, char** namePtr, OmlValueT* type);

OmlFilter*
omlf_histogram_new(
  const char* name,
  OmlValueT type,
  int index
) {
  if (! (type == OML_LONG_VALUE || type == OML_DOUBLE_VALUE)) {
    o_log(O_LOG_ERROR, "Can only handle number parameters\n");
    return NULL;
  }

  OmlHistFilter* self = (OmlHistFilter *)malloc(sizeof(OmlHistFilter));
  memset(self, 0, sizeof(OmlHistFilter));

  strcpy(self->name, name);
  self->input = sample;
  self->output = process;
  self->meta = meta;

  self->index = index;
  self->output_cnt = 3;
  self->result[0].type = OML_DOUBLE_VALUE;
  self->result[1].type = OML_DOUBLE_VALUE;
  self->result[2].type = OML_DOUBLE_VALUE;

  self->sample_sum = 0;
  self->sample_count = 0;
  self->sample_min = HUGE;
  self->sample_max = -1 * HUGE;

  return (OmlFilter*)self;
}

static int
sample(
    OmlFilter* f,
    OmlValueU* values,  //! values of sample
    OmlMP*     mp      //! MP context
) {
  OmlHistFilter* self = (OmlHistFilter*)f;
  OmlValueU* value = values + self->index;
  OmlValueT type = mp->param_defs[self->index].param_types;
  double val;

  switch (type) {
  case OML_LONG_VALUE:
    val = value->longValue;
    break;
  case OML_DOUBLE_VALUE:
    val = value->doubleValue;
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
  OmlHistFilter* self = (OmlHistFilter*)f;

  if (self->sample_count > 0) {
    self->result[0].value.doubleValue = 1.0 * self->sample_sum / self->sample_count;
    self->result[1].value.doubleValue = self->sample_min;
    self->result[2].value.doubleValue = self->sample_max;
  } else {
    self->result[0].value.doubleValue = 0;
    self->result[1].value.doubleValue = 0;
    self->result[2].value.doubleValue = 0;
  }

  writer->out(writer, self->result, 3);


  return 0;
}

static int
meta(
  OmlFilter* f,
  int param_index,
  char** namePtr,
  OmlValueT* type
) {

  if (param_index > 2) return 0;

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
  return 1;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
