/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file socket.c
 * \brief A thin abstraction layer that manages sockets and provides some additional state management functions.
 */
#include <assert.h>
#include <sys/timeb.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "mem.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE !TRUE
#endif

#define MAX_SOCKET_INSTANCES 100

static int nonblocking_mode = 1;

//! Data-structure, to store communication related parameters and information.
typedef struct SocketInt {

  //! Name used for debugging
  char* name;

  o_socket_sendto sendto;
  o_get_sockfd get_sockfd;

  //! Socket file descriptor of the communication socket created to send/receive.
  int sockfd;

  //! True for TCP, false for UDP
  int is_tcp;

  //! IP address of the multicast socket/channel.
  char* dest;

  //! Remote port connected to
  char* service;

  //! Local port used
  int localport;

  //! Name of the interface (eth0/eth1) to bind to
  char* iface;

  //! Hold the server address.
  struct sockaddr_in servAddr;

  //! Inet address of the local machine/host.
  struct in_addr iaddr;

  //! Hold the multicast channel information.
  struct ip_mreq imreq;

  //! Called when new client connects (TCP servers only)
  o_so_connect_callback connect_callback;

  //! opaque argument to callback (TCP servers only)
  void* connect_handle;

  //! Local memory for debug name
  char nameBuf[64];

  int is_disconnected; /** < True if detected a SIGPIPE or ECONNREFUSED on a sendto() */

  struct addrinfo *results, /** < Results from getaddrinfo to iterate over */
                  *rp;      /** < Current iterator over getaddrinfo results */

} SocketInt;


int
socket_set_non_blocking_mode(
  int flag
) {
  return nonblocking_mode = flag;
}

int
socket_get_non_blocking_mode()

{
  return nonblocking_mode;
}

int
socket_is_disconnected (
 Socket* socket
) {
  return ((SocketInt*)socket)->is_disconnected;
}

int
socket_is_listening (
 Socket* socket
) {
  return (NULL != ((SocketInt*)socket)->connect_callback);
}

/** Create a new instance of the Socket object (SocketInt).
 *
 * The sendto and get_sockfd are set to the default socket_sendto() and
 * socket_get_sockfd() functions, respectively.
 *
 * \param name name of the object, used for debugging
 * \return a pointer to the SocketInt object
 */
static SocketInt*
socket_initialize(const char* name) {
  SocketInt* self = (SocketInt *)oml_malloc(sizeof(SocketInt));
  memset(self, 0, sizeof(SocketInt));

  self->name = self->nameBuf;
  strncpy(self->name, name != NULL ? name : "UNKNOWN", sizeof(self->nameBuf));

  self->sendto = socket_sendto;
  self->get_sockfd = socket_get_sockfd;

  return self;
}

/** Terminate the Socket and free its allocated memory.
 *
 * If the socket is still open, it will be closed.
 *
 * \param socket Socket to free
 * \see socket_new, socket_close
 */
void socket_free (Socket* socket) {
  SocketInt* self = (SocketInt *)socket;
  socket_close(socket);
  if (self->dest) { oml_free(self->dest); }
  if (self->service) { oml_free(self->service); }
  if (self->results) { freeaddrinfo(self->results); }
  oml_free (self);
}

/** Create an unbound Socket object.
 *
 * \param name name of the object, used for debugging
 * \param is_tcp  true if TCP, false for UDP XXX: This should be more generic
 * \return a pointer to the SocketInt object, cast as a Socket, with a newly opened system socket
 * \see socket_free, socket_close
 */
Socket* socket_new(const char* name, int is_tcp) {
  SocketInt* self = socket_initialize(name);
  self->is_tcp = is_tcp; /* XXX: replace is_tcp with a more meaningful variable to contain that */
  self->is_disconnected = 1;
  return (Socket*)self;
}


/** Create an IP Socket object bound to ADDR and PORT.
 *
 * This function binds the newly-created socket, but doesn't listen on it just yet.
 *
 * \param name name of the object, used for debugging
 * \param port port used
 * \param is_tcp true if TCP, false for UDP XXX: This should be more generic
 * \return a pointer to the SocketInt object, cast as a Socket
 */
Socket* socket_in_new(
  const char* name,
  int port,
  int is_tcp
) {
  SocketInt* self;

  if ((self = (SocketInt*)socket_new(name, is_tcp)) == NULL) {
    return NULL;
  } else if(0 > (self->sockfd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP))) {
    o_log(O_LOG_DEBUG, "socket(%s): Could not create listening socket for AF %d: %s\n",
        self->name, PF_INET, strerror(errno));
    socket_free((Socket*)self);
    return NULL;
  }

  self->servAddr.sin_family = AF_INET;
  self->servAddr.sin_port = htons(port);
  self->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(self->sockfd, (struct sockaddr *)&self->servAddr,
        sizeof(struct sockaddr_in)) < 0) {
    o_log(O_LOG_ERROR, "socket(%s): Error binding socket to interface: %s\n",
        name, strerror(errno));
    socket_free((Socket*)self);
    return NULL;
  }

  self->localport = ntohs(self->servAddr.sin_port);
  o_log(O_LOG_DEBUG, "socket(%s): Socket bound to port: %d\n", name, self->localport);

  instances = self;
  return (Socket*)self;
}

/** Connect the socket to remote peer.
 *
 * If addr is NULL, assume the servAddr is already populated, and ignore port.
 *
 * \param self OComm socket to use
 * \return 1 on success, 0 on error
 */
static int
s_connect(SocketInt* self) {
  int hostlen=40,  /* IPv6: (8*4 + 7) + 1*/
      servlen=7; /* ndigits(65536) + 1 */
  char host[hostlen], serv[servlen];
  struct addrinfo hints;
  *host = 0;
  *serv = 0;
  memset(&hints, 0, sizeof(struct addrinfo));
  /* XXX: This should be pulled up when we support UDP and/or multicast */
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP; /* FIXME: We should check self->is_tcp */

  if (!self->dest || !self->service) {
    o_log(O_LOG_ERROR, "socket(%s): destination or service missing. Is this an outgoing socket?\n",
        self->name);
    return 0;
  }

  if (!self->rp) {
    o_log(O_LOG_DEBUG, "socket(%s): Resolving %s:%s\n",
        self->name, self->dest, self->service);
    if (getaddrinfo(self->dest, self->service, &hints, &self->results)) {
      o_log(O_LOG_ERROR, "socket(%s): Error resolving %s:%s: %s\n",
          self->name, self->dest, self->service, strerror(errno));
      return 0;
    }
    self->rp = self->results;
  }

  for (; self->rp != NULL; self->rp = self->rp->ai_next) {
    if (!getnameinfo(self->rp->ai_addr, self->rp->ai_addrlen,
          host, hostlen, serv, servlen,
          NI_NUMERICHOST|NI_NUMERICSERV)) {
      o_log(O_LOG_DEBUG, "socket(%s): Connecting to [%s]:%s (AF %d, proto %d)\n",
          self->name, host, serv, self->rp->ai_family, self->rp->ai_protocol);
    } else {
      o_log(O_LOG_DEBUG, "socket(%s): Error converting AI %p back to name\n",
          self->name, self->rp);
      *host = 0;
      *serv = 0;
    }

    /* If we are here, any old socket is invalid, clean them up and try again */
    if(self->sockfd >= 0) {
      o_log(O_LOG_DEBUG2, "socket(%s): FD %d already open, closing...\n",
          self->name, self->sockfd);
      close(self->sockfd);
    }

    if(0 > (self->sockfd =
          socket(self->rp->ai_family, self->rp->ai_socktype, self->rp->ai_protocol))) {
      o_log(O_LOG_DEBUG, "socket(%s): Could not create socket to [%s]:%s: %s\n",
          self->name, host, serv, strerror(errno));

    } else {
      if (nonblocking_mode) {
        fcntl(self->sockfd, F_SETFL, O_NONBLOCK);
      }

      if (0 != connect(self->sockfd, self->rp->ai_addr, self->rp->ai_addrlen)) {
        o_log(O_LOG_DEBUG, "socket(%s): Could not connect to [%s]:%s: %s\n",
            self->name, host, serv, strerror(errno));

      } else {
        if (!nonblocking_mode) {
          o_log(O_LOG_DEBUG, "socket(%s): Connected to [%s]:%s\n",
              self->name, host, serv);
        }
        self->is_disconnected = 0;

        /* Get ready to try the next addrinfo in case this one failed */
        //self->rp = self->rp->ai_next;
        return 1;

      }
    }
  }
  o_log(O_LOG_WARN, "socket(%s): Failed to connect\n",
      self->name, self->dest, self->service);

  freeaddrinfo(self->results);
  self->results = NULL;
  return 0;
}

/** Create a new outgoing TCP Socket.
 *
 * \param name name of this Socket, for debugging purposes
 * \param dest DNS name or address of the destination
 * \param service symbolic name or port number of the service to connect to
 * \return a newly-allocated Socket, or NULL on error
 */
Socket*
socket_tcp_out_new(
  const char* name,   //! Name used for debugging
  const char* dest,   //! IP address of the server to connect to.
  const char* service //! Port of remote service
) {
  SocketInt* self;

  if (dest == NULL) {
    o_log(O_LOG_ERROR, "socket(%s): Missing destination\n", name);
    return NULL;
  }

  if ((self = (SocketInt*)socket_new(name, TRUE)) == NULL) {
    return NULL;
  }

  self->dest = oml_strndup(dest, strlen(dest));
  self->service = oml_strndup(service, strlen(service));


  //  eventloop_on_out_channel((Socket*)self, on_self_connected, NULL);
  return (Socket*)self;
}

/** Attempt to reconnect an OComm Socket.
 *
 * \param socket OComm socket
 * \return 0 on failure, the new socket number otherwise
 * \see s_connect
 */
int socket_reconnect(Socket* socket) {
  SocketInt* self = (SocketInt*)socket;

  if (self == NULL) {
    o_log(O_LOG_ERROR, "Missing socket definition\n");
    return 0;
  }

  return s_connect(self);
}

/** Eventloop callback called when a new connection is received on a listening Socket.
 *
 * This function accept()s the connection, and creates a SocketInt to wrap
 * the underlying system socket. It then runs the user-supplied callback
 * (passed to socket_server_new() when creating the listening Socket) on this
 * new object, passing it the handle (passed to socket_server_new( too))
 *
 * \param source source from which the event was received (e.g., an OComm Channel)
 * \param handle pointer to opaque data passed when creating the listening Socket
 */
static void
on_client_connect(
  SockEvtSource* source,
  //  SocketStatus status,
  void* handle
) {
  (void)source; // FIXME: Check why source parameter is unused
  socklen_t cli_len;
  SocketInt* self = (SocketInt*)handle;
  SocketInt* newSock = socket_initialize(NULL);
  cli_len = sizeof(newSock->servAddr);
  newSock->sockfd = accept(self->sockfd,
                (struct sockaddr*)&newSock->servAddr,
                 &cli_len);
  if (newSock->sockfd < 0) {
    o_log(O_LOG_ERROR, "socket(%s): Error on accept: %s\n",
          self->name, strerror(errno));
    free(newSock);
    return;
  }

  sprintf(newSock->name, "%s-io:%d", self->name, newSock->sockfd);

  if (self->connect_callback) {
    self->connect_callback((Socket*)newSock, self->connect_handle);
  }
}

/** Create a listening Socket object and register it to the global EventLoop.
 *
 * If callback is non-NULL, it is registered to the OCOMM eventloop to handle
 * monitor_in events.
 *
 * \param name name of this object, used for debugging
 * \param port port to listen on
 * \param callback function to call when a client connects
 * \param handle pointer to opaque data passed to callback function
 * \return a pointer to the SocketInt object (cast as a Socket)
 *
 * XXX: Fix server-side #369 here: register one socket per AP to the eventloop
 */
Socket*
socket_server_new(
  char* name,
  int port,
  o_so_connect_callback callback,
  void* handle
) {
  SocketInt* self;
  if ((self = (SocketInt*)socket_in_new(name, port, TRUE)) == NULL)
    return NULL;

  listen(self->sockfd, 5);
  self->connect_callback = callback;
  self->connect_handle = handle;

  if (callback) {
    // Wait for a client to connect. Handle that in callback
    eventloop_on_monitor_in_channel((Socket*)self, on_client_connect, NULL, self);
  }
  return (Socket*)self;
}

/** Prevent the remote sender from trasmitting more data.
 *
 * \param socket Socket object for which to shut communication down
 *
 * \see shutdown(3)
 */
int socket_shutdown(Socket *socket) {
  int ret = -1;

  o_log(O_LOG_DEBUG, "socket(%s): Shutting down for R/W\n", socket->name);
  ret = shutdown(((SocketInt *)socket)->sockfd, SHUT_RDWR);
  if(ret) o_log(O_LOG_WARN, "socket(%s): Failed to shut down: %s\n", socket->name, strerror(errno));

  return ret;
}

/*! Method for closing communication channel, i.e.
  closing the multicast socket.
 */
int
socket_close(
  Socket* socket
) {
  SocketInt *self = (SocketInt*)socket;
  if (self->sockfd >= 0) {
    close(self->sockfd);
    // TODO: should check if close suceeds
    self->sockfd = -1;
  }
  return 0;
}

int
socket_sendto(
  Socket* socket,
  char* buf,
  int buf_size
) {
  SocketInt *self = (SocketInt*)socket;
  int sent;

  if (self->is_disconnected) {
    if(!s_connect(self)) {
      return 0;
    }
  }

  // TODO: Catch SIGPIPE signal if other side is half broken
  if ((sent = sendto(self->sockfd, buf, buf_size, 0,
                    (struct sockaddr *)&(self->servAddr),
                    sizeof(self->servAddr))) < 0) {
    if (errno == EPIPE || errno == ECONNRESET) {
      // The other end closed the connection.
      self->is_disconnected = 1;
      o_log(O_LOG_ERROR, "socket(%s): The remote peer closed the connection: %s\n",
            self->name, strerror(errno));
      return 0;
    } else if (errno == ECONNREFUSED) {
      self->is_disconnected = 1;
      o_log(O_LOG_DEBUG, "socket(%s): Connection refused, trying next AI\n",
            self->name);
      self->rp = self->rp->ai_next;
      return 0;
    } else if (errno == EINTR) {
      o_log(O_LOG_WARN, "socket(%s): Sending data interrupted: %s\n",
            self->name, strerror(errno));
      return 0;
    } else {
      o_log(O_LOG_ERROR, "socket(%s): Sending data failed: %s\n",
            self->name, strerror(errno));
    }
    return -1;
  }
  return sent;
}

int
socket_get_sockfd(
  Socket* socket
) {
  SocketInt *self = (SocketInt*)socket;

  return self->sockfd;
}

uint16_t
socket_get_port(Socket *s)
{
  assert(s);
  SocketInt *self = (SocketInt*)s;
  return ntohs(self->servAddr.sin_port);
}

size_t
socket_get_addr_sz(Socket *s)
{
  assert(s);
  return INET_ADDRSTRLEN;
}

void
socket_get_addr(Socket *s, char *addr, size_t addr_sz)
{
  assert(s);
  assert(addr);
  assert(socket_get_addr_sz(s) <= addr_sz);
  SocketInt *self = (SocketInt*)s;
  const char *tmp = inet_ntop(AF_INET, &self->servAddr.sin_addr, addr, addr_sz);
  assert(tmp == addr);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
