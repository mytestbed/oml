/*
 * Copyright 2007-2015 National ICT Australia (NICTA)
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
#include "mbuf.h"
#include "oml_utils.h"
#include "client.h"
#include "net_stream.h"

static ssize_t net_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length);
static ssize_t net_stream_write_immediate(OmlOutStream* hdl, uint8_t* buffer, size_t  length);
static int net_stream_close(OmlOutStream* hdl);

static int open_socket(OmlNetOutStream* self);

/** Create a new out stream for sending over the network
 *
 * *Don't forget to associate header data if you need it*
 *
 * \param transport string representing the protocol used to establish the connection (oml_strndup()'d locally)
 * \param hostname string representing the host to connect to (oml_strndup()'d locally)
 * \param service symbolic name or port number of the service to connect to (oml_strndup()'d locally)
 * \return a new OmlOutStream instance
 *
 * \see oml_strndup, out_stream_set_header_data
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
  self->os.dest = (char*)oml_strndup (mstring_buf(dest), mstring_len(dest));
  mstring_delete(dest);

  self->protocol = (char*)oml_strndup (transport, strlen (transport));
  self->host = (char*)oml_strndup (hostname, strlen (hostname));
  self->service = (char*)oml_strndup (service, strlen (service));

  logdebug("%s: Created OmlNetOutStream\n", self->os.dest);
  socket_set_non_blocking_mode(0);

  /* // Now see if we can connect to server */
  /* if (! open_socket(self)) { */
  /*   free(self); */
  /*   return NULL; */
  /* } */

  self->os.write = net_stream_write;
  self->os.write_immediate = net_stream_write_immediate;
  self->os.close = net_stream_close;
  return (OmlOutStream*)self;
}

/** Called to write into the socket
 * If the connection needs to be re-established, header is sent first, then buffer,
 * \copydetails oml_outs_write_f
 *
 * \see \see open_socket, net_stream_write_immediate
 */
static ssize_t
net_stream_write(OmlOutStream* hdl, uint8_t* buffer, size_t  length)
{
  OmlNetOutStream* self = (OmlNetOutStream*)hdl;
  size_t count;

  if (!self) { return -1; }

  /* Initialise the socket the first time */
  while (self->socket == NULL) {
    logdebug ("%s: Connecting to server\n", self->os.dest);
    if (!open_socket(self)) {
      logdebug("%s: Connection attempt failed\n", self->os.dest);
      return 0;
    }
  }


  out_stream_write_header(hdl);

  if(o_log_level_active(O_LOG_DEBUG4)) {
    char *out = to_octets(buffer, length);
    logdebug("%s: Sending data %s\n", self->os.dest, out);
    oml_free(out);
  }
  count = net_stream_write_immediate(hdl, buffer, length);
  return count;
}

/** Do the actual writing into the OComm Socket, with error handling
 * \copydetails oml_outs_write_immediate_f
 * \see write(3)
 */
static ssize_t
net_stream_write_immediate(OmlOutStream* outs, uint8_t* buffer, size_t  length)
{
  OmlNetOutStream *self = (OmlNetOutStream*) outs;

  if (!self) { return -1; }

  int result = socket_sendto(self->socket, (char*)buffer, length);

  if (result < 1 && socket_is_disconnected (self->socket)) {
    logwarn ("%s: Connection lost\n", self->os.dest);
    if (!result) {
      self->os.header_written = 0;

    } else { /* result < 0 */
      socket_free(self->socket);
      self->socket = NULL;
    }
  }
  return result;
}

/** Called to close the socket
 * \copydetails oml_outs_close_f
 */
static int
net_stream_close(OmlOutStream* stream)
{
  OmlNetOutStream* self = (OmlNetOutStream*)stream;

  if(!self) { return 0; }

  logdebug("%s: Destroying OmlNetOutStream at %p\n", self->os.dest, self);

  if (self->socket != 0) {
    socket_close(self->socket);
    self->socket = NULL;
  }
  oml_free(self->host);
  oml_free(self->protocol);
  oml_free(self->service);
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
    if ((sock = socket_tcp_out_new(self->os.dest, self->host, self->service)) == NULL) {
      return 0;
    }

    self->socket = sock;
    self->os.header_written = 0;
  } else {
    logerror("%s: Unsupported transport protocol '%s'\n", self->os.dest, self->protocol);
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

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
