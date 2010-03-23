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
#ifndef OML_FILTER_H_
#define OML_FILTER_H_

#include <oml2/omlc.h>
#include <oml2/oml_writer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _omlFilter;

typedef void* (*oml_filter_create)(
  OmlValueT   type,
  OmlValue*   result
);


/*! Function to set filter parameters.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_set)(
  struct _omlFilter* filter, //! pointer to filter instance
  const char* name,  //! Name of parameter
  OmlValue* value    //! Value of paramter
);


/*! Called for every sample.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_input)(
  struct _omlFilter* filter, //! pointer to filter instance
  OmlValue*          value   //! value of sample
);


/*! Called to request the current filter output. This is most likely
 * some function over the samples received since the last call.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_output)(
  struct _omlFilter* filter,
  struct _omlWriter* writer //! Write results of filter to this function
);

/*! Called once to write out the filters meta data
 * on its result arguments.
 *
 * Return 0 on success, -1 otherwise.
 */
typedef int (*oml_filter_meta)(
  struct _omlFilter* filter,
  int index_offset,  //! Index to use for first parameter
  char** namePtr,
  OmlValueT* type
);

typedef struct _omlFilterDef {
  //! Name of the filter output element
  const char* name;
  //! Type of this filter output element
  OmlValueT   type;
} OmlFilterDef;

/**
 * \struct
 * \brief
 */
typedef struct _omlFilter {
  //! Prefix for column name
  char name[64];

  //! Number of output value created
  int output_count;

  //! Set filter parameters
  oml_filter_set set;

  //! Process a new sample.
  oml_filter_input input;

  //! Calculate output, send it, and get ready for new sample period.
  oml_filter_output output;

  oml_filter_meta meta;

  void* instance_data;
  OmlFilterDef* definition;
  int index;
  OmlValueT input_type;

  OmlValue *result;

  struct _omlFilter* next;
} OmlFilter;

/*! Register a new type filter.
 *
 *  The filter type is created with the supplied create(), set(),
 *  input(), output() and meta() functions.  Its output conforms to
 *  the filter_def, if supplied.
 *
 *  If meta is NULL, the default meta function is used for instances
 *  of this filter, which inspects the filter_def to provide the
 *  filter meta information.
 *
 *  If filter_def is NULL, then meta must not be NULL; the meta
 *  function must be supplied to provide the filter meta information
 *  (for schema output).
 *
 *  If the set parameter is NULL, then a default no-op set function is
 *  supplied for instances of this filter type.
 *
 *  \param filter_name A string describing this type of filter, for use in create_filter().
 *  \param create The create function for this filter, which should initialize instance data.
 *  \param set The set function for this filter, which allows parameter setting.
 *  \param input The input function for this filter which adds new input samples to be filtered.
 *  \param output The output function for this filter, which puts a filtered result in the result vector.
 *  \param meta The meta output function for this filter, which is used to define the database schema for this filter.
 *  \param filter_def The output spec for this filter (name and type of each element of the output n-tuple).
 */
int
omlf_register_filter(const char* filter_name,
		     oml_filter_create create,
		     oml_filter_set set,
		     oml_filter_input input,
		     oml_filter_output output,
		     oml_filter_meta meta,
		     OmlFilterDef* filter_def);

#ifdef __cplusplus
}
#endif

#endif /*OML_FILTER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
