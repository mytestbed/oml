/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
/*!\file net_stream.c
  \brief Implements an out stream which sends measurement tuples over the network
*/

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <signal.h>

#include "oml2/omlc.h"
#include "oml2/oml_out_stream.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "mem.h"
#include "oml_util.h"
#include "client.h"

#define REATTEMP_INTERVAL 10    //! Seconds to wait before attempting to reach server again

typedef struct _omlNetOutStream {

  oml_outs_write_f write;
  oml_outs_close_f close;

 //----------------------------
  FILE* f;                      /* File to write result to */

  Socket*    socket;

  const char*       protocol;
  const char*       host;
  int         port;

  char  storage;                /* ALWAYS last. Holds parsed server URI string and extends
                                 * beyond this structure, see +_new+ function. */
  int   header_written;         // True if header has been written to file
} OmlNetOutStream;

static int open_socket(OmlNetOutStream* self);
static size_t net_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length);
static int net_stream_close(OmlOutStream* hdl);
static size_t socket_write(OmlNetOutStream* self, uint8_t* buffer, size_t  length);

/**
 * \fn OmlOutStream* net_stream_new(char* serverURI)
 * \brief Create a new out stream for sending over the network
 * \param serverURI URI of communicating peer
 * \return a new +OmlOutStream+ instance
 */
OmlOutStream*
net_stream_new(const char *transport, const char *hostname, const char *port)
{
  assert(transport != NULL && hostname != NULL && port != NULL);
  OmlNetOutStream* self = (OmlNetOutStream *)malloc(sizeof(OmlNetOutStream));
  memset(self, 0, sizeof(OmlNetOutStream));

  //  memcpy(&self->storage, serverURI, uriSize);
  self->protocol = (const char*)xstrndup (transport, strlen (transport));
  self->host = (const char*)xstrndup (hostname, strlen (hostname));
  self->port = atoi (port);

  logdebug("Net_stream: connecting to host %s://%s:%d\n",
          self->protocol, self->host, self->port);
  socket_set_non_blocking_mode(0);

  /* // Now see if we can connect to server */
  /* if (! open_socket(self)) { */
  /*   free(self); */
  /*   return NULL; */
  /* } */

  self->write = net_stream_write;
  self->close = net_stream_close;
  return (OmlOutStream*)self;
}

/**
 * \brief Called to close the socket
 * \param writer the netwriter to close the socket in
 * \return 0
 */
static int
net_stream_close(
  OmlOutStream* stream
) {
  OmlNetOutStream* self = (OmlNetOutStream*)stream;

  if (self->socket != 0) {
    socket_close(self->socket);
    self->socket = NULL;
  }
  return 0;
}

static void
termination_handler(int signum)
{
  // SIGPIPE is handled by disabling the writer that caused it.
  if (signum == SIGPIPE) {
    logwarn("Net_stream: caught SIGPIPE\n");
  }
}

static int open_socket(OmlNetOutStream* self)
{
  if(self->socket) {
    socket_free(self->socket);
    self->socket = NULL;
  }
  if (strcmp(self->protocol, "tcp") == 0) {
    Socket* sock;
    if ((sock = socket_tcp_out_new("sock", (char*)self->host, self->port)) == NULL) {
      return 0;
    }

    self->socket = sock;
    self->header_written = 0;
  } else {
    logerror("Net_stream: unsupported transport protocol '%s'\n", self->protocol);
    return 0;
  }

  // Catching SIGPIPE signals if the associated socket is closed
  // TODO: Not exactly sure if this is completely right for all application situations.
  struct sigaction new_action, old_action;

  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (SIGPIPE, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGPIPE, &new_action, NULL);

  return 1;
}

static size_t
net_stream_write(
  OmlOutStream* hdl,
  uint8_t* buffer,
  size_t  length,
  uint8_t* header,
  size_t   header_length
) {
  OmlNetOutStream* self = (OmlNetOutStream*)hdl;

  while (self->socket == NULL) {
    loginfo ("Net_stream: attempting to connect to server at %s://%s:%d\n",
             self->protocol, self->host, self->port);
    if (!open_socket(self)) {
      logwarn("Net_stream: connection attempt failed, sleeping for %ds\n", REATTEMP_INTERVAL);
      sleep(REATTEMP_INTERVAL);
    } else {
      logdebug("Net_stream: connection to %s://%s:%d successful\n",
             self->protocol, self->host, self->port);
    }
  }

  size_t count;
  if (! self->header_written) {
    if ((count = socket_write(self, header, header_length)) < header_length) {
      // TODO: This is not completely right as we end up rewriting the same header
      // if we can only partially write it. At this stage we think this is too hard
      // to deal with and we assume it doesn't happen.
      if (count > 0) {
        // PANIC
        logerror("Net_stream: only wrote parts of the header; this might cause problem later on\n");
      }
      return 0;
    }
    self->header_written = 1;
  }
  count = socket_write(self, buffer, length);
  return count;
}

static size_t
socket_write(
  OmlNetOutStream* self,
  uint8_t* buffer,
  size_t  length
) {
  int result = socket_sendto(self->socket, (char*)buffer, length);

  if (result == -1 && socket_is_disconnected (self->socket)) {
    logwarn ("Net_stream: connection to server at %s://%s:%d was lost\n",
             self->protocol, self->host, self->port);
    self->socket = NULL;      // Server closed the connection
  }
  return result;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
