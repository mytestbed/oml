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

#ifndef OML_WRITER_H_
#define OML_WRITER_H_

#include <oml2/omlc.h>

#define OML_PROTOCOL_VERSION 1

struct _omlWriter;


/*! Called for every line in the meta header.
 */
typedef int
(*oml_writer_meta)(
  struct _omlWriter* writer, //! pointer to writer instance
  char* string
);

/*! Called to finalize meta header
 */
typedef int
(*oml_writer_header_done)(
  struct _omlWriter* writer //! pointer to writer instance
);



/*! Called before calling 'out' for every result
 *  in this stream.
 */
typedef int
(*oml_writer_row_start)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlMStream* ms,
  double now                 //! timestamp
);

/*! Called after calling 'out' for every result
 *  in this stream.
 */
typedef int
(*oml_writer_row_end)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlMStream* ms
);

/*! Called for every result value.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int
(*oml_writer_out)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlValue*  values,         //! type of sample
  int        value_count     //! size of above array
);

/*! Called to close the writer.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int
(*oml_writer_close)(
  struct _omlWriter* writer //! pointer to writer instance
);

/**
 * \struct
 * \brief
 */
typedef struct _omlWriter {

  oml_writer_meta meta;
  oml_writer_header_done header_done;

  //! Called before and after writing individual
  // filter results with 'out'
  oml_writer_row_start row_start;
  oml_writer_row_end row_end;

  //! Writing the results froma single filter.
  oml_writer_out out;

  oml_writer_close close;

  struct _omlWriter* next;
} OmlWriter;

#ifdef __cplusplus
extern "C" {
#endif

OmlWriter*
create_writer(
    char* serverUri
);

#ifdef __cplusplus
}
#endif


#endif /* OML_WRITER_H */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
