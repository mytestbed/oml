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
/*!\file builtin.h
  \brief Lists all the builtin filters.
*/
#ifndef OML_FILTER_FACTORY_H_
#define OML_FILTER_FACTORY_H_

#include <oml2/oml_filter.h>

/* Builtin filter registration functions */
void omlf_register_filter_average (void);

const char* next_filter_name(void);
void register_builtin_filters (void);

/*! Create an instance of a filter of type 'fname'.
 */
OmlFilter*
create_filter(
    const char* filter_type,
    const char* instance_name,
    OmlValueT   type,
    int         index
);

#endif /* OML_FILTER_FACTORY_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
