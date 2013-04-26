/*
 * Copyright 2010-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file proxy_client.c
 * \brief Functions to manage the Client structure.
 */
#include <stdlib.h>
#include <string.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "mbuf.h"
#include "cbuf.h"
#include "proxy_client.h"
#include "message_queue.h"

static int
dummy_read_msg_start (struct oml_message *msg, MBuffer *mbuf)
{
  (void)msg; (void)mbuf;
  return -1;
}

/** Create and initialise a +Client+ structure to represent a single client.
 *
 * \param client_sock the socket associated to the client transmission
 *        (from libocomm).  The client_sock should be attached to an active
 *        client that has been accepted by the server socket.
 *
 * \param page_size the page size for the underlying memory store for
 *        buffering received measurements.
 * \param file_name save measurements to a file with this name.
 * \param server_port the port of the downstream OML server
 * \param server_address the address of the downstream OML Server
 *
 * \return a new Client structure
 */
Client*
client_new (Socket* client_sock, int page_size, char* file_name,
           int server_port, char* server_address)
{
  Client* self = (Client *)xmalloc(sizeof(Client));
  memset(self, 0, sizeof(Client));

  self->state = C_HEADER;
  self->downstream_port = server_port;
  self->downstream_addr = xstrndup (server_address, strlen (server_address));
  self->mbuf = mbuf_create ();
  self->headers = NULL;
  self->msg_start = dummy_read_msg_start;

  self->messages = msg_queue_create ();
  self->cbuf = cbuf_create (page_size);

  self->file = fopen(file_name, "wa");
  self->file_name =  xstrndup (file_name, strlen (file_name));

  self->recv_socket = client_sock;

  /* FIXME:  Return value checking */
  pthread_mutex_init (&self->mutex, NULL);
  pthread_cond_init (&self->condvar, NULL);

  return self;
}

void
client_free (Client *client)
{
  if (client == NULL) return;

  if (client->file) {
    fflush (client->file);
    fclose (client->file);
    client->file = NULL;
  }

  xfree (client->file_name);
  xfree (client->downstream_addr);

  struct header *header = client->headers;

  while (header) {
    struct header *next = header->next;
    header_free (header);
    header = next;
  }

  msg_queue_destroy (client->messages);
  cbuf_destroy (client->cbuf);

  pthread_cond_destroy (&client->condvar);
  pthread_mutex_destroy (&client->mutex);

  socket_free (client->recv_socket);

  xfree (client);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
