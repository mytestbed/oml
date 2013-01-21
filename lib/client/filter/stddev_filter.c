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
/*! \file stddev_filter.c \brief Implements a filter that calculates
 *  the standard deviation (and variance) over all samples it received
 *  over the sample period.
 *
 *  This filter calculates the standard deviation using a running
 *  accumulation method due to B.P. Belford, cited in:
 *
 *  Donald Knuth, Art of Computer Programming, Vol 2, page 232, 3rd edition.
 *
 *  For k=1, initialize with:
 *
 *  M_1 = x_1, S_1 = 0
 *
 *  where x_i is the i-th input sample.  For k > 1, compute the
 *  recurrence relations:
 *
 *  M_k = M_{k-1} + (x_k - M_{k-1}) / k
 *  S_k = S_{k-1} + (x_k - M_{k-1}) * (x_k - M_k)
 *
 *  Then the variance of the k-th sample is s^2 = S_k / (k-1).
 *
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "stddev_filter.h"

#define FILTER_NAME "stddev"

typedef struct _omlStddevFilterInstanceData InstanceData;

static int
input (OmlFilter* f, OmlValue* value);

static int
output (OmlFilter* f, OmlWriter* writer);

void*
omlf_stddev_new(
  OmlValueT type,
  OmlValue* result
  ) {
  if (! omlc_is_numeric_type (type)) {
    logerror ("%s filter: Can only handle numeric parameters\n", FILTER_NAME);
    return NULL;
  }

  InstanceData* self = (InstanceData*)xmalloc(sizeof(InstanceData));

  if (self) {
    memset(self, 0, sizeof(InstanceData));

    self->m = 0;
    self->s = 0;
    self->sample_count = 0;
    self->result = result;

    return self;
  } else {
    logerror ("%s filter: Could not allocate %d bytes for instance data\n",
        FILTER_NAME,
        sizeof(InstanceData));
    return NULL;
  }
}

void
omlf_register_filter_stddev (void)
{
  OmlFilterDef def [] =
    {
      { "stddev", OML_DOUBLE_VALUE },
      { "variance", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter (FILTER_NAME,
                        omlf_stddev_new,
                        NULL,
                        input,
                        output,
                        NULL,
                        def);
}

static int
input (
  OmlFilter* f,
  OmlValue* value
) {
  InstanceData* self = (InstanceData*)f->instance_data;
  double m = self->m;
  double s = self->s;
  double new_m;
  double new_s;
  double val;

  if (! omlc_is_numeric (*value))
    return -1;

  val = oml_value_to_double (value);

  self->sample_count++;
  if (self->sample_count == 1) {
    new_m = val;
    new_s = 0;
  } else {
    new_m = m + (val - m) / self->sample_count;
    new_s = s + (val - m) * (val - new_m);
  }
  self->m = new_m;
  self->s = new_s;
  return 0;
}

static int
output (
  OmlFilter* f,
  OmlWriter* writer
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  if (self->sample_count <= 0)
    return 1;

  omlc_set_double(*oml_value_get_value(&self->result[1]), 1.0 * self->s / (self->sample_count - 1));
  omlc_set_double(*oml_value_get_value(&self->result[0]), sqrt (omlc_get_double(*oml_value_get_value(&self->result[1]))));

  self->m = 0;
  self->s = 0;
  self->sample_count = 0;

  writer->out (writer, self->result, f->output_count);
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
