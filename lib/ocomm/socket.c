/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
/*!\file socket.c
  \brief This file contains a thin layer over sockets.
*/

#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"

#include <sys/timeb.h>
#include <sys/stat.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

//#include <expat.h>

#include "ocomm/o_log.h"

#ifndef TRUE
# define TRUE 1
#endif
#ifndef FALSE
# define FALSE !TRUE
#endif

#define MAX_SOCKET_INSTANCES 100

static int nonblocking_mode = 1;

//! Data-structure, to store communication related parameters and information.
typedef struct _socket {

  //! Name used for debugging
  char* name;

  o_socket_sendto sendto;
  o_get_sockfd get_sockfd;

  //! Socket file descriptor of the communication socket created to send/receive.
  int sockfd;

  //! True for TCP, false for UDP
  int is_tcp;

  //! IP address of the multicast socket/channel.
  //  char* addr;

  //! Remote port connected to
  //  int port;

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

  //! Link to next socket instance.
  struct _socket* next;

  int is_disconnected; ///< True if detected a SIGPIPE on a sendto()

} SocketInt;


/*
static int instance_cnt = 0;
static SocketInt* instances[MAX_SOCKET_INSTANCES];
*/

static SocketInt* instances = NULL;

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

/** Create a new instance of the Socket object (SocketInt).
 *
 * The sendto and get_sockfd are set to the default socket_sendto() and
 * socket_get_sockfd() functions, respectively.
 *
 * \param name name of the object, used for debugging
 * \return a pointer to the SocketInt object
 */
static SocketInt*
initialize(
  char* name
) {
  SocketInt* self = (SocketInt *)malloc(sizeof(SocketInt));
  memset(self, 0, sizeof(SocketInt));

  self->name = self->nameBuf;
  strcpy(self->name, name != NULL ? name : "UNKNOWN");

  self->sendto = socket_sendto;
  self->get_sockfd = socket_get_sockfd;

  return self;
}

void
socket_free (
  Socket* socket
) {
  free (socket);
}

/** Create a new system socket and link it to the SocketInt object. */
int
s_socket(
  SocketInt* self
) {
  // open a socket
  if((self->sockfd = socket(PF_INET,
                self->is_tcp ? SOCK_STREAM : SOCK_DGRAM, IPPROTO_IP)) < 0) {
    o_log (O_LOG_ERROR, "socket: Error creating socket\n");
    return 0;
  }
  if (nonblocking_mode) {
    fcntl(self->sockfd, F_SETFL, O_NONBLOCK);
  }
  o_log(O_LOG_DEBUG, "socket:%s: Socket %d successfully created\n",
    self->name, self->sockfd);
  return 1;
}

/** Create an unbound Socket object.
 *
 * \param name name of the object, used for debugging
 * \param is_tcp  true if TCP, false for UDP XXX: This should be more generic
 * \return a pointer to the SocketInt object, cast as a Socket
 */
Socket*
socket_new(
  char* name,
  int is_tcp
) {
  SocketInt* self = initialize(name);
  self->is_tcp = is_tcp;
  if (!s_socket(self)) {
    free(self);
    return NULL;
  }
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
Socket*
socket_in_new(
  char* name,
  int port,
  int is_tcp
) {
  SocketInt* self;
  if ((self = (SocketInt*)socket_new(name, is_tcp)) == NULL)
    return NULL;


//  o_log(O_LOG_DEBUG, "socket:%s: Attempt to join %s:%d\n", name, addr, port);

  self->servAddr.sin_family = PF_INET;
  self->servAddr.sin_port = htons(port);
  self->servAddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(self->sockfd, (struct sockaddr *)&self->servAddr,
      sizeof(struct sockaddr_in)) < 0) {
    o_log(O_LOG_ERROR, "socket:%s: Error binding socket to interface: %s\n",
          name, strerror(errno));
    return NULL;
  }

  self->localport = ntohs(self->servAddr.sin_port);
  o_log(O_LOG_DEBUG, "socket:%s: Socket bound to port: %d\n", name, self->localport);

  self->next = instances;
  instances = self;
  return (Socket*)self;
}
#if 0 // Suppress compiler warning b/c on_self_connected never used
static void
on_self_connected(
  Socket* source,
  SocketStatus status,
  void* handle
) {
  (void)handle;  // FIXME: Check why this parameter is unused.
  SocketInt* self = (SocketInt*)source;

  switch (status) {
  case SOCKET_CONN_REFUSED:
    o_log(O_LOG_ERROR, "socket:%s: Connection refused\n", self->name);
    break;
  default:
    o_log(O_LOG_ERROR, "socket:%s: Unknown socket status '%d'\n", self->name, status);
  }
}
#endif

/* Connect to remote addr.
 *  If addr is NULL, assume the servAddr is already populated
 */
static int
s_connect(
  SocketInt* self,
  char* addr,
  int port
) {
  if (addr != NULL) {
    struct hostent *server;

    server = gethostbyname(addr);
    if (server == NULL) {
      o_log(O_LOG_ERROR, "socket:%s: Unknown host %s\n", self->name, addr);
      return 0;
    }

    self->servAddr.sin_family = PF_INET;
    self->servAddr.sin_port = htons(port);
    bcopy((char *)server->h_addr,
      (char *)&self->servAddr.sin_addr.s_addr,
      server->h_length);
  }
  if (connect(self->sockfd, (struct sockaddr *)&self->servAddr,
          sizeof(struct sockaddr_in)) < 0) {
    if (errno != EINPROGRESS) {
      o_log(O_LOG_ERROR, "socket:%s: Error connecting to %s:%d: %s\n",
        self->name, addr, port, strerror(errno));
      return 0;
    }
  }
  return 1;
}

Socket*
socket_tcp_out_new(
  char* name,   //! Name used for debugging
  char* addr,   //! IP address of the server to connect to.
  int port //! Port of remote service
) {
  SocketInt* self;

  if (addr == NULL) {
    o_log(O_LOG_ERROR, "socket:%s: Missing address\n", name);
    return NULL;
  }

  if ((self = (SocketInt*)socket_new(name, TRUE)) == NULL)
    return NULL;

  if (!s_connect(self, addr, port)) {
    free(self);
    return NULL;
  }

  //  eventloop_on_out_channel((Socket*)self, on_self_connected, NULL);
  return (Socket*)self;
}

/*! Attempt to reconnect.
 *
 */
int
socket_reconnect(
  Socket* socket
) {
  SocketInt* self = (SocketInt*)socket;

  if (self == NULL) {
    o_log(O_LOG_ERROR, "Missing socket definition\n");
    return 0;
  }

  if (self->sockfd > 0) {
    close(self->sockfd);
    s_socket(self);
  }
  return s_connect(self, NULL, -1);
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

  SocketInt* newSock = initialize(NULL);
  cli_len = sizeof(newSock->servAddr);
  newSock->sockfd = accept(self->sockfd,
                (struct sockaddr*)&newSock->servAddr,
                 &cli_len);
  if (newSock->sockfd < 0) {
    o_log(O_LOG_ERROR, "socket:%s: Error on accept: %s\n",
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

static in_addr_t
iface2addr(
  char* name,
  char* iface //! Name of the interface (eth0/eth1) to bind to
) {
  if (iface == NULL) {
    // use DEFAULT interface
    return INADDR_ANY;
  }

  struct ifreq ifr;
  int ufd;
  memset(&ifr, 0, sizeof(ifr));
  strncpy(ifr.ifr_name, iface, IFNAMSIZ);
  ifr.ifr_addr.sa_family = AF_INET;

  if (((ufd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
      || ioctl(ufd, SIOCGIFADDR, &ifr) < 0)
    {
      o_log(O_LOG_ERROR, "socket:%s: Unable to resolve outgoing interface: %s",
        name, iface);
      return -1;
    }
  close(ufd);

  return ((struct sockaddr_in *)&(ifr.ifr_addr))->sin_addr.s_addr;
}



Socket*
socket_mc_in_new(
  char* name,   //! Name used for debugging
  char* addr,   //! IP address of the multicast socket/channel.
  int port, //! Port used
  char* iface //! Name of the interface (eth0/eth1) to bind to
) {
  SocketInt* self;
  if ((self = (SocketInt*)socket_in_new(name, port, FALSE)) == NULL)
    return NULL;

  //  self->addr = addr;
  //self->localport = port;
  self->iface = iface;

  // JOIN multicast group on default interface
  self->imreq.imr_multiaddr.s_addr = inet_addr(addr);
  self->imreq.imr_interface.s_addr = iface2addr(name, iface);

  if (setsockopt(self->sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
         (const void *)&(self->imreq),
         sizeof(struct ip_mreq)) < 0) {
    o_log(O_LOG_ERROR, "socket:%s: Error while joining a multicast group\n", name);
    return NULL;
  }
  o_log(O_LOG_DEBUG, "socket:%s: Ready to receive data on multicast address: %s\n",
    name, addr);
  return (Socket*)self;
}

Socket*
socket_mc_out_new(
  char* name,   //! Name used for debugging
  char* mcast_addr,   //! IP address of the multicast socket/channel.
  int mcast_port, //! Port used
  char* iface //! Name of the interface (eth0/eth1) to bind to
) {
  SocketInt* self;
//  if ((self = (SocketInt*)socket_new(name, mcast_addr, 0)) == NULL)
//    return NULL;
  if ((self = (SocketInt*)socket_new(name, FALSE)) == NULL)
    return NULL;

  // Multicast parameters
  unsigned char ttl = 3;
  unsigned char one = 3;  // loopback

  struct in_addr addr;
  addr.s_addr = iface2addr(name, iface);
  o_log(O_LOG_DEBUG, "socket:%s: Binding to %x\n", name, addr.s_addr);
  if (setsockopt(self->sockfd, IPPROTO_IP, IP_MULTICAST_IF, &addr, sizeof(addr)) < 0) {
    o_log (O_LOG_ERROR, "socket:%s: Setting outgoing interface for socket\n\t%s",
       name, strerror(errno));
    return NULL;
  }

  if (setsockopt(self->sockfd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl,
        sizeof(unsigned char)) < 0) {
    o_log(O_LOG_ERROR, "socket:%s: While setting TTL parameter for multicast socket\n\t%s",
      name, strerror(errno));
    return NULL;
  }
  if (setsockopt(self->sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
         &one, sizeof(unsigned char)) < 0) {
    o_log(O_LOG_ERROR, "socket%s: While setting the loopback on multicast socket\n\t%s",
      name, strerror(errno));
    return NULL;
  }

  //  self->addr = mcast_addr;
  self->servAddr.sin_port = htons(mcast_port);
  self->servAddr.sin_addr.s_addr = inet_addr(mcast_addr);
  o_log(O_LOG_DEBUG, "socket:%s: Ready to send data on: %s:%d\n",
    name, mcast_addr, mcast_port);
  return (Socket*)self;
}

/*! Method for closing communication channel, i.e.
  closing the multicast socket.
 */
int
socket_close(
  Socket* socket
) {
  SocketInt *self = (SocketInt*)socket;
  if (self->sockfd > 0) {
    close(self->sockfd);
    // TODO: should check if close suceeds
    self->sockfd = -1;
  }
  return 0;
}


/*! Method for closing ALL communication channels
 * Return 0 on success, -1 otherwise
 */
int
socket_close_all()

{
  int result = 0;
  SocketInt* sock = instances;

  while (sock != NULL) {
    if (socket_close((Socket*)sock) != 0) {
      result = -1;
    }
    sock = sock->next;
  }
  return result;
}

int
socket_sendto(
  Socket* socket,
  char* buf,
  int buf_size
) {
  SocketInt *self = (SocketInt*)socket;
  int sent;

  // TODO: Catch SIGPIPE signal if other side is half broken
  if ((sent = sendto(self->sockfd, buf, buf_size, 0,
                    (struct sockaddr *)&(self->servAddr),
                    sizeof(self->servAddr))) < 0) {
    if (errno == EPIPE || errno == ECONNRESET) {
      // The other end closed the connection.
      self->is_disconnected = 1;
      o_log(O_LOG_ERROR, "socket:%s: The remote peer closed the connection: %s\n",
            self->name, strerror(errno));
    } else if (errno = EINTR) {
      o_log(O_LOG_WARN, "socket:%s: Sending data interrupted: %s\n",
            self->name, strerror(errno));
      return 0;
    } else {
      o_log(O_LOG_ERROR, "socket:%s: Sending to multicast channel failed: %s\n",
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

/**
char*
socket_get_addr(
  Socket* socket
) {
  SocketInt *self = (SocketInt*)socket;

  return self->addr;
}
**/

/**
typedef enum _SockStatus {
  SOCKET_WRITEABLE,
  SOCKET_CONN_CLOSED,
  SOCKET_CONN_REFUSED,
  SOCKET_DROPPED,  //! Socket monitoring dropped by eventloop
  SOCKET_UNKNOWN
} SocketStatus;
**/

static const char* SocketStatus_names [] =
  {
    "SOCKET_WRITEABLE",
    "SOCKET_CONN_CLOSED",
    "SOCKET_CONN_REFUSED",
    "SOCKET_DROPPED",  //! Socket monitoring dropped by eventloop
    "SOCKET_UNKNOWN"
  };

const char*
socket_status_string(
  SocketStatus status
 ) {
  size_t length = sizeof (SocketStatus_names) / sizeof (SocketStatus_names[0]);

  if (status < length)
    {
      return SocketStatus_names[status];
    }
  else
    {
      return "SOCKET_UNKNOWN";
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
