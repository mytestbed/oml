/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file net_stream.c
 * \brief Implements an OmlOutStream stream which sends measurement tuples over the network
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

/** OmlOutStream writing out to an OComm Socket */
typedef struct OmlNetOutStream {

  /*
   * Fields from OmlOutStream interface
   */

  /** \see OmlOutStream::write, oml_outs_write_f */
  oml_outs_write_f write;
  /** \see OmlOutStream::close, oml_outs_close_f */
  oml_outs_close_f close;

  /*
   * Fields specific to the OmlNetOutStream
   */

  /** File to write result to
   * XXX: This is a Network implementation, what is this field used for?
   * Not removing to avoid altering the layout of the structure
   */
  FILE* f;

  /** OComm Socket in which the data is writte */
  Socket*    socket;

  /** Protocol used to establish the connection */
  const char*       protocol;
  /** Host to connect to */
  const char*       host;
  /** Port to connect to */
  int         port;

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
 * \param transport string representing the protocol used to establish the connection
 * \param hostname string representing the host to connect to
 * \param port string representing the port to connect to
 * \return a new OmlOutStream instance
 */
OmlOutStream*
net_stream_new(const char *transport, const char *hostname, const char *port)
{
  assert(transport != NULL && hostname != NULL && port != NULL);
  OmlNetOutStream* self = (OmlNetOutStream *)malloc(sizeof(OmlNetOutStream));
  memset(self, 0, sizeof(OmlNetOutStream));

  self->protocol = (const char*)xstrndup (transport, strlen (transport));
  self->host = (const char*)xstrndup (hostname, strlen (hostname));
  self->port = atoi (port);

  logdebug("OmlNetOutStream: connecting to host %s://%s:%d\n",
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

/** Called to close the socket
 * \see oml_outs_close_f
 */
static int
net_stream_close(OmlOutStream* stream)
{
  OmlNetOutStream* self = (OmlNetOutStream*)stream;

  if (self->socket != 0) {
    socket_close(self->socket);
    self->socket = NULL;
  }
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
    if ((sock = socket_tcp_out_new("sock", (char*)self->host, self->port)) == NULL) {
      return 0;
    }

    self->socket = sock;
    self->header_written = 0;
  } else {
    logerror("OmlNetOutStream: unsupported transport protocol '%s'\n", self->protocol);
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

  while (self->socket == NULL) {
    loginfo ("OmlNetOutStream: attempting to connect to server at %s://%s:%d\n",
             self->protocol, self->host, self->port);
    if (!open_socket(self)) {
      logwarn("OmlNetOutStream: connection attempt failed, sleeping for %ds\n", REATTEMP_INTERVAL);
      sleep(REATTEMP_INTERVAL);
    } else {
      logdebug("OmlNetOutStream: connection to %s://%s:%d successful\n",
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
        logerror("OmlNetOutStream: only wrote parts of the header; this might cause problem later on\n");
      }
      return 0;
    }
    self->header_written = 1;
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
    logwarn ("OmlNetOutStream: connection to server at %s://%s:%d was lost\n",
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
