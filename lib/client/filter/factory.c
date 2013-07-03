/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file factory.c
 * \brief Implements the functions to create and manage filters.
 */

#include <string.h>
#include <stdlib.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "mem.h"
#include "oml_value.h"
#include "factory.h"

typedef struct FilterType {
  const char * name;
  oml_filter_create create;
  oml_filter_set set;
  oml_filter_input input;
  oml_filter_output output;
  oml_filter_newwindow newwindow;
  oml_filter_meta meta;

  OmlFilterDef* definition;
  int output_count;

  struct FilterType* next;
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
  OmlValue* result = (OmlValue*)xmalloc(count * sizeof(OmlValue));

  if (!result) {
    logerror ("Failed to allocate memory for filter result vector\n");
    return NULL;
  }

  oml_value_array_init(result, count);

  int i = 0;
  for (i = 0; i < count; i++) {
    if (def[i].type == OML_INPUT_VALUE)
      oml_value_set_type(&result[i], type);
    else
      oml_value_set_type(&result[i], def[i].type);
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
    logerror ("Unknown filter '%s'.\n", filter_type);
    return NULL;  // nothing found
  }

  OmlFilter * f = (OmlFilter*)xmalloc(sizeof(OmlFilter));
  memset(f, 0, sizeof(OmlFilter));

  strncpy (f->name, instance_name, sizeof(f->name));
  if (f->name[sizeof(f->name)-1] != '\0')
    f->name[sizeof(f->name)-1] = '\0';
  f->index = index;
  f->set = ft->set;
  f->input_type = type;
  f->input = ft->input;
  f->output = ft->output;
  f->newwindow = ft->newwindow;
  f->meta = ft->meta;
  f->definition = ft->definition;   /* FIXME:  Copy and substitute OML_INPUT_VALUE types */
  f->output_count = ft->output_count;
  f->result = create_filter_result_vector (f->definition, type, ft->output_count);
  f->instance_data = ft->create(type, f->result);

  return f;
}

/** Destroy a filter and free its memory.
 *
 * This function is designed so it can be used in a while loop to clean up the
 * entire linked list:
 *
 *   while( (f=destroy_ms(f)) );
 *
 * \param f pointer to the filter to destroy
 * \returns f->next (can be NULL)
 */
OmlFilter *destroy_filter(OmlFilter* f) {
  OmlFilter *next;
  if (!f)
    return NULL;

  logdebug("Destroying filter %s at %p\n", f->name, f);

  next = f->next;

  if(f->result) {
    oml_value_array_reset(f->result, f->output_count);
    xfree(f->result);
  }
  if(f->instance_data)
    xfree(f->instance_data);
  xfree(f);

  return next;
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
  OmlFilterDef* copy = (OmlFilterDef*)xmalloc((count+1) * sizeof(OmlFilterDef));

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
             oml_filter_newwindow newwindow,
             oml_filter_meta meta,
             OmlFilterDef* filter_def)
{
  if (create == NULL || input == NULL || output == NULL) {
    logerror ("Filter %s needs a create function, an input function, and an output function (one of them was null).\n", filter_name);
    return -1;
  }

  if (filter_def == NULL) {
    logerror ("Filter %s needs a filter definition (got a null definition).\n", filter_name);
    return -1;
  }

  FilterType* ft = (FilterType*)xmalloc(sizeof(FilterType));
  ft->name = filter_name;
  ft->create = create;
  ft->set = set;
  ft->input = input;
  ft->output = output;
  ft->newwindow = newwindow;
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
void omlf_register_filter_last (void);
void omlf_register_filter_histogram (void);
void omlf_register_filter_stddev (void);
void omlf_register_filter_sum (void);
void omlf_register_filter_delta (void);

/**
 *  Register all built-in filters.
 */
void register_builtin_filters ()
{
  omlf_register_filter_average ();
  omlf_register_filter_first ();
  omlf_register_filter_last ();
  //  omlf_register_filter_histogram ();
  omlf_register_filter_stddev ();
  omlf_register_filter_sum ();
  omlf_register_filter_delta ();
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
