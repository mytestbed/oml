/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_writer.h
 * \brief Abstract interface for writers.
 *
 * A writer is in charge of serialising the OML samples (using either the text
 * or binary protocol \ref binprotocol), and outputting them somewhere (network
 * or file).
 *
 * \see OmlTextWriter, OmlBinWriter
 */

#ifndef OML_WRITER_H_
#define OML_WRITER_H_

#include <oml2/omlc.h>

#define OML_PROTOCOL_VERSION 4

struct OmlWriter;

/** Function called whenever some header metadata needs to be added.
 * \param writer pointer to OmlWriter instance
 * \param string character string to add to headers
 * \return 1 on success, 0 on error
 */
typedef int (*oml_writer_meta)(struct OmlWriter* writer, char* string);

/** Function called to finalise meta header
 *
 * This function should essentially add the 'content: [binary|text]' line to
 * the already written headers
 *
 * \param writer pointer to OmlWriter instance
 * \return 1 on success, 0 on error
 *
 * \see oml_writer_meta
 */
typedef int (*oml_writer_header_done)(struct OmlWriter* writer);

/** Function called to prepare a new sample.
 * \param writer pointer to OmlWriter instance
 * \param ms OmlMStream for which the sample is
 * \param now current timestamp
 *
 * \return 1 on success, 0 on error
 *
 * \see oml_writer_out, oml_writer_row_end
 *
 */
typedef int (*oml_writer_row_start)(struct OmlWriter* writer, OmlMStream* ms, double now);

/** Function called after all items in a tuple have been sent
 * \param writer pointer to OmlWriter instance
 * \param ms OmlMStream for which the sample is
 *
 * \return 1 on success, 0 on error
 *
 * \see oml_writer_row_start, oml_writer_out
 */
typedef int (*oml_writer_row_end)(struct OmlWriter* writer, OmlMStream* ms);

/** Function called for every result value in a measurement tuple (sample)
 * \param writer pointer to OmlWriter instance
 * \param values array of OmlValue to write out
 * \param values_count size of the values array
 *
 * \return the number of values written on success, or 0 otherwise
 */
typedef int (*oml_writer_out)( struct OmlWriter* writer, OmlValue* values, int values_count);

/** Function called to close the writer and free its allocated objects.
 *
 * This function is designed so it can be used in a while loop to clean up the
 * entire linked list:
 *
 *   while( (w=w->close(w)) );
 *
 * \param writer pointer to the writer to close and destroy
 * \return a pointer to the next writer in the list (writer->next, which can be NULL)
 */
typedef struct OmlWriter* (*oml_writer_close)(struct OmlWriter* writer);

/** An instance of an OML Writer */
typedef struct OmlWriter {

  /** Pointer to function writing header information \see oml_writer_meta */
  oml_writer_meta meta;
  /** Pointer to function finalising header information \see oml_writer_header_done */
  oml_writer_header_done header_done;

  /** Pointer to function preparing a new sample \see oml_writer_row_start */
  oml_writer_row_start row_start;
  /** Pointer to function finalising a new sample \see oml_writer_row_end */
  oml_writer_row_end row_end;

  /** Pointer to function outputting values into the stream \see oml_writer_out */
  oml_writer_out out;

  /** Pointer to function closing the stream \see oml_writer_close */
  oml_writer_close close;

  /** Pointer to the next OmlWriter in the linked list */
  struct OmlWriter* next;
} OmlWriter;

/** Stream encoding type, for use with create_writer */
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
