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

/** Create an OmlOutStream from the components of a parsed URI
 *
 * Scheme, and either host/port or path are mandatory.
 *
 * \param scheme string containing the selected scheme
 * \param host string containing the host
 * \param port string containing the port
 * \param path string containing the path
 * \return a pointer to the newly allocated OmlOutStream, or NULL on error
 *
 * \see create_out_stream
 */
static OmlOutStream*
create_out_stream_from_components(const char *scheme, const char *hostname, const char *port, const char *filepath)
{
  OmlOutStream *os = NULL;

  assert(scheme);
  assert((hostname && port) || filepath);

  OmlURIType uri_type;
  uri_type = oml_uri_type(scheme);

  switch (uri_type) {
  case OML_URI_FILE:
  case OML_URI_FILE_FLUSH:
    os = file_stream_new(filepath);
    if(OML_URI_FILE_FLUSH == uri_type) {
      file_stream_set_buffered(os, 0);
    }
    break;

  case OML_URI_TCP:
    os = net_stream_new(scheme, hostname, port);
    break;

  case OML_URI_UDP:
  case OML_URI_UNKNOWN:
  default:
    logwarn ("URI scheme %s is not supported\n", scheme);
    break;
  }

  if (os == NULL) {
    logerror ("Failed to create stream for URI %s:%s[%s]%s%s%s\n",
        scheme, hostname?"//":"", hostname, port?":":"", port, filepath);
    return NULL;
  }

  return os;
}

/** Create an OmlOutStream for the specified URI */
OmlOutStream*
create_out_stream(const char *uri)
{
  const char *scheme = NULL;
  const char *hostname = NULL;
  const char *port = NULL;
  const char *filepath = NULL;
  OmlOutStream *os = NULL;

  if (uri == NULL || strlen(uri) < 1) {
    logerror ("Missing or invalid collection URI definition (e.g., --oml-collect)\n");
    return NULL;
  }

  if (parse_uri (uri, &scheme, &hostname, &port, &filepath) == -1) {
    logerror ("Error parsing collection URI '%s'; failed to create stream for this destination\n",
              uri);
    if (scheme) oml_free ((void*)scheme);
    if (hostname) oml_free ((void*)hostname);
    if (port) oml_free ((void*)port);
    if (filepath) oml_free ((void*)filepath);
    return NULL;
  }

  os = create_out_stream_from_components(scheme, hostname, port, filepath);

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
      if (((size_t)count > 0) && ((size_t)count < header_length)) {
        logwarn("%s: Only wrote parts of the header; this might cause problem later on\n", self->dest);
      }
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
