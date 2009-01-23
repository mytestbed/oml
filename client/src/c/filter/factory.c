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

#include <malloc.h>
#include <ocomm/o_log.h>
#include "filter/factory.h"

/*! Create an instance of a filter of type 'fname'.
 */
void
register_filter(
    const char*       filterName,
    oml_filter_create create_func
) {
  FilterReg* fr = (FilterReg*)malloc(sizeof(FilterReg));
  fr->filterName = filterName;
  fr->create_func = create_func;
  fr->next = firstFilter;
  firstFilter = fr;
}

/*! Create an instance of a filter of type 'fname'.
 */
OmlFilter*
create_filter(
    const char* filterName,
    const char* paramName,
    OmlValueT   type,
    int         index
) {
  FilterReg* fr = firstFilter;
  for(; fr != NULL; fr = fr->next) {
    if (strcmp(filterName, fr->filterName) == 0) break;
  }
  if (fr == NULL) {
    o_log(O_LOG_ERROR, "Unknown filter '%s'.\n", filterName);
    return NULL;  // nothing found
  }

  OmlFilter* f = fr->create_func(paramName, type, index);
  return f;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
