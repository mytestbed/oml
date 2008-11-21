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

#ifndef OML_WRITER_H_
#define OML_WRITER_H_

struct _omlWriter;

/*! Called for every result value.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int (*oml_writer_out)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlValue*  values,         //! type of sample
  int        value_count     //! size of above array
);

typedef struct _omlWriter {

  //! Writing a result.
  oml_writer_out out;

} OmlWriter;


extern OmlWriter*
file_writer_new(char* fileName);

extern OmlWriter*
net_writer_new(char* protocol, char* location);




#endif /* OML_WRITER_H */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
