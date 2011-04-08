/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
/*!\file net_writer.c
  \brief Implements a writer which sends results over the network
*/

#include <log.h>
#include <ocomm/o_socket.h>
#include <stdlib.h>
#include <stdlib.h>
#include <string.h>

#include "oml2/omlc.h"
#include "client.h"
#include "oml2/oml_writer.h"
#include "oml2/oml_filter.h"
#include <marshal.h>
#include <mbuf.h>

#define DEF_PROTOCOL "tcp"
#define DEF_PORT 3003

/**
 * \struct OmlNetWriter
 * \brief a structure that send the data to the server
 */

typedef struct _omlNetWriter {

  oml_writer_meta meta;
  oml_writer_header_done header_done;

  //! Called before and after writing individual
  // filter results with 'out'
  oml_writer_row_start row_start;
  oml_writer_row_end row_end;

  //! Writing a result.
  oml_writer_out out;

  oml_writer_close close;

  OmlWriter* next;

  /**********************/

  int        is_enabled;
  int        first_row;
  Socket*    socket;
  MBuffer* mbuf;

  int        stream_count; // used to give each stream an ID

  const char* protocol;
  const char* host;
  int         port;

} OmlNetWriter;

static int write_meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);

static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int out(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static int close(OmlWriter* writer);

/**
 * \brief Create a new +OmlWriter+
 * \param protocol the transport protocol
 * \param location the host and the port number of the server
 * \return a new +OmlWriter+
 */
OmlWriter*
net_writer_new(char* protocol, char* location)
{
  OmlNetWriter* self = (OmlNetWriter *)malloc(sizeof(OmlNetWriter));
  memset(self, 0, sizeof(OmlNetWriter));

  self->mbuf = mbuf_create ();

  char* host;
  char* p = location;
  if (! (strcmp(protocol, "tcp") == 0 || strcmp(protocol, "udp") == 0)) {
    host = protocol;
    protocol = DEF_PROTOCOL;
  } else {
    while (*p != '\0' && *p == '/') p++; // skip leading "//"
    host = p;
    while (*p != '\0' && *p != ':') p++;
    if (*p != '\0') *(p++) = '\0';
  }
  char* portS = p;
  while (*p != '\0' && *p != ':') p++;
  if (*p != '\0') *(p++) = '\0';
  int port = (*portS == '\0') ? DEF_PORT : atoi(portS);

  //  char* localBind = p;  // This would be for multicast, ignore right now

  self->protocol = protocol;
  self->port = port;
  self->host = host;

  loginfo ("Net proto: <%s> host: <%s> port: <%d>\n", protocol, host, port);

  socket_set_non_blocking_mode(0);
  if (protocol == NULL || strcmp(protocol, "tcp") == 0) {
    if ((self->socket = socket_tcp_out_new("sock", host, port)) == NULL) {
      free (self);
      return NULL;
    }
  }
  self->meta = write_meta;
  self->header_done = header_done;

  self->row_start = row_start;
  self->row_end = row_end;
  self->out = out;
  self->close = close;

  self->first_row = 1;
  self->is_enabled = 1;

  return (OmlWriter*)self;
}

/**
 * \brief Definition of the write_meta function of the oml net writer
 * \param writer the net writer that will send the data to the server
 * \param str the string to send
 * \return 1 if the socket is not open, 0 if successful
 */
static int
write_meta(OmlWriter* writer, char* str)
{
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;
  if (!self->is_enabled) return 1;

  int len = strlen (str);

  /* +2 for '\n' and '\0' */
  char *s = (char*)malloc ((strlen (str) + 2)* sizeof (char));

  strncpy (s, str, len + 1);
  s[len] = '\n';
  s[len + 1] = '\0';
  len = strlen(s);

  int result = socket_sendto(self->socket, s, len);

  if (result == -1 && socket_is_disconnected (self->socket))
    {
      logwarn("Connection to server at %s://%s:%d was lost\n",
             self->protocol, self->host, self->port);
      self->is_enabled = 0;       // Server closed the connection
    }

  free (s);
  return (result == len);
}

/**
 * \brief finish the writing of the first information
 * \param writer the writer that write this information
 * \return
 */
static int
header_done(OmlWriter* writer)
{
  write_meta(writer, "content: binary");
  write_meta(writer, "");

  return 0;
}

/**
 * \brief marshal and then transfer the values
 * \param writer pointer to writer instance
 * \param values type of sample
 * \param value_count size of above array
 * \return 0 if sucessful 1 otherwise
 */
static int
out(OmlWriter* writer, OmlValue* values, int value_count)
{
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;
  if (!self->is_enabled) return 1;

  int cnt = marshal_values(self->mbuf, values, value_count);
  return cnt == value_count;
}

/**
 * \brief before sending datastore information about the time and the stream
 * \param writer the netwriter to send data
 * \param ms the stream to store the measruement from
 * \param now a timestamp that represensent the current time
 * \return 1
 */
int
row_start(OmlWriter* writer, OmlMStream* ms, double now)
{
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;
  if (!self->is_enabled) return 1;

  marshal_measurements(self->mbuf, ms->index, ms->seq_no, now);
  return 1;
}

/**
 * \brief send the data after finalysing the data structure
 * \param writer the net writer that send the measurements
 * \param ms the stream of measurmenent
 * \return 1
 */
int
row_end(OmlWriter* writer, OmlMStream* ms)
{
  (void)ms;
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;
  if (!self->is_enabled) return 1;

  marshal_finalize(self->mbuf);
  int len = mbuf_message_length (self->mbuf);
  logdebug("Sending message of size '%d'\n", len);
  int result = socket_sendto(self->socket, (char*)mbuf_message (self->mbuf), len);

  if (result == -1 && socket_is_disconnected (self->socket))
    {
      logwarn("Connection to server at %s://%s:%d was lost\n",
             self->protocol, self->host, self->port);
      self->is_enabled = 0;       // Server closed the connection
    }

  mbuf_clear (self->mbuf);
  return 1;
}

/**
 * \brief Called to close the socket
 * \param writer the netwriter to close the socket in
 * \return 0
 */
static int
close(OmlWriter* writer)
{
  OmlNetWriter* self = (OmlNetWriter*)writer;

  if (self->socket != 0) {
    socket_close(self->socket);
    self->socket = NULL;
  }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
