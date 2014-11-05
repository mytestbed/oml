/*
 * Copyright 2011-2015 National ICT Australia (NICTA)
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

#include <unistd.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct OmlOutStream;

/** Interface to write a chunk into the lower level out stream
 *
 * \warning This function should take care of properly sending the headers, if
 * needed, prior to writing out the data.
 *
 * \param outs OmlOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 *
 * \return the number of sent bytes on success, -1 otherwise
 *
 * \see OmlOutStream::write, out_stream_write, OmlOutStream::header_written, out_stream_write_header,
 */
typedef ssize_t (*oml_outs_write_f)(struct OmlOutStream* outs, uint8_t* buffer, size_t length);

/** Interface to immediately write a chunk into the lower level out stream
 *
 * \param outs OmlOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 *
 * \return the number of sent bytes on success, -1 otherwise
 *
 * \see OmlOutStream::write_immediate, out_stream_write_immediate
 */
typedef ssize_t (*oml_outs_write_immediate_f)(struct OmlOutStream* outs, uint8_t* buffer, size_t length);

/** Interface to close an OmlOutStream
 *
 * \warning This function should note call oml_free() on outs, as
 * oml_stream_close takes care of it.
 *
 * \param outs OmlOutStream to close
 * \return 0 on success, -1 otherwise
 *
 * \see OmlOutStream::close, oml_stream_close
 */
typedef int (*oml_outs_close_f)(struct OmlOutStream* outs);

/** Interface to set the object containing header data
 *
 * \param outs OmlOutStream to manipulate
 * \param header_data pointer to the opaque data structure containing the headers (can be NULL)
 * \return a pointer to the old structure, cast as a (void*)
 *
 * \see OmlOutStream::write, out_stream_write, out_stream_set_header_data, OmlOutStream::header_data
 */
typedef void* (*oml_outs_set_header_data_f)(struct OmlOutStream* outs, void* header_data);

/** Create an OmlOutStream for the specified URI
 *
 * \param uri	collection URI
 * \return a pointer to the newly allocated OmlOutStream, or NULL on error
 *
 * \see create_writer
 */
struct OmlOutStream* create_out_stream(const char *uri);

/** Write data into stream
 *
 * \param outs OmlOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 *
 * \return the number of sent bytes on success, -1 otherwise
 *
 * \see oml_outs_write_f
 */
ssize_t out_stream_write(struct OmlOutStream* self, uint8_t* buffer, size_t length);

/** Create an OmlOutStream for the specified URI
 *
 * \param uri 	collection URI
 * \return a pointer to the newly allocated OmlOutStream, or NULL on error
 *
 * \see create_writer
 */
struct OmlOutStream* create_out_stream(const char *uri);

/** Immediately write data into stream
 *
 * \param outs OmlOutStream to write into
 * \param buffer pointer to the beginning of the data to write
 * \param length length of data to read from buffer
 *
 * \return the number of sent bytes on success, -1 otherwise
 *
 * \see oml_outs_write_immediate_f
 */
ssize_t out_stream_write_immediate(struct OmlOutStream* self, uint8_t* buffer, size_t length);

/** Close OmlOutStream
 *
 * \param writer OmlOutStream to close
 * \return 0 on success, -1 otherwise
 *
 * \see oml_outs_close_f
 */
int out_stream_close(struct OmlOutStream* self);

/** Set the pointer to the opaque data structure containing the headers
 *
 * \param outs OmlOutStream to manipulate
 * \param header_data pointer to the opaque data structure containing the headers (can be NULL)
 * \return a pointer to the old structure, cast as a (void*)
 * \see OmlOutStream::header_data
 */
void* out_stream_set_header_data(struct OmlOutStream* outs, void* header_data);

/** Write header information if not already done, and record this fact
 *
 * This function call writefp to write the header data if self->header_written is 0.
 *
 * \param outs OmlOutStream to write into
 *
 * \return the number of sent bytes on success (0 if no header was written), -1 otherwise
 * \see oml_outs_write_immediate_f, OmlOutStream::header_written
 */
ssize_t out_stream_write_header(struct OmlOutStream* outs);

/** A low-level output stream */
typedef struct OmlOutStream {
  oml_outs_write_f write;                     /**< Pointer to a function in charge of writing into the stream \see out_stream_write */
  oml_outs_write_immediate_f write_immediate; /**< Pointer to a function in charge of immediately writing data into the underlying stream \see out_stream_write_immediate */
  oml_outs_close_f close;                     /**< Pointer to a function in charge of closing the stream \see out_stream_close */
  oml_outs_set_header_data_f set_header_data; /**< Pointer to a function in charge of setting header data \see out_stream_set_header_data */

  char* dest; /**< Description of this output stream, usually overriden by a URI or filename */

  void* header_data;                          /**< Pointer to data structure containing the header data to be transmitted at the beginning; not owned by the OmlOutStream \see out_stream_set_header_data, OmlOutStream::set_header_data */
  int   header_written;                       /**< True if header has been written to the stream */
} OmlOutStream;

extern OmlOutStream *file_stream_new(const char *file);

int file_stream_set_buffered(OmlOutStream* hdl, int buffered);
int file_stream_get_buffered(OmlOutStream* hdl);

/* from net_stream.c */

extern OmlOutStream *net_stream_new(const char *transport, const char *hostname, const char *port);

/* from zlib_stream.c */

extern OmlOutStream *zlib_stream_new(OmlOutStream *out);

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
