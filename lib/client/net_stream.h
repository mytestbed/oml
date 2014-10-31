/*
 * Copyright 2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file net_stream.h
 * \brief Interface for the network OmlOutStream.
 * \see OmlOutStream, OSocket
 */
#include "ocomm/o_socket.h"
#include "oml2/oml_out_stream.h"
#include "mbuf.h"

/** OmlOutStream writing out to an OComm Socket */
typedef struct OmlNetOutStream {

  /*
   * Fields from OmlOutStream interface
   */

  /** \see OmlOutStream::write, oml_outs_write_f */
  oml_outs_write_f write;
  /** \see OmlOutStream::close, oml_outs_close_f */
  oml_outs_close_f close;

  /** \see OmlOutStream::dest */
  char *dest;

  /** \see OmlOutStream::header_data, oml_outs_set_header_data */
  MBuffer* header_data;

  /** \see OmlOutStream::header_written */
  int   header_written;

  /*
   * Fields specific to the OmlNetOutStream
   */

  /** OComm Socket in which the data is written */
  Socket*    socket;

  /** Protocol used to establish the connection */
  char*       protocol;
  /** Host to connect to */
  char*       host;
  /** Service to connect to */
  char*       service;

} OmlNetOutStream;

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
