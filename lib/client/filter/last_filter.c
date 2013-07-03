/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file last_filter.c
 * \brief Implements a filter which captures the last value presented
 */

#include <stdlib.h>
#include <string.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "last_filter.h"

#define FILTER_NAME "last"

typedef struct OmlLastFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* values);

static int
newwindow(OmlFilter* f);

void*
omlf_last_new(
  OmlValueT type,
  OmlValue* result
) {
  InstanceData* self = (InstanceData *)xmalloc(sizeof(InstanceData));
  if(self) {
    memset(self, 0, sizeof(InstanceData));

    self->result = result;
    self->sample_count = 0;

  } else {
    logerror ("%s filter: Could not allocate %d bytes for instance data\n",
        FILTER_NAME,
        sizeof(InstanceData));
    return NULL;
  }

  return self;
}

void
omlf_register_filter_last (void)
{
  OmlFilterDef def [] =
    {
      { "last", OML_INPUT_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter (FILTER_NAME,
            omlf_last_new,
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
  OmlValueU* v = oml_value_get_value(value);;
  OmlValueT type = oml_value_get_type(value);;

  if (type != self->result[0].type) {
    logwarn ("%s filter: Discarding sample type (%s) different from initial definition (%s)\n",
        FILTER_NAME, oml_type_to_s(type), oml_type_to_s(self->result[0].type));
    return 0;
  }
  self->sample_count++;
  /* Overwrite previously stored value */
  return oml_value_set(&self->result[0], v, type);
}

static int
process(
  OmlFilter* f,
  OmlWriter*  writer //! Write results of filter to this function
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  if (self->sample_count <= 0)
    return 1;

  writer->out(writer, self->result, f->output_count);

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
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
