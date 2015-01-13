/*
 * Copyright 2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file zlib_stream.h
 * \brief Interface for the Zlib OmlOutStream.
 * \see OmlOutStream
 */
#include <zlib.h>
#include <sys/time.h>

#include "oml2/oml_out_stream.h"
#include "zlib_utils.h"
#include "mbuf.h"

/** Encapsulation header sent uncompressed into the underlying stream */
#define ZENCAPHEADER ENCAPHEADER " gzip\n"

typedef struct OmlZlibOutStream {

  OmlOutStream os;              /**< OmlOutStream header */

  /** \see OmlOutStream::header_data */
  MBuffer* header_data;

  /** \see OmlOutStream::header_written */
  int   header_written;

  /*
   * Fields specific to the OmlZlibOutStream
   */

  OmlOutStream* outs;           /**< OmlOutStream into which to write result */

  int   chunk_size;             /**< Zlib buffer size */
  int	zlevel;                 /**< Compression level for Zlib */
  z_stream  strm;               /**< Zlib compressed stream */
  uint8_t*  in;                 /**< input buffer */
  uint8_t*  in_length;          /**< input buffer length*/
  uint8_t*  in_wr_offset;       /**< input buffer write offset */

  uint8_t*  out;                /**< output buffer */
  uint8_t*  out_length;         /**< output buffer length*/
  uint8_t*  out_wr_offset;      /**< output buffer write offset */

  time_t    last_flush;         /**< last time the Zlib stream was force-flushed */
  int       nwrites;            /**< number of times OmlZlibOutStream::write has been called since last fush */

  MBuffer*  encapheader;        /**< MBuffer containing headers to be sent uncompressed on the underlying stream */

} OmlZlibOutStream;

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
