/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file average_filter.c
 * \brief Implements a filter which calculates the average over all the samples
 * it received over the sample period.
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "average_filter.h"

#define FILTER_NAME  "avg"

typedef struct OmlAvgFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* value);

static int
newwindow(OmlFilter* f);

void*
omlf_average_new(OmlValueT type, OmlValue* result)
{
  if (! omlc_is_numeric_type (type)) {
    logerror ("%s filter: Can only handle numeric parameters\n", FILTER_NAME);
    return NULL;
  }

  InstanceData* self = (InstanceData *)xmalloc(sizeof(InstanceData));

  if (self) {
    memset(self, 0, sizeof(InstanceData));

    self->sample_max = NAN;
    self->sample_min = NAN;
    self->sample_sum = NAN;
    self->sample_count = 0.;
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
omlf_register_filter_average (void)
{
  OmlFilterDef def [] =
    {
      { "avg", OML_DOUBLE_VALUE },
      { "min", OML_DOUBLE_VALUE },
      { "max", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter (FILTER_NAME,
                        omlf_average_new,
                        NULL,
                        sample,
                        process,
                        newwindow,
                        NULL,
                        def);
}

static int
sample(OmlFilter* f, OmlValue* value)
{
  InstanceData* self = (InstanceData*)f->instance_data;
  double val;

  if (! omlc_is_numeric (*value))
    return -1;

  val = oml_value_to_double (value);

  if (isnan(self->sample_sum)) {
    self->sample_sum = val;
  } else {
    self->sample_sum += val;
  }
  if (val < self->sample_min || isnan(self->sample_min)) self->sample_min = val;
  if (val > self->sample_max || isnan(self->sample_max)) self->sample_max = val;
  self->sample_count++;

  return 0;
}

static int
process(OmlFilter* f, OmlWriter* writer)
{
  InstanceData* self = (InstanceData*)f->instance_data;

  omlc_set_double(*oml_value_get_value(&self->result[0]), 1.0 * self->sample_sum / self->sample_count);
  omlc_set_double(*oml_value_get_value(&self->result[1]), self->sample_min);
  omlc_set_double(*oml_value_get_value(&self->result[2]), self->sample_max);

  writer->out(writer, self->result, f->output_count);

  return 0;
}

static int
newwindow(OmlFilter* f)
{
  InstanceData* self = (InstanceData*)f->instance_data;

  self->sample_max = NAN;
  self->sample_min = NAN;
  self->sample_sum = NAN;
  self->sample_count = 0.;

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
