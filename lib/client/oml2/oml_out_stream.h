/*
 * Copyright 2011-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_out_stream.h
 * \brief Abstract interface for output streams.
 *
 * Various writers, and particularly the BufferedWriter, use this interface to
 * output data into a stream (e.g., file or socket).
 */

#ifndef OML_OUT_STREAM_H_
#define OML_OUT_STREAM_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OmlOutStream;

/** Write a chunk into the lower level out stream
 *
 * \param outs OmlOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 * \param header pointer to the beginnig of header data to write
 * \param header_length length of header data to write
 *
 * XXX: Why are header and normal data separate?
 *
 * \return the number of sent bytes on success, -1 otherwise
 */
typedef size_t (*oml_outs_write_f)(struct OmlOutStream* outs, uint8_t* buffer, size_t length, uint8_t* header, size_t header_length
);

/** Close an OmlOutStream
 *
 * \param writer OmlOutStream to close
 * \return 0 on success, -1 otherwise
 */
typedef int (*oml_outs_close_f)(struct OmlOutStream* writer);

/** A low-level output stream */
typedef struct OmlOutStream {
  /** Pointer to a function in charge of writing into the stream \see oml_outs_write_f */
  oml_outs_write_f write;
  /** Pointer to a function in charge of closing the stream \see oml_outs_close_f */
  oml_outs_close_f close;
} OmlOutStream;

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
