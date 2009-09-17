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
#ifndef UTIL_H__
#define UTIL_H__

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"

#define LENGTH(a) ((sizeof(a))/(sizeof((a)[0])))

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
 tab-width: 4
 indent-tabs-mode: nil
*/
