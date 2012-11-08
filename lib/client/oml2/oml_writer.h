/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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

#define OML_PROTOCOL_VERSION 3

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

/*! Called after all items in tuple have been sent (via +out+).
 *
 */
typedef int
(*oml_writer_row_end)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlMStream* ms
);

/*! Called for every result value (column).
 *
 * Return 0 on success, -1 otherwise
 */
typedef int
(*oml_writer_out)(
  struct _omlWriter* writer, //! pointer to writer instance
  OmlValue*  values,         //! array of column values
  int        values_count    //! size of above array
);

/** Called to close the writer and free its allocated objects.
 *
 * This function is designed so it can be used in a while loop to clean up the
 * entire linked list:
 *
 *   while( (w=w->close(w)) );
 *
 * \param writer pointer to the writer to close and destroy
 * \return a pointer to the next writer in the list (writer->next, which can be NULL)
 */
typedef struct _omlWriter* (*oml_writer_close)(struct _omlWriter* writer);

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

  //! Writing the results from a single filter.
  oml_writer_out out;

  oml_writer_close close;

  struct _omlWriter* next;
} OmlWriter;

enum StreamEncoding {
  SE_None, // Not explicitly specified by the user
  SE_Text,
  SE_Binary
};

#ifdef __cplusplus
extern "C" {
#endif

OmlWriter* create_writer(const char* uri, enum StreamEncoding encoding);

#ifdef __cplusplus
}
#endif

#endif /* OML_WRITER_H */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
