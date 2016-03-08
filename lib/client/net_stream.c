/*
 * Copyright 2007-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file net_stream.c
 * \brief An OmlOutStream implementation that writer that sends measurement tuples over the network.
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

  /*
   * Fields specific to the OmlNetOutStream
   */

  /** OComm Socket in which the data is writte */
  Socket*    socket;

  /** Protocol used to establish the connection */
  char*       protocol;
  /** Host to connect to */
  char*       host;
  /** Service to connect to */
  char*       service;

  /** Old storage, no longer used, kept for compatibility */
  char  storage;

  /** True if header has been written to the stream \see open_socket*/
  int   header_written;

} OmlNetOutStream;

static int open_socket(OmlNetOutStream* self);
static size_t net_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length);
static int net_stream_close(OmlOutStream* hdl);
static ssize_t socket_write(OmlNetOutStream* self, uint8_t* buffer, size_t  length);

/** Create a new out stream for sending over the network
 * \param transport string representing the protocol used to establish the connection (oml_strndup()'d locally)
 * \param hostname string representing the host to connect to (oml_strndup()'d locally)
 * \param service symbolic name or port number of the service to connect to (oml_strndup()'d locally)
 * \return a new OmlOutStream instance
 *
 * \see oml_strndup
 */
OmlOutStream*
net_stream_new(const char *transport, const char *hostname, const char *service)
{
  MString *dest;
  assert(transport != NULL && hostname != NULL && service != NULL);
  OmlNetOutStream* self = (OmlNetOutStream *)oml_malloc(sizeof(OmlNetOutStream));
  memset(self, 0, sizeof(OmlNetOutStream));

  dest = mstring_create();
  mstring_sprintf(dest, "%s:%s:%s", transport, hostname, service);
  self->dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  self->protocol = (char*)oml_strndup (transport, strlen (transport));
  self->host = (char*)oml_strndup (hostname, strlen (hostname));
  self->service = (char*)oml_strndup (service, strlen (service));

  logdebug("%s: Created OmlNetOutStream\n", self->dest);
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

/** Called to close the socket
 * \see oml_outs_close_f
 */
static int
net_stream_close(OmlOutStream* stream)
{
  OmlNetOutStream* self = (OmlNetOutStream*)stream;

  if (self->socket != 0) {
    socket_free(self->socket);
    self->socket = NULL;
  }
  logdebug("%s: Destroying OmlNetOutStream at %p\n", self->dest, self);
  oml_free(self->dest);
  oml_free(self->host);
  oml_free(self->protocol);
  oml_free(self->service);
  oml_free(self);
  return 0;
}

/** Signal handler
 * \param signum received signal number
 */
static void
signal_handler(int signum)
{
  // SIGPIPE is handled by disabling the writer that caused it.
  if (signum == SIGPIPE) {
    logwarn("OmlNetOutStream: caught SIGPIPE\n");
  }
}

/** Open an OComm Socket with the parameters of this OmlNetOutStream
 *
 * This function tries to register a signal handler to catch closed sockets
 * (SIGPIPE), but sometimes doesn't (XXX: the conditions need to be clarified).
 *
 * \param self OmlNetOutStream containing the parameters
 * \return 1 on success, 0 on error
 *
 * \see signal_handler
 */
static int
open_socket(OmlNetOutStream* self)
{
  struct sigaction new_action, old_action;

  if(self->socket) {
    socket_free(self->socket);
    self->socket = NULL;
  }
  if (strcmp(self->protocol, "tcp") == 0) {
    Socket* sock;
    if ((sock = socket_tcp_out_new(self->dest, self->host, self->service)) == NULL) {
      return 0;
    }

    self->socket = sock;
    self->header_written = 0;
  } else {
    logerror("%s: Unsupported transport protocol '%s'\n", self->dest, self->protocol);
    return 0;
  }

  // Catching SIGPIPE signals if the associated socket is closed
  // TODO: Not exactly sure if this is completely right for all application situations.
  new_action.sa_handler =signal_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (SIGPIPE, NULL, &old_action);
  /* XXX: Shouldn't we set up the handler ONLY if the old one is SIG_IGN? */
  if (old_action.sa_handler != SIG_IGN) {
    sigaction (SIGPIPE, &new_action, NULL);
  }

  return 1;
}

/** Called to write into the socket
 * \see oml_outs_write_f
 *
 * If the connection needs to be re-established, header is sent first, then buffer,
 *
 * \see \see open_socket, socket_write
 */
static size_t
net_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length, uint8_t* header, size_t  header_length)
{
  OmlNetOutStream* self = (OmlNetOutStream*)hdl;

  /* Initialise the socket the first time */
  while (self->socket == NULL) {
    logdebug ("%s: Connecting to server\n", self->dest);
    if (!open_socket(self)) {
      logdebug("%s: Connection attempt failed\n", self->dest);
      return 0;
    }
  }

  /* If the underlying socket as registered a disconnection, it will reconnect on its own
   * however, we need to check it to make sure we send the headers before anything else */
  if(socket_is_disconnected(self->socket)) {
    self->header_written = 0;
  }

  size_t count;
  if (! self->header_written) {
    if(o_log_level_active(O_LOG_DEBUG4)) {
      char *out = to_octets(header, header_length);
      logdebug("%s: Sending header %s\n", self->dest, out);
      oml_free(out);
    }
    if ((count = socket_write(self, header, header_length)) < header_length) {
      // TODO: This is not completely right as we end up rewriting the same header
      // if we can only partially write it. At this stage we think this is too hard
      // to deal with and we assume it doesn't happen.
      if (count > 0) {
        // PANIC
        logwarn("%s: Only wrote parts of the header; this might cause problem later on\n", self->dest);
      }
      return 0;
    }
    self->header_written = 1;
  }
  if(o_log_level_active(O_LOG_DEBUG4)) {
    char *out = to_octets(buffer, length);
    logdebug("%s: Sending data %s\n", self->dest, out);
    oml_free(out);
  }
  count = socket_write(self, buffer, length);
  return count;
}

/** Do the actual writing into the OComm Socket, with error handling
 * \param self OmlNetOutStream through which the data should be written
 * \param buffer data to write
 * \param length length of the data to write
 *
 * \return the size of data written, or -1 on error
 *
 * \see write(3)
 */
static ssize_t
socket_write(OmlNetOutStream* self, uint8_t* buffer, size_t  length)
{
  int result = socket_sendto(self->socket, (char*)buffer, length);

  if (result == -1 && socket_is_disconnected (self->socket)) {
    logwarn ("%s: Connection lost\n", self->dest);
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
