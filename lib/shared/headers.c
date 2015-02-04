/*
 * Copyright 2010-2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file headers.c
 * \brief Implements OMSP header-parsing functions.
 *
 * XXX: Headers code should be factored from text/binary writers, and moved here (#1101).
 */
/** \page omsp OML Measurement Stream Protocol (OMSP)
 *
 * The OML Measurement Stream Protocol is used to describe and transport
 * measurement tuples between Injection Points and Processing/Collection
 * Points. All data injected in a Measurement Point (MP) (with \ref
 * omlc_inject) is timestamped and sent to the destination as a
 * Measurement Stream (MS).
 *
 * The liboml2 provides an \ref api "API"
 * allowing to generate this MSs using this protocol and send them to
 * a remote host, or store them in a local file.
 *
 * Upon connection to a collection point, a set of \ref omspheaders "headers"
 * is first sent, describing the injection point (protocol version, name,
 * application, local timestamp), along with the \ref omspschema "schemata of
 * the transported MSs".
 *
 * Once done, timestamped measurement data is serialised, either using a
 * \ref omspbin "binary encoding", or a \ref omsptext "text encoding".
 *
 * \section Generalities
 *
 * There are 5 versions of the OML protocol.
 *
 * - OMSP V1 was the initial protocol, inherited from OML (version 1!);
 * - OMSP V2 introduced more precise types (<a
 *   href="http://git.mytestbed.net/?p=oml.git;a=shortlog;h=28daef3f">commit:28daef3f</a>),
 *   and was released with OML 2.4.0;
 * - OMSP V3 introduced changes to the binary protocol to support blobs and,
 *   incidentally, longer marshalled packets (<a
 *   href="http://git.mytestbed.net/?p=oml.git;a=shortlog;h=6d8f0597">commit:6d8f0597</a>),
 *   and was released with OML 2.5.0;
 * - OMSP V4 was introduced with OML 2.10.0; its main additions are the support
 *   for the definition of new Measurement Points (and Measurement Stream
 *   Schemas) at any time, and the ability to inject metadata.
 * - OMSP V5 which is the current, most recent and implemented version (since
 *   OML 2.11); its main advantages are the support for vectors, and the
 *   introduction of a DOUBLE64_T IEEE 754 binary64 for more precision in
 *   representing doubles in binary mode (vectors only),
 *   - OMSPv5 was extended with an `encapsulation` header, sent before the protocol
 *     version, to inform the server of an extra encapsulation (e.g., gzip) being used
 *     (since OML 2.12).
 *
 * The protocol is loosely modelled after HTTP. The client first start
 * with a few \ref omspheaders "textual headers", then switches into
 * either the \ref omsptext "text" or \ref omspbin "binary" protocol for
 * the serialisation of tuples, following a previously advertised \ref
 * omspschema "schema". Both modes include contextual information with
 * each tuple. There is no feedback communication from the server.
 *
 * The client first opens a connection to an OML server and sends a
 * header followed by a sequence of measurement tuples. The header
 * consists of a sequence of key/value pairs representing parameters
 * valid for the whole connection, each terminated by a new line. The
 * headers are also used to declare the schema following which
 * measurement tuples will be streamed. The end of the header section is
 * identified by an empty line. Each measurement tuple is then
 * serialised following the mode selected in the headers. For the
 * \ref omsptext "text mode", this is a
 * series of newline-terminated strings containing tab-separated
 * text-based serialisations of each element, while the
 * \ref omspbin "binary mode" encodes
 * the data following a specific marshalling. Clients end the session by
 * simply closing the TCP connection.
 *
 * \subsection kv Key/Value Parameters
 *
 * The connection is initially configured through setting the value of a few
 * property, using a key/value model. The properties (and their keys) are the
 * following.
 *
 * - `encapsulation`: (optional, before any of the keys below) type of encapsulation
 *   (e.g, gzip) for the data, this is version-independent (only supported with
 *   \ref oml2-server "oml2-server">=2.12); currently supported encapsulations are:
 *     - gzip: the following data is gzip-compressed (\ref oml2-server "oml2-server">=2.12);
 * - `protocol`: (always first) OMSP version, as specified in this document. The
 *     \ref oml2-server "oml2-server" currently supports 1--5;
 * - `domain` (`experiment-id` in V<4): string identifying the experimental
 *     domain (should match `/[-_A-Za-z0-9]+/`);
 * - `start-time` (`start_time` in V<4; not before `domain`): local UNIX time in
 *   seconds taken at the time the header is being sent (see
 *     <a href="http://linux.die.net/man/3/gettimeofday">gettimeofday(3)</a>).
 *     This key must not appear before `domain`; \ref timestamps "the server
 *     uses this information to rebase timestamps within its own timeline";
 * - `sender-id`: string identifying the source of this stream (should match
 *   `/[_A-Za-z0-9]+/`);
 * - `app-name`: string identifying the application producing the
 *     a measurements (should match `/[_A-Za-z0-9]+/`), in the storage backend, this
 *     may be used to identify specific measurements collections (e.g., tables in SQL);
 * - `schema`: describes the \ref omspschema "schema of each measurement stream".
 * - `content`: encoding of forthcoming tuples, can be either `binary` for the
 *     \ref omspbin "binary protocol" or `text` for the \ref omsptext "text
 *     protocol".
 *
 * A blank line marks the end of the headers and informs the receiver that
 * samples follow (\ref omspbinary "binary" or \ref omsptext "text" depending
 * on the `content` advertised).  * These parameters can only be set as
 * part of the \ref omspheaders "headers" and are not valid once the server
 * expects serialised measurements (V<4).
 *
 * Since V>=4, key/value metadata can be sent along with tuples using the \ref
 * schema0 "schema 0", the rest of the key/value parameters presented here
 * are all invalid in schema 0, and will be rejected by the server, _except_
 * for key `schema` itself, allowing to (re)define schemata (*XXX* not including
 * schema 0?).
 *
 * \subsection timestamping Time-stamping and book-keeping
 * Regardless of the mode (\ref omspbin "binary" or \ref omsptext "text"), each
 * measurement tuple is prefixed with some per-sample metadata.
 *
 * Prior to serialising tuples according to their schema, three elements are
 * inserted.
 * - `timestamp`: a *double* timestamp in seconds relative to the
 *     `start-time` sent in the headers,
 *     \ref timestamps "the server uses this information to rebase timestamps within
 *     its own timeline";
 * - `stream_id:` an *integer* (marshalled specifically as a `uint8_t` in
 *      \ref omspbin "binary mode") indicating which previously defined schema
 *      this tuple follows;
 * - `seq_no:` an *int32* monotonically increasing sequence number in the
 *     context of this measurement stream.
 *
 * The order of these fields varies depending on the mode (\ref omsptext "text"
 * or \ref omspbin "binary").
 */
/**
 * \page omsp
 * \section omspheaders OMSP Headers
 *
 * The headers are text-based, and used to transfer the
 * \ref kv "key/value  parameters of the experiment".  All of them have to appear
 * exactly once, in the order they were introduced in \ref kv "that section".
 * The only exception is the `schema` key which needs to appear once for
 * every measurement stream carried by the connection. In a similar fashion to
 * HTTP, the end of header data is indicated by a blank line.
 *
 * \subsection omspheadersexample Examples
 *
 * Protocol version 3 and below can only define MSs at the time headers are sent.
 *
 *     protocol: 3
 *     experiment-id: exv3
 *     start_time: 1281591603
 *     sender-id: senderv3
 *     app-name: generator
 *     schema: 1 generator_sin label:string phase:double value:double
 *     schema: 2 generator_lin label:string counter:long
 *     content: text
 *     
 *
 * Protocol version 4 added the possibility to declare MP after headers are
 * sent, though the use of the newly introduced _schema0_ metadata stream.
 * Here, MS `generator_d_sin` created by sending a tuple with subject `.` (the
 * experiment root), key `schema`, and a value representing the schema of
 * the new MS, including its schema number and stream name, using the text
 * protocol.
 *
 *     protocol: 4
 *     domain: exv4
 *     start-time: 1281591603
 *     sender-id: senderv4
 *     app-name: generator
 *     schema: 0 _experiment_metadata subject:string key:string value:string
 *     schema: 1 generator_d_lin label:string seq_no:uint32
 *     content: text
 *     
 *     0.163925        0       1       .       schema  2 generator_d_sin label:string phase:double value:double
 *
 * Since OML 2.12 (OMSP>=v5), data can be compressed in-flight by encapsulating it
 * into gzip.
 *
 *     encapsulation: gzip
 *     [gzip-compressed content]
 *
 * Inflating the rest of  content (not including the first line) with, say, `gunzip -c`, shows the data.
 *
 *     protocol: 5
 *     domain: test
 *     start-time: 1420517574
 *     sender-id: generator
 *     app-name: generator
 *     schema: 0 _experiment_metadata subject:string key:string value:string
 *     schema: 1 _client_instrumentation measurements_injected:uint32 measurements_dropped:uint32 bytes_allocated:uint64 bytes_freed:uint64 bytes_in_use:uint64 bytes_max:uint64
 *     schema: 2 generator_d_lin label:string seq_no:uint32
 *     schema: 3 generator_d_sin label:string phase:double value:double
 *     content: text
 *     
 *     0.227902        0       1       .       appname generator
 *
 */

#include <stdlib.h>
#include <string.h>

#include "headers.h"
#include "mem.h"
#include "string_utils.h"

static const struct {
  const char *name;
  size_t namelen;
  enum HeaderTag tag;
} header_map [] = {
  { "protocol",      8,  H_PROTOCOL },
  { "domain",        6,  H_DOMAIN },
  { "experiment-id", 13, H_DOMAIN },
  { "content",       7,  H_CONTENT },
  { "app-name",      8,  H_APP_NAME },
  { "schema",        6,  H_SCHEMA },
  { "sender-id",     9,  H_SENDER_ID },
  { "start-time",    10, H_START_TIME },
  { "start_time",    10, H_START_TIME }, /* This one will be deprecated at some point */
  { NULL, 0, H_NONE }
};

/** Convert a string header name into the tag for that header.
 *
 * The +str+ should be a null terminated string.  The parameter +n+
 * sets the bounds of the string to be considered for extracting the
 * tag.  All characters from the start to the n-th are considered part
 * of the name, so any trailing whitespace should be after the n-th
 * character.
 *
 * The function takes +str+ and compares it against the list of known
 * headers.  If it finds a match, it converts the header to its
 * corresponding tag; otherwise it returns H_NONE.
 *
 * \param str -- string name to convert.
 * \param n -- the number of bytes of str to consider
 * \return -- the tag.
 */
enum HeaderTag
tag_from_string (const char *str, size_t n)
{
  int i = 0;
  char first;
  char second;

  if (str == NULL || n <= 2)
    return H_NONE;

  first = *str;
  second = *(str+1);

  /* Premature optimization for fun and profit */
  for (; header_map[i].name != NULL; i++) {
    char h_first  = header_map[i].name[0];
    char h_second = header_map[i].name[1];
    size_t h_len = header_map[i].namelen;

    if (n < h_len)
      continue;

    if (first != 's') {
      /* We can fail on mismatched first char if it's not 's' */
      if ((first == h_first) &&
          (strncmp (str+1, header_map[i].name+1, n - 1) == 0))
        return header_map[i].tag;
    } else {
      /* If 's', we can fast-fail on the second char */
      if (first == h_first && second == h_second &&
          (strncmp (str+2, header_map[i].name+2, n - 2) == 0)) {
        return header_map[i].tag;
      }
    }
  }
  return H_NONE;
}

const char *
tag_to_string (enum HeaderTag tag)
{
  int i = 0;
  for (i = 0; header_map[i].name != NULL; i++)
    if (header_map[i].tag == tag)
      return header_map[i].name;
  return NULL;
}

/**
 *  @brief Parse a protocol header line into a struct header.
 *
 *  Each header consists of a tag, followed by a colon, followed by a
 *  value, terminated by a new line character '\n'.  This function
 *  parses such a header into the tag, converted from a string into an
 *  enum HeaderTag, and the value, which is kept as a string.  They
 *  are returned as a newly allocated struct header object; the caller
 *  owns the memory and should deallocated it when it is no longer
 *  needed, using header_free().
 *
 *  Whitespace is not permitted at the start of the string, but is
 *  permitted before and after the colon ':'.  This space is stripped
 *  from the value, but trailing space is not.
 *
 *  The string str should be null-terminated, but can be longer than
 *  +n+ characters.  Characters after the n-th will be ignored.  The
 *  +value+ part of the returned +struct header+ will not have any
 *  trailing whitespace removed.
 *
 *  If the tag part is not recognized, NULL is returned.  If memory
 *  can't be allocated for the +struct header+, then NULL is returned
 *  instead.  If there is no colon ':' in the first +n+ characters of
 *  +str+, then NULL is returned.
 *
 *  @param str Input string to parse
 *  @param n Number of characters of str to parse.
 *  @return The header, or NULL if the header could not be parsed.
 */
struct header*
header_from_string (const char *str, size_t n)
{
  if (!str) return NULL;

  const char *p = str;
  const char *q = memchr (p, ':', n);
  if (!q)
    return NULL;

  /* Strip trailing whitespace on the header name */
  p = find_white(p);

  int namelen;
  if (p < q)
    namelen = p - str;
  else
    namelen = q - str;

  enum HeaderTag tag = tag_from_string (str, namelen);
  if (tag == H_NONE)
    return NULL;

  /* Skip the ':' and strip leading whitespace on header value */
  if (*q)
    q = skip_white (++q);

  char * value = NULL;
  size_t valuelen = n - (q - str);
  struct header *header = NULL;

  if ((int)valuelen > 0) {
    value = oml_strndup (q, valuelen);
    if (!value)
      return NULL;

    header = oml_malloc (sizeof (struct header));
    if (!header) {
      oml_free (value);
      return NULL;
    }
    header->tag = tag;
    header->value = value;
    return header;
  }

  return NULL;
}

/**
 *  Free the memory of a +struct header+ object.
 */
void
header_free (struct header *header)
{
  if (header) {
    if (header->value)
      oml_free (header->value);
    oml_free (header);
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
