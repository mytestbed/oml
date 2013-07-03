/*
 * Copyright 2010-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file delta_filter.c
 * \brief Implements a filter which returns the difference between the
 * previously reported and current sample values. While it's at it, it also
 * reports the current (i.e. last) value.
*/

#include <stdlib.h>
#include <string.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "delta_filter.h"

#define FILTER_NAME  "delta"

typedef struct OmlDeltaFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* values);

static int
newwindow(OmlFilter* f);

void*
omlf_delta_new(
  OmlValueT type,
  OmlValue* result
) {
  if (! omlc_is_numeric_type (type)) {
    logerror ("%s filter: Can only handle numeric parameters\n", FILTER_NAME);
    return NULL;
  }

  InstanceData* self = (InstanceData *)xmalloc(sizeof(InstanceData));
  if (self) {
    memset(self, 0, sizeof(InstanceData));

    self->previous = 0.;
    self->current = 0.;
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
omlf_register_filter_delta (void)
{
  OmlFilterDef def [] =
    {
      { "delta", OML_DOUBLE_VALUE },
      { "last", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter (FILTER_NAME,
            omlf_delta_new,
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
    OmlValue * value  //! values of sample
) {
  InstanceData* self = (InstanceData*)f->instance_data;
  double val;

  if (! omlc_is_numeric (*value))
    return -1;

  val = oml_value_to_double (value);
  self->current = val;
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

  omlc_set_double(*oml_value_get_value(&self->result[0]), (self->current - self->previous));
  omlc_set_double(*oml_value_get_value(&self->result[1]), self->current);
  writer->out(writer, self->result, f->output_count);

  return 0;
}

static int
newwindow(OmlFilter* f)
{
  InstanceData* self = (InstanceData*)f->instance_data;

  self->previous = self->current;
  self->current = 0;
  self->sample_count = 0;

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
