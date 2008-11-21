/*
 * Copyright 2007-2008 National ICT Australia (NICTA), Australia
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
/*!\file builtin.h
  \brief Lists all the builtin filters.
*/
#ifndef OML_FILTER_FACTORY_H_
#define OML_FILTER_FACTORY_H_

#include <oml2/oml_filter.h>

extern OmlFilter*
omlf_first_new(
  const char* name,
  OmlValueT  type,
  int        index
);

extern OmlFilter*
omlf_average_new(
  const char* name,
  OmlValueT   type,
  int         index
);

typedef struct _filterReg {
  const char*         filterName;
  oml_filter_create   create_func;
  struct _filterReg*  next;
} FilterReg;

FilterReg f1 = {"first", omlf_first_new, NULL};
FilterReg f2 = {"avg", omlf_average_new, &f1};

FilterReg* firstFilter = &f2;

#endif /* OML_FILTER_FACTORY_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
