/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
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
#include <stdlib.h>
#include <string.h>
#include <log.h>
#include <mem.h>
#include <cbuf.h>
#include <mbuf.h>

#include "client.h"
#include "message_queue.h"

static int
dummy_read_msg_start (struct oml_message *msg, MBuffer *mbuf)
{
  return -1;
}

/**
 * @brief Create and initialise a +Client+ structure to represent a single client.
 *
 * @param client_sock the socket associated to the client transmission
 *        (from libocomm).  The client_sock should be attached to an active
 *        client that has been accepted by the server socket.
 *
 * @param page_size the page size for the underlying memory store for
 *        buffering received measurements.
 * @param file_name save measurements to a file with this name.
 * @param server_port the port of the downstream OML server
 * @param server_address the address of the downstream OML Server
 *
 * @return a new Client structure
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
    header_free (header);
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
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
