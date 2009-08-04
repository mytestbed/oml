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
/*!\file factory.c
  \brief Implements the functions to create and manage filters.
*/

#include <string.h>
#include <malloc.h>
#include <ocomm/o_log.h>
#include "filter/factory.h"

typedef struct _filterType {
  const char * name;
  oml_filter_create create;
  oml_filter_set set;
  oml_filter_input input;
  oml_filter_output output;
  oml_filter_meta meta;

  OmlFilterDef* definition;
  int output_count;

  struct _filterType* next;
} FilterType;

static FilterType* filter_types = NULL;

const char*
next_filter_name(void)
{
  static FilterType* ft = NULL;
  static int done = 1;
  const char* name;

  if (ft == NULL && done)
	{
	  done = 0;
	  ft = filter_types;
	}
  else if (ft == NULL)
	{
	  done = 1;
	  return NULL;
	}

  name = ft->name;

  ft = ft->next;

  return name;
}

static OmlValue*
create_filter_result_vector (OmlFilterDef* def, OmlValueT type, int count)
{
  OmlValue* result = (OmlValue*)malloc(count * sizeof(OmlValueU));

  if (!result) {
    o_log (O_LOG_ERROR, "Failed to allocate memory for filter result vector.\n");
    return NULL;
  }

  memset (result, 0, count * sizeof (OmlValueU));

  int i = 0;
  for (i = 0; i < count; i++) {
    result[i].type = def[i].type;

    if (result[i].type == OML_INPUT_VALUE)
      result[i].type = type;

	if (result[i].type == OML_STRING_VALUE)
	  {
		// oml_value_reset() doesn't really do a hard reset for
		// strings... it keeps existing memory hanging around, if any.
		// so, we need to set up the string a bit more carefully.
		result[i].value.stringValue.is_const = 0;
		result[i].value.stringValue.ptr = NULL;
		result[i].value.stringValue.size = 0;
		result[i].value.stringValue.length = 0;
	  }
	else
	  oml_value_reset(&result[i]);
  }

  return result;
}

/*! Create an instance of a filter of type 'fname'.
 */
OmlFilter*
create_filter(
    const char* filter_type,
    const char* instance_name,
    OmlValueT   type,
    int         index
) {
  FilterType* ft = filter_types;
  for (; ft != NULL; ft = ft->next) {
    if (strcmp (filter_type, ft->name) == 0) break;
  }
  if (ft == NULL) {
    o_log(O_LOG_ERROR, "Unknown filter '%s'.\n", filter_type);
    return NULL;  // nothing found
  }

  OmlFilter * f = (OmlFilter*)malloc(sizeof(OmlFilter));
  memset(f, 0, sizeof(OmlFilter));

  strcpy (f->name, instance_name); /* FIXME:  strncpy */
  f->index = index;
  f->set = ft->set;
  f->input_type = type;
  f->input = ft->input;
  f->output = ft->output;
  f->meta = ft->meta;
  f->definition = ft->definition;   /* FIXME:  Copy and substitute OML_INPUT_VALUE types */
  f->output_count = ft->output_count;
  f->result = create_filter_result_vector (f->definition, type, ft->output_count);
  f->instance_data = ft->create(type, f->result);

  return f;
}

static int
default_filter_set (OmlFilter* filter,
		    const char* name,
		    OmlValue* value)
{
  (void)filter;
  (void)name;
  (void)value;
  /* The default parameter setting function ignores parameters */
  return 0;
}

static int
default_filter_meta (OmlFilter* filter,
		     int index_offset,
		     char** name_ptr,
		     OmlValueT* type_ptr)
{
  if (!filter || (index_offset < 0) || (index_offset >= filter->output_count))
    return -1;

  if (name_ptr)
    *name_ptr = (char*)filter->definition[index_offset].name;

  if (type_ptr) {
    OmlValueT type = filter->definition[index_offset].type;

    if (type == OML_INPUT_VALUE)
      type = filter->input_type;

    *type_ptr = type;
  }
  return 0;
}

static OmlFilterDef* copy_filter_definition (OmlFilterDef* def,
					     int count)
{
  OmlFilterDef* copy = (OmlFilterDef*)malloc((count+1) * sizeof(OmlFilterDef));

  memcpy (copy, def, (count+1) * sizeof (OmlFilterDef));

  return copy;
}

/*! Register an instance of a filter of type filter_name.
 */
int
omlf_register_filter(const char* filter_name,
		     oml_filter_create create,
		     oml_filter_set set,
		     oml_filter_input input,
		     oml_filter_output output,
		     oml_filter_meta meta,
		     OmlFilterDef* filter_def)
{
  if (create == NULL || input == NULL || output == NULL) {
    o_log (O_LOG_ERROR, "Filter %s needs a create function, an input function, and an output function (one of them was null).\n", filter_name);
    return -1;
  }

  if (filter_def == NULL) {
    o_log (O_LOG_ERROR, "Filter %s needs a filter definition (got a null definition).\n", filter_name);
    return -1;
  }

  FilterType* ft = (FilterType*)malloc(sizeof(FilterType));
  ft->name = filter_name;
  ft->create = create;
  ft->set = set;
  ft->input = input;
  ft->output = output;
  ft->output_count = 0;

  OmlFilterDef* dp = filter_def;
  for (; !(dp->name == NULL && dp->type == 0); dp++)
    ft->output_count++;

  ft->definition = copy_filter_definition (filter_def, ft->output_count);

  if (set == NULL)
      ft->set = default_filter_set;

  if (meta == NULL)
    ft->meta = default_filter_meta;
  else
    ft->meta = meta;

  ft->next = filter_types;
  filter_types = ft;

  return 0;
}

/* Builtin filter registration functions */
void omlf_register_filter_average (void);
void omlf_register_filter_first (void);
void omlf_register_filter_histogram (void);
void omlf_register_filter_stddev (void);

/**
 *  Register all built-in filters.
 */
void register_builtin_filters ()
{
  omlf_register_filter_average ();
  omlf_register_filter_first ();
  omlf_register_filter_histogram ();
  omlf_register_filter_stddev ();
}


/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
