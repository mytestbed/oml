/*
 * Copyright 2007-2014 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_util.c
 * \brief Miscellaneous functions and macros used elsewhere in testing.
 */
#ifndef UTIL_H__
#define UTIL_H__

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"

#define LENGTH(a) ((sizeof(a))/(sizeof((a)[0])))

#define MAKEOMLCMDLINE(argc, argv, collect)   \
  const char* argv[] = {                      \
    __FUNCTION__,                             \
    "--oml-id", __FUNCTION__,                 \
    "--oml-domain", __FILE__,                 \
    "--oml-collect", collect,                 \
    "--oml-log-level", "2"};                  \
  int argc = 9;

typedef struct
{
  int length; /* Length of the test vector */
  OmlValueT type; /* Type of all elements of test vector */
  OmlValueU* vector; /* Vector of values */
} TestVector;

typedef struct
{
  int count; /* Number of vectors in inputs[] and outputs[] */
  TestVector** inputs;
  TestVector** outputs;
} TestData;

OmlValueU*
make_vector (void* v, OmlValueT type, int n);

TestVector*
make_test_vector (void* v, OmlValueT type, int n);

int
vector_type_check (OmlValue* values, OmlValueT type, int n);

int
vector_values_check (OmlValue* values, OmlValueU* expected, OmlValueT type, int n);

void
run_filter_test (TestData* test_data, OmlFilter* f);

#endif // UTIL_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
