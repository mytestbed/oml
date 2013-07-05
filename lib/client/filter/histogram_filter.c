/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file histogram_filter.c
 * \brief Implements a filter which calculates
 *
 * XXX: This filter is currently non-functional, and therefore disabled.
 *
 * \see register_builtin_filters
 */

#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "histogram_filter.h"

#define FILTER_NAME "histogram"

typedef struct OmlHistFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* value);

static int
newwindow(OmlFilter* f);

/*
static int
meta(OmlFilter* f, int param_index, char** namePtr, OmlValueT* type);
*/

void*
omlf_histogram_new(
  OmlValueT type,
  OmlValue* result
) {
  if (! omlc_is_numeric_type (type)) {
    logerror ("%s filter: Can only handle numeric parameters\n", FILTER_NAME);
    return NULL;
  }

  InstanceData* self = (InstanceData *)xmalloc(sizeof(InstanceData));

  if(self) {
    memset(self, 0, sizeof(InstanceData));

    self->sample_sum = 0;
    self->sample_min = HUGE;
    self->sample_max = -1 * HUGE;
    self->sample_count = 0;
    self->result = result;
  } else {
    logerror ("%s filter: Could not allocate %d bytes for instance data\n",
        FILTER_NAME,
        sizeof(InstanceData));
    return NULL;
  }

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

  omlf_register_filter (FILTER_NAME,
            omlf_histogram_new,
            NULL,
            sample,
            process,
            newwindow,
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

  if (self->sample_count <= 0)
    return 1;

  omlc_set_double(*oml_value_get_value(&self->result[0]), 1.0 * self->sample_sum / self->sample_count);
  omlc_set_double(*oml_value_get_value(&self->result[1]), self->sample_min);
  omlc_set_double(*oml_value_get_value(&self->result[2]), self->sample_max);

  writer->out(writer, self->result, 3);

  return 0;
}

static int
newwindow(OmlFilter* f)
{
  InstanceData* self = (InstanceData*)f->instance_data;

  self->sample_count = 0;

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
