/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_filter.h
 * \brief Abstract interface for filters.
 */
#ifndef OML_FILTER_H_
#define OML_FILTER_H_

#include <oml2/omlc.h>
#include <oml2/oml_writer.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OmlFilter;

/** Filter instance creation function.
 *
 * If this filter needs to dynamically allocate instance date, this instance
 * MUST be allocated using xmalloc().
 *
 * \param type OmlValueT of the sample stream that this instance will process
 * \param result pointer to a vector of OmlValue (of size output_count) where the oml_filter_output() will write
 * \return xmalloc()ed instance data
 * \see OmlFilter, oml_filter_output, xmalloc
 */
typedef void* (*oml_filter_create)(OmlValueT type, OmlValue* result);

/** Optional unction to set filter parameters at runtime.
 *
 * \param filter pointer to OmlFilter instance
 * \param name name of the parameter
 * \param value new value of the parameter, as a pointer to an OmlValue
 * \return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_set)(struct OmlFilter* filter, const char* name, OmlValue* value);

/** Function called whenever a new sample is to be delivered to the filter.
 *
 * \param filter pointer to OmlFilter instance
 * \param value new sample, as a pointer to an OmlValue
 * \return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_input)(struct OmlFilter* filter, OmlValue* value);


/** Function called whenever aggregated output is requested from the filter.
 * some function over the samples received since the last call.
 *
 * \param filter pointer to OmlFilter instance
 * \param writer pointor to an OmlWriter in charge of outputting the aggregated sample
 * \return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_output)(struct OmlFilter* filter, struct OmlWriter* writer);

/** Function called whenever aggregated output has been written to all writers,
 * and can then be cleared.
 *
 * \param filter pointer to OmlFilter instance
 * \return 0 on success, -1 otherwise
 */
typedef int (*oml_filter_newwindow)(struct OmlFilter* filter);

/** Optional function returning metainformation for complex outputs.
 *
 * XXX: This function will probably go at some point soon. Don't use it.
 *
 * If a filter returns more than one output value, of possibly different types,
 * they each need to be named and properly typed. This function allows to query
 * information about a specific output, identified by its index.  It is however
 * preferable to do so through the filter_def argument of the
 * omlf_register_filter() function.
 *
 * \param filter pointer to OmlFilter instance
 * \param index_offset index in result array to query for meta information
 * \param[out] namePtr name of the output value at index index_offset (XXX: must be statically allocated)
 * \param[out] type OmlTypeT of the output value at index index_offset
 * \return 0 on success, -1 otherwise
 * \see omlf_register_filter, OmlFilterDef
 */
typedef int (*oml_filter_meta)(struct OmlFilter* filter, int index_offset, char** namePtr, OmlValueT* type);

/** Definition of a filter's output element. */
typedef struct OmlFilterDef {
  /* Name of the filter output element */
  const char* name;

  /* Type of this filter output element */
  OmlValueT   type;

} OmlFilterDef;

/** Definition of a filter instance.
 *
 * A filter is defined by its output schema and its methods.
 *
 * \see OmlFilterDef, oml_filter_create, oml_filter_set, oml_filter_input, oml_filter_output
 */
typedef struct OmlFilter {
  /** Name of the filter (suffix for the aggregated output's name) */
  char name[64];

  /** Number of output value (allocated by the factory) */
  int output_count;

  /** Function to set filter parameters (optional) \see oml_filter_set */
  oml_filter_set set;

  /** Function to process a new sample \see oml_filter_input */
  oml_filter_input input;
  /** Function to send the current output to a writer \see oml_filter_output */
  oml_filter_output output;

  /** Function returning describing complex outputs (optional) \see oml_filter_meta */
  oml_filter_meta meta;

  /** Instance data, \see xmalloc, xfree */
  void* instance_data;

  /** Definition of the filter's output schema as an array of OmlFilterDef */
  OmlFilterDef* definition;

  /** Index of the field of the MP processed by this filter */
  int index;
  /** Type of the processed field */
  OmlValueT input_type;

  /** Array of results where the filter is expected to write its ouput, allocated by the factory */
  OmlValue *result;

  /** Filters are stored in a linked list for a given MP */
  struct OmlFilter* next;

  /** Function to start a new sampling period \see oml_filter_newwindow */
  oml_filter_newwindow newwindow; /* XXX: To be pulled up after output on the next ABI version change */
} OmlFilter;

/** Register a new filter type.
 *
 *  The filter type is created with the supplied oml_filter_create(), oml_filter_set(),
 *  oml_filter_input(), oml_filter_output() functions.  Its output conforms to
 *  the filter_def, if supplied, or oml_filter_meta() otherwise.
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
 *  \param filter_name string describing this type of filter, for use in create_filter()
 *  \param create oml_filter_create() function which should initialize instance data
 *  \param set oml_filter_set() function which allows parameter setting (can be NULL)
 *  \param input oml_filter_input() function which adds new input samples to be filtered
 *  \param output oml_filter_output() function which puts a filtered result in the result vector
 *  \param newwindow oml_filter_newwindow() output function which is used to start a new sampling window
 *  \param meta oml_filter_meta() output function which is used to define the output schema (can be NULL is filter_def is not)
 *  \param filter_def output spec (name and type of each element of the output n-tuple; can be NULL if meta is not)
 *  \see oml_filter_create, oml_filter_set, oml_filter_input, oml_filter_output, oml_filter_meta, OmlFilterDef
 */
int
omlf_register_filter(const char* filter_name, oml_filter_create create, oml_filter_set set, oml_filter_input input,
    oml_filter_output output, oml_filter_newwindow newwindow, oml_filter_meta meta, OmlFilterDef* filter_def);

#ifdef __cplusplus
}
#endif

#endif /*OML_FILTER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
