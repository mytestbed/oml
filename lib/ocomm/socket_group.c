/*
 * Copyright 2007-2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file socket.c
 * \brief Functions for managing a group of sockets as a unit.
 * Internally maintains a linked list of sockets which can be modified at
 */
#include <string.h>
#include <stdlib.h>

#include "mem.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_socket_group.h"
#include "ocomm/o_log.h"

#if 0 // FIXME:  Not used?
static o_socket_sendto sendto;
static o_get_sockfd get_sockfd;
#endif

typedef struct _sockHolder {

  Socket* socket;
  struct _sockHolder* next;

} SocketHolder;


//! Data-structure, to store object state
typedef struct _socketGroup {

  //! Name used for debugging
  char* name;

  o_socket_sendto sendto;
  o_get_sockfd get_sockfd;


  SocketHolder* first;

  //! Local memory for debug name
  char nameBuf[64];

} SocketGroupInt;




void
socket_group_add(
  Socket* this,   //! This object
  Socket* socket  //! Socket to add
) {
  SocketGroupInt* self = (SocketGroupInt*)this;

  SocketHolder* holder = (SocketHolder *)oml_malloc(sizeof(SocketHolder));
  memset(holder, 0, sizeof(SocketHolder));

  holder->socket = socket;
  holder->next = self->first;
  self->first = holder;
}

void
socket_group_remove(
  Socket* this,   //! This object
  Socket* socket  //! Socket to remove
) {
  SocketGroupInt* self = (SocketGroupInt*)this;

  SocketHolder* prev = NULL;
  SocketHolder* holder = self->first;
  while (holder != NULL) {
    if (socket ==  holder->socket) {
      // remove holder
      if (prev == NULL) {
	self->first = holder->next;
      } else {
	prev->next = holder->next;
      }
      oml_free(holder);
      return;
    }
    prev = holder;
    holder = holder->next;
  }
}

static int
sg_sendto(
  Socket* this,
  char* buf,
  int buf_size
) {
  SocketGroupInt* self = (SocketGroupInt*)this;
  int result = 0;

  SocketHolder* holder = self->first;
  while (holder != NULL) {
    Socket* s = holder->socket;
    int r;

    if ((r = s->sendto(s, buf, buf_size)) < result) {
      result = r;
    }
    holder = holder->next;
  }
  return result;
}

static int
sg_get_sockfd(
  Socket* socket
) {
  SocketGroupInt* self = (SocketGroupInt*)socket;

  o_log(O_LOG_ERROR, "Shouldn't call 'get_sockfd' on socket group '%s'.\n",
	self->name);
  return -1;
}

//! Return a new 'instance' structure
Socket*
socket_group_new(
  char* name
) {
  SocketGroupInt* self = (SocketGroupInt *)oml_malloc(sizeof(SocketGroupInt));
  memset(self, 0, sizeof(SocketGroupInt));

  self->name = self->nameBuf;
  strcpy(self->name, name != NULL ? name : "UNKNOWN");

  self->sendto = sg_sendto;
  self->get_sockfd = sg_get_sockfd;

  return (Socket*)self;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
