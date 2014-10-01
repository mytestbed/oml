/*
 * Copyright 2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_out_stream.c
 * \brief Helper functions for all OmlOutStreams
 * \see OmlOutStream
 */
#include <assert.h>
#include <string.h>
#include <errno.h>

#include "oml2/omlc.h"
#include "oml2/oml_out_stream.h"

/** Write header information if not already done, and record this fact */
ssize_t
out_stream_write_header(OmlOutStream* self, oml_outs_write_f_immediate writefp, uint8_t* header, size_t header_length)
{
  ssize_t count=0;

  assert(self);
  assert(writefp);
  assert(header || !header_length);

  if (! self->header_written) {
    if ((count = writefp(self, header, header_length)) < 0) {
      logerror("%s: Error writing header: %s\n", self->dest, strerror(errno));
      return -1;

    } else {
      if ((size_t)count < header_length)
        logwarn("%s: Only wrote parts of the header; this might cause problem later on\n", self->dest);
      self->header_written = 1;
    }
  }

  return count;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
