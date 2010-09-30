#include <stdlib.h>
#include <string.h>
#include <log.h>
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
 * \brief Create and initialise a +Client+ structure
 * \param client_sock the socket associated to the client transmition
 * \param page_size the size of the buffers
 * \param file_name the name of the file to save the measurements
 * \param server_port the destination port of the OML server
 * \param server_address the address of the OML Server
 * \return a new Client structure
 */
Client*
client_new (Socket* client_sock, int page_size, char* file_name,
           int server_port, char* server_address)
{
  Client* self = (Client *)malloc(sizeof(Client));
  memset(self, 0, sizeof(Client));

  self->state = C_HEADER;
  self->mbuf = mbuf_create ();
  self->headers = NULL;
  self->msg_start = dummy_read_msg_start;

  self->messages = msg_queue_create ();

  self->cbuf = cbuf_create (-1);

  self->file = fopen(file_name, "wa");
  self->file_name =  strdup (file_name);

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

  free (client->file_name);

  pthread_cond_destroy (&client->condvar);
  pthread_mutex_destroy (&client->mutex);

  socket_free (client->send_socket);
  socket_free (client->recv_socket);

  free (client);
}
