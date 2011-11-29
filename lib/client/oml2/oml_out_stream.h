/*
 * Copyright 2011 National ICT Australia (NICTA), Australia
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

#ifndef OML_OUT_STREAM_H_
#define OML_OUT_STREAM_H_

#include <stdint.h>

struct _omlOutStream;

/*! Called to write a chunk to the lower level out stream
 *
 * Return number of sent bytes on success, -1 otherwise
 */
typedef size_t (*oml_outs_write_f)(
  struct _omlOutStream* outs,
  uint8_t* buffer,
  size_t  length
);

/*! Called to close the stream.
 *
 * Return 0 on success, -1 otherwise
 */
typedef int
(*oml_outs_close_f)(
  struct _omlOutStream* writer //! pointer to writer instance
);

typedef struct _omlOutStream {
  oml_outs_write_f write;
  oml_outs_close_f close;
} OmlOutStream;

#ifdef __cplusplus
extern "C" {
#endif

OmlOutStream*
create_stream(
  char* serverUri
);

#ifdef __cplusplus
}
#endif

#endif /* OML_OUT_STREAM_H */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
