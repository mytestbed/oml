/*
 * Copyright 2007-2016 National ICT Australia (NICTA), Australia
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file o_socket.h
 * \brief Header file for the OComm Socket library. OSockets are a thin
 * abstraction layer that manages sockets and provides some additional state
 * management functions.
 * \author Max Ott (max@winlab.rutgers.edu), Olivier Mehani <olivier.mehani@nicta.com.au>
 */

#ifndef O_SOCKET_H
#define O_SOCKET_H

#include <stddef.h>
#include <stdint.h>
#include <sys/socket.h>
#include <netdb.h>

#ifdef __cplusplus
extern "C" {
#endif

struct Socket;

/** Send a message through the socket
 * \return 0 on success, -1 otherwise
 */
typedef int (*o_socket_sendto)(struct Socket* socket, char* buf, int buf_size);


/*! Return the file descripter associated with this socket
 * \return socket's fd, -1 otherwise
 */
typedef int (*o_get_sockfd)(struct Socket* socket);

/** An opaque data type for Ocomm Sockets */
typedef struct Socket {

  char* name;               /**< Name used for debugging */

  o_socket_sendto sendto;   /**< Function called when data needs to be sent */
  o_get_sockfd get_sockfd;  /**< Function called to get the underlying socket(3) file descriptor */

} Socket;

/** A union to allow easy manipulation of sockaddr without casting */
typedef union {
  struct sockaddr sa;
  struct sockaddr_in sa_in;
  struct sockaddr_in6 sa_in6;
  struct sockaddr_storage sa_stor;
} sockaddr_t;

/** Define the signature of a callback to report a new IN socket
 * from a client connecting to a listening socket.
 * \param newSock newly-created Socket to handle the client
 * \param opaque argument to connect_callback
 */
typedef void (*o_so_connect_callback)(Socket* newSock, void* handle);

/** Set a global flag which, when set true will cause all newly created sockets
 * to be put in non-blocking mode, otherwise the sockets remain in the system
 * default mode.
 */
int socket_set_non_blocking_mode(int flag);

/** If the return value is non-zero all newly created sockets will be put in
 * non-blocking mode, otherwise the sockets remain in the system default mode.
 */
int socket_get_non_blocking_mode(void);

/** Get information about the connected state of a Socket */
int socket_is_disconnected (Socket* socket);

/** Get information about the listening state of a Socket */
int socket_is_listening (Socket* socket);

/** Create an unbound socket object.  */
Socket* socket_new(const char* name, int is_tcp);

/** Create OSocket objects bound to node and service. */
Socket* socket_in_new(const char* name, const char* node, const char* service, int is_tcp);

/** Create listening OSocket objects, and register them with the EventLoop .*/
Socket* socket_server_new(const char* name, const char* node, const char* service, o_so_connect_callback callback, void* handle);

/** Create a outgoing TCP socket object. */
Socket* socket_tcp_out_new(const char* name, const char* addr, const char *service);

/** Prevent the remote sender from trasmitting more data. */
int socket_shutdown(Socket *socket);

/** Close the communication channel associated to an OSocket. */
int socket_close(Socket* socket);

/** Free memory associated to an OSocket. */
void socket_free (Socket* socket);

/** Send a message through the socket */
int socket_sendto(Socket* socket, char* buf, int buf_size);

/* Return the file descripter associated with this socket */
int socket_get_sockfd(Socket* socket);

/** Return the port number for this socket. */
uint16_t socket_get_port(Socket *s);

/** Get the size needed to store a string representation of an OSocket's address */
size_t socket_get_addr_sz(Socket *s);

/** Get the peer address of an OSocket. */
void socket_get_peer_addr(Socket *s, char *addr, size_t addr_sz);

/** Get the name (address+service) of a socket. */
void sockaddr_get_name(const sockaddr_t *sa, socklen_t sa_len, char *name, size_t namelen);

/** Get the name (address+service) of an OSocket.  */
void socket_get_name(Socket *s, char *name, size_t namelen, int remote);

#ifdef __cplusplus
}
#endif

#endif /* O_SOCKET_H */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
