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
/*!\file sum_filter.c
  \brief Implements a filter which sums all the samples it received over the sample period.
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include "oml_value.h"
#include "sum_filter.h"

typedef struct _omlSumFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* value);

void*
omlf_sum_new(OmlValueT type, OmlValue* result)
{
  if (! omlc_is_numeric_type (type)) {
    logerror ("Can only handle numeric parameters\n");
    return NULL;
  }

  InstanceData* self = (InstanceData *)xmalloc(sizeof(InstanceData));

  if (self) {
    memset(self, 0, sizeof(InstanceData));

    self->sample_sum = 0.;
    self->result = result;
  } else {
    logerror ("Could not allocate %d bytes for sum filter instance data\n",
          sizeof(InstanceData));
    return NULL;
  }

  return self;
}

void
omlf_register_filter_sum (void)
{
  OmlFilterDef def [] =
    {
      { "sum", OML_DOUBLE_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter ("sum",
                        omlf_sum_new,
                        NULL,
                        sample,
                        process,
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

  self->sample_sum += val;

  return 0;
}

static int
process(OmlFilter* f, OmlWriter* writer)
{
  InstanceData* self = (InstanceData*)f->instance_data;

  omlc_set_double(*oml_value_get_value(&self->result[0]), self->sample_sum);
  writer->out(writer, self->result, f->output_count);

  self->sample_sum = 0.;

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
