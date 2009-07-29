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
/*!\file average_filter.c
  \brief Implements a filter which calculates the average over all
  the samples it received over the sample period.
*/

#include <malloc.h>
#include <string.h>
#include <math.h>
#include <ocomm/o_log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include "average_filter.h"

typedef struct _omlAvgFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* value);

void*
omlf_average_new(
  OmlValueT type,
  OmlValue* result
) {
  if (! omlc_is_numeric_type (type)) {
    o_log(O_LOG_ERROR, "Can only handle numeric parameters\n");
    return NULL;
  }

  InstanceData* self = (InstanceData *)malloc(sizeof(InstanceData));

  if (self) {
	memset(self, 0, sizeof(InstanceData));

	self->sample_sum = 0;
	self->sample_count = 0;
	self->sample_min = HUGE;
	self->sample_max = -1 * HUGE;
	self->result = result;
	return self;
  } else {
	o_log(O_LOG_ERROR, "Could not allocate %d bytes for avg filter instance data\n",
		  sizeof(InstanceData));
	return NULL;
  }
}

void
omlf_register_filter_average (void)
{
  OmlFilterDef def [] =
    {
      { "avg", OML_DOUBLE_VALUE },
      { "min", OML_DOUBLE_VALUE },
      { "max", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter ("avg",
						omlf_average_new,
						NULL,
						sample,
						process,
						NULL,
						def);
}

static int
sample(
    OmlFilter* f,
    OmlValue*  value  //! value of this sample
) {
  InstanceData* self = (InstanceData*)f->instance_data;
  OmlValueU* v = &value->value;
  OmlValueT type = value->type;
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
  OmlWriter* writer //! Write results of filter to this function
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  if (self->sample_count > 0) {
    self->result[0].value.doubleValue = 1.0 * self->sample_sum / self->sample_count;
    self->result[1].value.doubleValue = self->sample_min;
    self->result[2].value.doubleValue = self->sample_max;
  } else {
    self->result[0].value.doubleValue = 0;
    self->result[1].value.doubleValue = 0;
    self->result[2].value.doubleValue = 0;
  }

  writer->out(writer, self->result, f->output_count);

  return 0;
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
