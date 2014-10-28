/*
 * Copyright 2007-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_writer.c
 * \brief Generic functions for all OmlWriter implementations.
 */
#include "oml2/oml_writer.h"
#include "oml2/oml_out_stream.h"
#include "oml_utils.h"

/** Create an OmlWriter for the specified URI
 *
 * \param uri collection URI
 * \param encoding StreamEncoding to use for the output, either SE_Text or SE_Binary
 * \return a pointer to the new OmlWriter, or NULL on error
 *
 * \see create_out_stream
 */
OmlWriter*
create_writer(const char* uri, enum StreamEncoding encoding)
{
  OmlOutStream* out_stream = NULL;
  OmlWriter* writer = NULL;

  out_stream = create_out_stream(uri);
  if (out_stream == NULL) {
    logerror ("Failed to create stream for URI %s\n", uri);
    return NULL;
  }

  if (SE_None == encoding) {
    if (oml_uri_is_file(oml_uri_type(uri))) {
      encoding = SE_Text; /* default encoding */

    } else {
      encoding = SE_Binary; /* default encoding */
    }
  }

  switch (encoding) {
  case SE_Text:   writer = text_writer_new (out_stream); break;
  case SE_Binary: writer = bin_writer_new (out_stream); break;
  case SE_None:
    logerror ("No encoding specified (this should never happen -- please report this as an OML bug)\n");
    // should cleanup streams
    return NULL;
  }
  if (writer == NULL) {
    logerror ("Failed to create writer for encoding '%s'.\n", encoding == SE_Binary ? "binary" : "text");
    return NULL;
  }

  writer->next = NULL;
  return writer;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
