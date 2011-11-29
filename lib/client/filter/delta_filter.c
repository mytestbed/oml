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
/*!\file delta_filter.c
  \brief Implements a filter which returns the difference between the previously
  reported and current sample values. While it's at it, it also reports
  the current (i.e. last) value.
*/

#include <stdlib.h>
#include <string.h>
#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include "oml_value.h"
#include "filter/delta_filter.h"

typedef struct _omlDeltaFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* values);

void*
omlf_delta_new(
  OmlValueT type,
  OmlValue* result
) {
  if (! omlc_is_numeric_type (type)) {
    logerror ("Can only handle numeric parameters\n");
    return NULL;
  }

  InstanceData* self = (InstanceData *)malloc(sizeof(InstanceData));
  if (self) {
    memset(self, 0, sizeof(InstanceData));

    self->previous = 0;
    self->current = 0;
    self->result = result;

  } else {
    logerror ("Could not allocate %d bytes for delta filter instance data\n",
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

  omlf_register_filter ("delta",
            omlf_delta_new,
            NULL,
            sample,
            process,
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
  return 0;
}

static int
process(
  OmlFilter* f,
  OmlWriter*  writer //! Write results of filter to this function
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  omlc_set_double(self->result[0].value, (self->current - self->previous));
  omlc_set_double(self->result[1].value, self->current);
  writer->out(writer, self->result, f->output_count);

  self->previous = self->current;
  self->current = 0;

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
