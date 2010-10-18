/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
  \brief Implements a filter which captures the first value presented
*/

#include <stdlib.h>
#include <string.h>
#include <log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include "filter/first_filter.h"

typedef struct _omlFirstFilterInstanceData InstanceData;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValue* values);

static int
meta(OmlFilter* f, int param_index, char** namePtr, OmlValueT* type);

void*
omlf_first_new(
  OmlValueT type,
  OmlValue* result
) {
  InstanceData* self = (InstanceData *)malloc(sizeof(InstanceData));
  memset(self, 0, sizeof(InstanceData));

  self->is_first = 1;
  self->result = result;
  self->result[0].type = type;  // FIXME:  Is this needed?

  if (type == OML_STRING_VALUE)
    omlc_set_const_string (self->result[0].value, "");

  return self;
}

void
omlf_register_filter_first (void)
{
  OmlFilterDef def [] =
    {
      { "first", OML_INPUT_VALUE },
      { NULL, 0 }
    };

  omlf_register_filter ("first",
            omlf_first_new,
            NULL,
            sample,
            process,
            meta,
            def);
}

static int
sample(
    OmlFilter* f,
    OmlValue * value  //! values of sample
) {
  InstanceData* self = (InstanceData*)f->instance_data;
  OmlValueU* v = &value->value;
  OmlValueT type = value->type;

  if (type != self->result[0].type) {
    logerror ("Different type from initial definition\n");
    return 0;
  }
  if (self->is_first) {
    self->is_first = 0;
    return oml_value_copy(v, type, &self->result[0]);
  }
  return 0;
}

static int
process(
  OmlFilter* f,
  OmlWriter*  writer //! Write results of filter to this function
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  self->is_first = 1;
  writer->out(writer, self->result, 1);
  oml_value_reset(&self->result[0]);
  if (self->result[0].type == OML_STRING_VALUE)
    omlc_set_const_string (self->result[0].value, "");

  return 0;
}

static int
meta(
  OmlFilter* f,
  int param_index,
  char** namePtr,
  OmlValueT* type
) {
  InstanceData* self = (InstanceData*)f->instance_data;

  if (param_index > 0) return 0;

  *namePtr = NULL;
  *type = self->result[0].type;
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
