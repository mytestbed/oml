#include <stdlib.h>
#include <string.h>
#include <log.h>

#include "client.h"

static int
dummy_read_msg_start (struct oml_message *msg, MBuffer *mbuf)
{
  return -1;
}

/**
 * \brief Create and initialise a new +ClientBuffer+ structure
 * \param size the maximum lenght of the buffer
 * \param number the page number
 * \return
 */
ClientBuffer*
make_client_buffer (int size, int number)
{
  ClientBuffer* self = (ClientBuffer*) malloc(sizeof(ClientBuffer));
  memset(self, 0, sizeof(ClientBuffer));
  self->max_length = size;
  self->page_number = number;
  self->current_size = 0;
  self->bytes_already_sent = 0;
  self->buff = (unsigned char*) malloc(size*sizeof(char));

  memset(self->buff, 0, size*sizeof(char));
  self->buff_to_send = self->buff;
  self->current_pointer = self->buff;
  self->next = NULL;

  return self;
}

void
free_client_buffer (ClientBuffer *buffer)
{
  free (buffer->buff);
  free (buffer);
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

  //TODO change it to integrate option of the command line
  self->recv_buffer = make_client_buffer (page_size, 0);

  self->first_buffer = self->recv_buffer;
  self->send_buffer = self->recv_buffer;
  self->current_page = 0;
  self->bytes_sent = 0;

  self->file = fopen(file_name, "wa");
  self->file_name =  strdup (file_name);

  self->recv_socket = client_sock;
  self->recv_socket_closed = 0;

  //  self->send_socket =  socket_tcp_out_new(file_name, server_address, server_port);
  //  self->send_socket_closed = 0;

  /* FIXME:  Return value checking */
  //  pthread_create(&self->thread, NULL, client_send_thread, (void*)self);
  //  pthread_mutex_init (&self->mutex, NULL);
  //  pthread_cond_init (&self->condvar, NULL);

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

  ClientBuffer *buffer = client->first_buffer, *next = NULL;
  while (buffer) {
    next = buffer->next;
    free_client_buffer (buffer);
    buffer = next;
  }

  pthread_cond_destroy (&client->condvar);
  pthread_mutex_destroy (&client->mutex);

  socket_free (client->send_socket);
  socket_free (client->recv_socket);

  free (client);
}
