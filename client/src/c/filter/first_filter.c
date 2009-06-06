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
  \brief Implements a filter which captures the first value presented
*/

#include <malloc.h>
#include <string.h>
#include <ocomm/o_log.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
//#include "filter/factory.h"


typedef struct _omlFirstFilter {
  //! Name used for debugging
  char name[64];

  //! Number of output value created
  int output_cnt;

  //! Process a new sample.
  oml_filter_sample sample;
  //! Calculate output, send it, and get ready for new sample period.
  oml_filter_process process;
  oml_filter_meta meta;

  OmlFilter* next;

  /* ------------------ */
  int index;

  OmlValue result;
  int is_first;			/* set to true if no value has been stored */
} OmlFirstFilter;

static int
process(OmlFilter* filter, OmlWriter* writer);

static int
sample(OmlFilter* f, OmlValueU* values, OmlMP* mp);

static int
meta(OmlFilter* f, int param_index, char** namePtr, OmlValueT* type);

OmlFilter*
omlf_first_new(
  const char* name,
  OmlValueT type,
  int index
) {
  OmlFirstFilter* self = (OmlFirstFilter *)malloc(sizeof(OmlFirstFilter));
  memset(self, 0, sizeof(OmlFirstFilter));

  strcpy(self->name, name);
  self->sample = sample;
  self->process = process;
  self->meta = meta;

  self->index = index;
  self->output_cnt = 1;
  self->result.type = type;
  self->is_first = 1;
  return (OmlFilter*)self;
}

static int
sample(
    OmlFilter* f,
    OmlValueU* values,  //! values of sample
    OmlMP*     mp      //! MP context
) {
  OmlFirstFilter* self = (OmlFirstFilter*)f;
  OmlValueT type = mp->param_defs[self->index].param_types;

  if (type != self->result.type) {
    o_log(O_LOG_ERROR, "Different type from initial definition\n");
    return 0;
/*     OmlValueT tt = self->result.type; */
/*     if (!((type == OML_STRING_PTR_VALUE && tt == OML_STRING_VALUE) */
/*           || (type == OML_STRING_VALUE && tt == OML_STRING_PTR_VALUE))) { */
/*       o_log(O_LOG_ERROR, "Different type from initial definition\n"); */
/*       return 0; */
/*     } */
  }
  if (self->is_first) {
    OmlValueU* value = values + self->index;
    self->is_first = 0;
    return oml_value_copy(value, type, &self->result);
  }
  return 1;
}

static int
process(
  OmlFilter* f,
  OmlWriter*  writer //! Write results of filter to this function
) {
  OmlFirstFilter* self = (OmlFirstFilter*)f;

  self->is_first = 1;
  writer->out(writer, &self->result, 1);
  oml_value_reset(&self->result);
  return 1;
}

static int
meta(
  OmlFilter* f,
  int param_index,
  char** namePtr,
  OmlValueT* type
) {
  OmlFirstFilter* self = (OmlFirstFilter*)f;

  if (param_index > 0) return 0;

  *namePtr = NULL;
  *type = self->result.type;
  return 1;
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
