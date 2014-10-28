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
#include "oml_utils.h"

/** Create an OmlOutStream for the specified URI */
OmlOutStream*
create_out_stream(const char *uri)
{
  const char *scheme= NULL;
  const char *hostname = NULL;
  const char *port = NULL;
  const char *filepath = NULL;
  OmlOutStream* os = NULL;
  OmlURIType uri_type;

  if (uri == NULL || strlen(uri) < 1) {
    logerror ("Missing or invalid collection URI definition (e.g., --oml-collect)\n");
    return NULL;
  }

  uri_type = oml_uri_type(uri);

  if (parse_uri (uri, &scheme, &hostname, &port, &filepath) == -1) {
    logerror ("Error parsing collection URI '%s'; failed to create stream for this destination\n",
              uri);
    if (scheme) oml_free ((void*)scheme);
    if (hostname) oml_free ((void*)hostname);
    if (port) oml_free ((void*)port);
    if (filepath) oml_free ((void*)filepath);
    return NULL;
  }

  if (oml_uri_is_file(uri_type)) {
    os = file_stream_new(filepath);
    if(OML_URI_FILE_FLUSH == uri_type) {
      file_stream_set_buffered(os, 0);
    }
  } else {
    os = net_stream_new(scheme, hostname, port);
  }
  if (os == NULL) {
    logerror ("Failed to create stream for URI %s\n", uri);
    return NULL;
  }

  oml_free ((void*)scheme);
  oml_free ((void*)hostname);
  oml_free ((void*)port);
  oml_free ((void*)filepath);

  return os;
}

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
