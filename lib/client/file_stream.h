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
#include <stdio.h>

#include "oml2/oml_out_stream.h"
#include "mbuf.h"

typedef struct OmlFileOutStream {

  OmlOutStream os;                /**< OmlOutStream header */

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
