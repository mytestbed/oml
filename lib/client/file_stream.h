/*
 * Copyright 2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file file_stream.h
 * \brief Interface for the file OmlOutStream.
 * \see OmlOutStream
 */
#include "mbuf.h"
#include <stdio.h>
#include "oml2/oml_out_stream.h"

typedef struct OmlFileOutStream {

  /*
   * Fields from OmlOutStream interface
   */

  /** \see OmlOutStream::write, oml_outs_write_f */
  oml_outs_write_f write;
  /** \see OmlOutStream::close, oml_outs_close_f */
  oml_outs_close_f close;

  /** \see OmlOutStream::header_data, oml_outs_set_header_data */
  char *dest;

  /** \see OmlOutStream::header_data */
  MBuffer* header_data;

  /** \see OmlOutStream::header_written */
  int   header_written;

  /*
   * Fields specific to the OmlFileOutStream
   */

  FILE* f;                      /**< File pointer into which to write result to */

} OmlFileOutStream;

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
