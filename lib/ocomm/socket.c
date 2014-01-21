/*
 * Copyright 2007-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file socket.c
 * \brief OComm Socket (OSocket) are a thin abstraction layer that manages
 * sockets and provides some additional state management functions.
 * \author Max Ott (max@winlab.rutgers.edu), Olivier Mehani <olivier.mehani@nicta.com.au>
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

#define MAX_SOCKET_INSTANCES  100
#define HOSTLEN               256 /* RFC 1035, section 2.3.4 + 1 */
#define ADDRLEN               INET6_ADDRSTRLEN
#define SERVLEN               7   /* ndigits(65536) + 1          */
#define SOCKNAMELEN           (HOSTLEN+SERVLEN+2)

static int nonblocking_mode = 1;

/** Implementation of the Socket interface store communication related parameters and information. */
typedef struct SocketInt {

  char* name;               /**< Name used for debugging */

  o_socket_sendto sendto;   /**< Function called when data needs to be sent */
  o_get_sockfd get_sockfd;  /**< Function called to get the underlying socket(3) file descriptor */

  int sockfd;               /**< File descriptor of the underlying socket(3) */

  int is_tcp;               /**< True if TCP is used for this socket (XXX: anything else is broken */

  char* dest;               /**< String representing the destination of the connection */

  char* service;            /**< Remote service to connect to */

  sockaddr_t servAddr;      /**< System's representation of the local address */

  o_so_connect_callback connect_callback; /**< Callback for when new clients connect to listening sockets */

  void* connect_handle;     /**< Opaque argument to connect_callback (TCP servers only) */

  int is_disconnected;      /**< 1 if a SIGPIPE or ECONNREFUSED was received on a sendto() */

  struct addrinfo *results, /** < Results from getaddrinfo to iterate over */
                  *rp;      /** < Current iterator over getaddrinfo results */

  Socket* next;             /**< Pointer to the next OSocket in case more than one were instantiated. */

} SocketInt;


/** Set a global flag which, when set true will cause all newly created sockets
 * to be put in non-blocking mode, otherwise the sockets remain in the system
 * default mode.
 *
 * \param flag non-zero to enable nonblocking_mode
 * \return flag
 */
int
socket_set_non_blocking_mode(int flag) {
  return nonblocking_mode = flag;
}

/** If the return value is non-zero all newly created sockets will be put in
 * non-blocking mode, otherwise the sockets remain in the system default mode.
 */
int
socket_get_non_blocking_mode()
{
  return nonblocking_mode;
}

/** Get information about the connected state of a Socket
 * \param socket Socket to examine
 * \return 0 if the socket is connected, non-zero otherwise
 **/
int
socket_is_disconnected (Socket* socket)
{
  return ((SocketInt*)socket)->is_disconnected;
}

/** Get information about the listening state of a Socket
 * \param socket Socket to examine
 * \return 0 if the socket is listening, non-zero otherwise
 **/
int
socket_is_listening (Socket* socket)
{
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
socket_initialize(const char* name)
{
  const char* sock_name = (name != NULL)?name:"UNKNOWN";
  SocketInt* self = (SocketInt *)oml_malloc(sizeof(SocketInt));
  memset(self, 0, sizeof(SocketInt));
  self->sockfd = -1;

  self->name = oml_strndup(sock_name, strlen(sock_name));

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
void
socket_free (Socket* socket)
{
  SocketInt* self = (SocketInt *)socket;
  socket_close(socket);
  if (self->name) { oml_free(self->name); }
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
Socket*
socket_new(const char* name, int is_tcp)
{
  SocketInt* self = socket_initialize(name);
  self->is_tcp = is_tcp; /* XXX: replace is_tcp with a more meaningful variable to contain that */
  self->is_disconnected = 1;
  return (Socket*)self;
}

/** Create OSocket objects bound to name and service.
 *
 * This function binds the newly-created socket, but doesn't listen on it just
 * yet.  If name resolves to more than one AF, several sockets are created,
 * linked through their `next' field.
 *
 * \param name name of the object, used for debugging
 * \param node address or name to listen on; defaults to all if NULL
 * \param service symbolic name or port number of the service to bind to
 * \param is_tcp true if TCP, false for UDP XXX: This should be more generic
 * \return a pointer to a linked list of SocketInt objects, cast as Socket
 *
 * \see getaddrinfo(3)
 */
Socket*
socket_in_new(const char* name, const char* node, const char* service, int is_tcp)
{
  SocketInt *self = NULL, *list = NULL;
  char nameserv[SOCKNAMELEN], *namestr = NULL;
  size_t namestr_size;
  struct addrinfo hints, *results, *rp;
  int ret;

  *nameserv = 0;
  memset(&hints, 0, sizeof(struct addrinfo));

  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = IPPROTO_TCP; /* FIXME: We should check self->is_tcp */
  hints.ai_flags= AI_PASSIVE;
  int val = 1;

  if((ret=getaddrinfo(node, service, &hints, &results))) {
      o_log(O_LOG_ERROR, "socket(%s): Error resolving %s:%s: %s\n",
          name, node, service, gai_strerror(ret));
      return NULL;
  }

  for (rp = results; rp != NULL; rp = rp->ai_next) {
    sockaddr_get_name((sockaddr_t*)rp->ai_addr, rp->ai_addrlen, nameserv, SOCKNAMELEN);
    namestr_size = strlen(name) + strlen(nameserv + 2);
    namestr = oml_realloc(namestr, namestr_size);
    snprintf(namestr, namestr_size, "%s-%s", name, nameserv);
    o_log(O_LOG_DEBUG, "socket(%s): Binding to %s (AF %d, proto %d)\n",
        name, nameserv, rp->ai_family, rp->ai_protocol);

    if (NULL == (self = (SocketInt*)socket_new(namestr, is_tcp))) {
      o_log(O_LOG_WARN, "socket(%s): Could allocate Socket to listen on %s: %s\n",
          self->name, nameserv, strerror(errno));

    } else if(0 > (self->sockfd =
          socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol))) {
      /* It's OK if some AFs fail, we check that in the end */
      o_log(O_LOG_DEBUG, "socket(%s): Could not create socket to listen on %s: %s\n",
          self->name, nameserv, strerror(errno));
      socket_free((Socket*)self);

    } else if(0 != setsockopt(self->sockfd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int))){
      o_log(O_LOG_ERROR, "socket(%s): Could not set option SO_REUSEADDR on socket to listen on %s: %s\n",
          self->name, nameserv, strerror(errno));
      close(self->sockfd);
      /* XXX: Not optimal: we could reuse the Socket */
      socket_free((Socket*)self);

    } else if (0 != bind(self->sockfd, rp->ai_addr, rp->ai_addrlen)) {
      o_log(O_LOG_ERROR, "socket(%s): Error binding socket to listen on %s: %s\n",
          self->name, nameserv, strerror(errno));
      close(self->sockfd);
      /* XXX: Not optimal: we could reuse the Socket */
      socket_free((Socket*)self);

    } else {
      /* The last connected AFs (hence less important according to GAI) are
       * now at the front, but it's not a big deal as we treat them all
       * equally (i.e, we're going to listen on them all */
      self->next = (Socket*)list;
      list = self;
      self = NULL;
    }

  }

  if (namestr) { oml_free(namestr); }
  freeaddrinfo(results);

  if (NULL == list) {
    o_log(O_LOG_ERROR, "socket(%s): Could not create any socket to listen on [%s]:%s: %s\n",
        self->name, node, service, strerror(errno));
  }

  return (Socket*)list;
}

/** Connect the socket to remote peer.
 *
 * If addr is NULL, assume the servAddr is already populated, and ignore port.
 *
 * \param self OComm socket to use
 * \return 1 on success, 0 on error
 */
static int
s_connect(SocketInt* self)
{
  char name[SOCKNAMELEN];
  struct addrinfo hints;
  int ret;

  *name = 0;

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
    if ((ret=getaddrinfo(self->dest, self->service, &hints, &self->results))) {
      o_log(O_LOG_ERROR, "socket(%s): Error resolving %s:%s: %s\n",
          self->name, self->dest, self->service, gai_strerror(ret));
      return 0;
    }
    self->rp = self->results;
  }

  for (; self->rp != NULL; self->rp = self->rp->ai_next) {
    sockaddr_get_name((sockaddr_t*)self->rp->ai_addr, self->rp->ai_addrlen,
        name, SOCKNAMELEN);
    o_log(O_LOG_DEBUG, "socket(%s): Connecting to %s (AF %d, proto %d)\n",
        self->name, name, self->rp->ai_family, self->rp->ai_protocol);

    /* If we are here, any old socket is invalid, clean them up and try again */
    if(self->sockfd >= 0) {
      o_log(O_LOG_DEBUG2, "socket(%s): FD %d already open, closing...\n",
          self->name, self->sockfd);
      close(self->sockfd);
    }

    if(0 > (self->sockfd =
          socket(self->rp->ai_family, self->rp->ai_socktype, self->rp->ai_protocol))) {
      o_log(O_LOG_DEBUG, "socket(%s): Could not create socket to %s %s\n",
          self->name, name, strerror(errno));

    } else {
      if (nonblocking_mode) {
        fcntl(self->sockfd, F_SETFL, O_NONBLOCK);
      }

      if (0 != connect(self->sockfd, self->rp->ai_addr, self->rp->ai_addrlen)) {
        o_log(O_LOG_DEBUG, "socket(%s): Could not connect to %s: %s\n",
            self->name, name, strerror(errno));

      } else {
        if (!nonblocking_mode) {
          o_log(O_LOG_DEBUG, "socket(%s): Connected to %s\n",
              self->name, name);
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
socket_tcp_out_new(const char* name, const char* dest, const char* service)
{
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
on_client_connect(SockEvtSource* source, void* handle)
{
  (void)source; // FIXME: Check why source parameter is unused
  char host[ADDRLEN], serv[SERVLEN];
  size_t namesize;
  *host = 0;
  *serv = 0;

  socklen_t cli_len;
  SocketInt* self = (SocketInt*)handle;
  SocketInt* newSock = socket_initialize(NULL);
  cli_len = sizeof(newSock->servAddr.sa_stor);
  newSock->sockfd = accept(self->sockfd,
                &newSock->servAddr.sa,
                 &cli_len);

  if (newSock->sockfd < 0) {
    o_log(O_LOG_ERROR, "socket(%s): Error on accept: %s\n",
          self->name, strerror(errno));
    oml_free(newSock);
    return;
  }

  /* XXX: Duplicated somewhat with socket_in_new and s_connect */
  if (!getnameinfo(&newSock->servAddr.sa, cli_len,
        host, ADDRLEN, serv, SERVLEN,
        NI_NUMERICHOST|NI_NUMERICSERV)) {
    namesize =  strlen(host) + strlen(serv) + 3 + 1;
    newSock->name = oml_realloc(newSock->name, namesize);
    snprintf(newSock->name, namesize, "[%s]:%s", host, serv);

  } else {
    namesize =  strlen(host) + 4 + 10 + 1; /* XXX: 10 is arbitrarily chosen for the
                                              number of characters in sockfd's decimal
                                              representation */
    newSock->name = oml_realloc(newSock->name, namesize);
    snprintf(newSock->name, namesize, "%s-io:%d", self->name, newSock->sockfd);
    o_log(O_LOG_WARN, "socket(%s): Error resolving new client source, defaulting to %s: %s\n",
        self->name, newSock->name, strerror(errno));
  }

  if (self->connect_callback) {
    self->connect_callback((Socket*)newSock, self->connect_handle);
  }
}

/** Create listening OSocket objects, and register them with the EventLoop.
 *
 * If callback is non-NULL, it is registered to the OCOMM eventloop to handle
 * monitor_in events.
 *
 * \param name name of the object, used for debugging
 * \param node address or name to listen on; defaults to all if NULL
 * \param service symbolic name or port number of the service to bind to
 * \param callback function to call when a client connects
 * \param handle pointer to opaque data passed to callback function
 * \return a pointer to a linked list of Socket objects
 *
 * \see socket_in_new
 */
Socket*
socket_server_new(const char* name, const char* node, const char* service, o_so_connect_callback callback, void* handle)
{
  Socket *socketlist;
  SocketInt *it;

  socketlist = socket_in_new(name, node, service, TRUE);

  for (it=(SocketInt*)socketlist; it; it=(SocketInt*)it->next) {
    listen(it->sockfd, 5);
    it->connect_callback = callback;
    it->connect_handle = handle;

    if (callback) {
      // Wait for a client to connect. Handle that in callback
      eventloop_on_monitor_in_channel((Socket*)it, on_client_connect, NULL, it);
    }
  }
  return socketlist;
}

/** Prevent the remote sender from trasmitting more data.
 *
 * \param socket Socket object for which to shut communication down
 *
 * \see shutdown(3)
 */
int
socket_shutdown(Socket *socket)
{
  int ret = -1;

  o_log(O_LOG_DEBUG, "socket(%s): Shutting down for R/W\n", socket->name);
  ret = shutdown(((SocketInt *)socket)->sockfd, SHUT_RDWR);
  if(ret) o_log(O_LOG_WARN, "socket(%s): Failed to shut down: %s\n", socket->name, strerror(errno));

  return ret;
}

/** Close the communication channel associated to an OSocket.
 * \param socket Socket to close
 * \return 0 on success, -1 otherwise
 */
int
socket_close(Socket* socket)
{
  SocketInt *self = (SocketInt*)socket;
  if (self->sockfd >= 0) {
    close(self->sockfd);
    // TODO: should check if close suceeds
    self->sockfd = -1;
  }
  return 0;
}

/** Send a message through the socket
 *
 * \param socket Socket to send message through
 * \param buf data to send
 * \param buf_size amount of data to read from buf
 * \return the amount of data sent
 *
 * \see sendto(3)
 */
int
socket_sendto(Socket* socket, char* buf, int buf_size)
{
  SocketInt *self = (SocketInt*)socket;
  int sent;

  if (self->is_disconnected) {
    if(!s_connect(self)) {
      return 0;
    }
  }

  // TODO: Catch SIGPIPE signal if other side is half broken
  if ((sent = sendto(self->sockfd, buf, buf_size, 0,
                    &(self->servAddr.sa),
                    sizeof(self->servAddr.sa_stor))) < 0) {
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

/** Get the socket FD of an OComm Socket object
 *
 * \param socket OSocket
 * \return the underlying socket descriptor
 */
int
socket_get_sockfd(Socket* socket)
{
  SocketInt *self = (SocketInt*)socket;
  return self->sockfd;
}

/** Get the port of socket contained in an OComm Socket object
 *
 * \param socket OSocket
 * \return the underlying socket descriptor
 */
uint16_t
socket_get_port(Socket *s)
{
  assert(s);
  SocketInt *self = (SocketInt*)s;
  return ntohs(self->servAddr.sa_in.sin_port);
}

/** Get the size needed to store a string representation of an OSocket's address
 *
 * This is just a dummy function exposing SOCKNAMELEN
 *
 * \param s Socket under consideration
 */
size_t
socket_get_addr_sz(Socket *s)
{
  assert(s);
  return SOCKNAMELEN;
}

/** Get the peer address of an OSocket.
 *
 * \param s OSocket
 * \param addr memory buffer to return the string in (nil-terminated)
 * \param namelen length of name, using socket_get_addr_sz() is a good idea
 *
 * \see sockaddr_get_name, getsockname(3), socket_get_addr_sz
 */
void
socket_get_peer_addr(Socket *s, char *addr, size_t addr_sz)
{
  int ret;
  sockaddr_t sa;
  socklen_t sa_len = sizeof(sa);
  SocketInt *self = (SocketInt*)s;

  assert(self);
  assert(self->sockfd>=0);
  assert(addr);
  assert(socket_get_addr_sz(s) <= addr_sz);

  memset(&sa, 0, sa_len);

  if(getpeername(self->sockfd, &sa.sa, &sa_len)) {
    o_log(O_LOG_WARN, "%s: Error getting peer address: %s\n",
        self->name, strerror(errno));
    snprintf(addr, addr_sz, "Unknown peer");

  } else if ((ret=getnameinfo(&sa.sa, sa_len, addr, addr_sz, NULL, 0, NI_NUMERICHOST))) {
    o_log(O_LOG_WARN, "%s: Error converting peer address to name: %s\n",
        self->name, gai_strerror(ret));
    snprintf(addr, addr_sz, "Unknown address (AF%d)", sa.sa.sa_family);
  }
}

/** Get the name (address+service) of an OSocket.
 *
 * \param s OSocket
 * \param name memory buffer to return the string in (nil-terminated)
 * \param namelen length of name, using socket_get_addr_sz() is a good idea
 * \param remote 0 if the local name is needed, non-zero for the remote peer
 *
 * \see sockaddr_get_name, getsockname(3), socket_get_addr_sz
 */
void
socket_get_name(Socket *s, char *name, size_t namelen, int remote)
{
  sockaddr_t sa;
  socklen_t sa_len = sizeof(sa);
  SocketInt *self = (SocketInt*) s;

  assert(self);
  assert(name);
  assert(self->sockfd);

  memset(&sa, 0, sa_len);

  if(remote) {
    if(!getpeername(self->sockfd, &sa.sa, &sa_len)) {
      sockaddr_get_name(&sa, sa_len, name, namelen);

    } else {
      logwarn("%s: Cannot get details of socket peer: %s", s->name, strerror(errno));
      *name = 0;
    }

  } else {
    if(!getsockname(self->sockfd, &sa.sa, &sa_len)) {
      sockaddr_get_name(&sa, sa_len, name, namelen);

    } else {
      logwarn("%s: Cannot get details of socket: %s", s->name, strerror(errno));
      *name = 0;
    }
  }

}

/** Get the name (address+service) of a socket.
 *
 * \param sa sockaddr_t containig the data
 * \param sa_len length of sa
 * \param name memory buffer to return the string in (nil-terminated)
 * \param namelen length of nam, SOCKNAMELEN is a good idea
 *
 * \see socket_get_name, getnameinfo(3)
 */
void
sockaddr_get_name(const sockaddr_t *sa, socklen_t sa_len, char *name, size_t namelen)
{
  char host[ADDRLEN], serv[SERVLEN];
  int ret;

  assert(sa);
  assert(name);

  *host = 0;
  *serv = 0;

  if (!(ret=getnameinfo(&sa->sa, sa_len,
        host, ADDRLEN, serv, SERVLEN,
        NI_NUMERICHOST|NI_NUMERICSERV))) {
    snprintf(name, namelen, "[%s]:%s", host, serv);

  } else {
    o_log(O_LOG_DEBUG, "Error converting sockaddr %p to name: %s\n", sa, gai_strerror(ret));
    snprintf(name, namelen, "AF%d", sa->sa.sa_family);
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
