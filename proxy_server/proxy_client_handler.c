/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
#include <unistd.h>

#include <log.h>
#include <mem.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include <message.h>
#include <text.h>
#include <binary.h>

#include "proxy_client_handler.h"

#define DEF_TABLE_COUNT 10

extern ProxyServer* proxyServer;

/**
 * \brief Create and initialise a new +ClientBuffer+ structure
 * \param size the maximum lenght of the buffer
 * \param number the page number
 * \return
 */
ClientBuffer* make_client_buffer (int size, int number)
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


static void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

/* errno is not a good name for an variable as it expands to a fn ptr
   when unistd.h is included */
static void
status_callback(SockEvtSource* source, SocketStatus status, int err_no,
                void* handle);

void
client_free (Client *client);

/**
 * \brief function that will try to send data to the real OML server
 * \param handle the client data structure i.e. a pointer to a Client*.
 */
static void* client_send_thread (void* handle)
{
  Client *client = (Client*)handle;
  pthread_mutex_lock (&client->mutex);

  while(1) {
    pthread_cond_wait (&client->condvar, &client->mutex);

    if (proxyServer->state == ProxyState_SENDING) {
      while (client->send_buffer &&
             client->send_buffer != client->recv_buffer) {
        ClientBuffer* buffer = client->send_buffer;

        int bytes_to_send = buffer->current_size - buffer->bytes_already_sent;

        if (bytes_to_send > 0) {
          /* Don't block other threads while sending data */
          pthread_mutex_unlock (&client->mutex);
          int result = socket_sendto (client->send_socket, (char*)buffer->buff_to_send,
                                      bytes_to_send);
          pthread_mutex_lock (&client->mutex);
          if (result == 0) {
            buffer->buff_to_send += bytes_to_send;
            buffer->bytes_already_sent = buffer->current_size;
            client->bytes_sent += bytes_to_send;
          } else {
            logerror ("Error sending %d bytes to downstream server:  %s\n",
                      bytes_to_send, strerror (errno));
          }
        }
        client->send_buffer = client->send_buffer->next;
      }

      if (client->recv_socket_closed == 1) {
        /* Client disconnected */
        logdebug("Closing downstream server connection '%s'\n", client->send_socket->name);
        socket_close (client->send_socket);
        client->send_socket_closed = 1;
        client_free (client);
        pthread_exit (NULL);
      }
    } else {
      logdebug ("Client sender thread woke up when not in state SENDING (actual state %d)\n",
                proxyServer->state);
    }
  }
}

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

  self->recv_buffer = make_client_buffer (page_size, 0); //TODO change it to integrate option of the command line

  self->first_buffer = self->recv_buffer;
  self->send_buffer = self->recv_buffer;
  self->current_page = 0;
  self->bytes_sent = 0;

  self->file = fopen(file_name, "wa");
  self->file_name =  strdup (file_name);

  self->recv_socket = client_sock;
  self->recv_socket_closed = 0;

  self->send_socket =  socket_tcp_out_new(file_name, server_address, server_port);
  self->send_socket_closed = 0;

  /* FIXME:  Return value checking */
  pthread_create(&self->thread, NULL, client_send_thread, (void*)self);
  pthread_mutex_init (&self->mutex, NULL);
  pthread_cond_init (&self->condvar, NULL);

  return self;
}

void
client_free (Client *client)
{
  if (client == NULL) return;
  /* Unlink this client from the main client list */
  Client *current = proxyServer->first_client;
  if (current == client) proxyServer->first_client = current->next;
  while (current) {
    if (current->next == client) {
      current->next = current->next->next;
      break;
    }
    current = current->next;
  }

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

void
client_socket_monitor (Socket* client_sock, Client* client)
{
  eventloop_on_read_in_channel(client_sock, client_callback, status_callback, (void*)client);
}

/**
 * @brief Read a line from mbuf.
 *
 * Sets *line_p to point to the start of the line, and *length_p to
 * the length of the line and returns 0 if successful.  Note that
 * *line_p points into the mbuf internal buffer, and the line is not
 * '\0'-terminated, i.e. the data isn't touched.
 *
 * If there is no newline character, returns -1 and *line_p and
 * *length_p are untouched.
 *
 * @param line_p out parameter for the start of the line
 * @param length_p out parameter for the length of the line
 * @param mbuf the buffer that contains the data to be read
 * @return 1 if successful, 0 otherwise
 */
static int
read_line(char** line_p, int* length_p, MBuffer* mbuf)
{
  uint8_t* line = mbuf_rdptr (mbuf);
  int length = mbuf_find (mbuf, '\n');

  // No newline found
  if (length == -1)
    return -1;

  *line_p = (char*)line;
  *length_p = length;
  return 0;
}

int
read_header (Client *self, MBuffer *mbuf)
{
  char *line;
  int len;
  struct header *header;

  if (self == NULL || mbuf == NULL) {
    logerror ("read_header():  NULL client or input buffer.\n");
    return 0;
  }

  if (read_line (&line, &len, mbuf) == -1)
    return 0;

  if (len == 0) {
    // Empty line denotes separator between header and body
    int skip_count = mbuf_find_not (mbuf, '\n');
    mbuf_read_skip (mbuf, skip_count);
    self->state = C_CONFIGURE;
    return 0;
  }

  header = header_from_string (line, len);
  mbuf_read_skip (mbuf, len + 1);
  if (header == NULL) {
    /* Could be protocol error (not ':'), memory alloc error, or unknown tag */
    self->state = C_PROTOCOL_ERROR;
    return -1;
  }

  /* Store the header for later processing */
  header->next = self->headers;
  self->headers = header;
  if (header->tag != H_NONE) {
    self->header_table[header->tag] = header;
  }

  return 1; // still in header
}

/**
 * @brief Convert a content header into the correct symbol type.
 *
 * @param str the string represention of the content type, either
 * "text" or "binary".
 * @return The symbolic representation of the content type, either
 * CONTENT_BINARY, CONTENT_TEXT, or CONTENT_NONE to indicate an unrecognized
 * content type string.
 */
enum ContentType
content_from_string (const char *str)
{
  if (str == NULL)
    return CONTENT_NONE;

  if (strcmp (str, "binary") == 0)
    return CONTENT_BINARY;
  else if (strcmp (str, "text") == 0)
    return CONTENT_TEXT;

  return CONTENT_NONE;
}

/**
 * \brief function called when the socket receive some data
 * \param source the socket event
 * \param handle the cleint handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
void
client_callback(SockEvtSource *source, void *handle, void *buf, int buf_size)
{
  (void)source;
  Client* self = (Client*)handle;
  MBuffer *mbuf = self->mbuf;
  struct header *header;
  struct oml_message msg;
  int result;

  logdebug ("'%s': received %d octets of data\n", source->name, buf_size);
  logdebug ("'%s': %s\n", source->name, to_octets (buf, buf_size));

  result = mbuf_write (mbuf, buf, buf_size);
  if (result == -1) {
    logerror ("'%s': Failed to write message from client into message buffer. Data is being lost!\n",
              source->name);
    return;
  }

 loop:
  switch (self->state) {
  case C_HEADER:
    /* Read headers until out of header data to read*/
    while (read_header (self, mbuf));
    if (self->state != C_HEADER)
      goto loop;
    break;
  case C_CONFIGURE:
    if (self->header_table[H_EXPERIMENT_ID] == H_NONE ||
        self->header_table[H_CONTENT] == H_NONE) {
      self->state = C_PROTOCOL_ERROR;
    } else {
      self->experiment_id = self->header_table[H_EXPERIMENT_ID]->value;
      self->content = content_from_string (self->header_table[H_CONTENT]->value);
      if (self->content == CONTENT_NONE)
        self->state = C_PROTOCOL_ERROR;
    }
    logdebug ("%s\n", self->experiment_id);
    logdebug ("%d\n", self->content);
    for (header = self->headers; header != NULL; header = header->next) {
      logdebug ("HEADER:  '%s' : '%s'\n",
                tag_to_string(header->tag),
                header->value);
    }
    switch (self->content){
    case CONTENT_TEXT:
      self->msg_start = text_read_msg_start;
      break;
    case CONTENT_BINARY:
      self->msg_start = bin_read_msg_start;
      break;
    default:
      /* Default is set when client is created (client_new()) */
      break;
    }
    mbuf_consume_message (mbuf); // Next message starts after the headers.
    self->state = C_DATA;
    break;
  case C_DATA:
    result = self->msg_start (&msg, mbuf);
    if (result == -1) {
      logerror ("'%s': protocol error in received message\n", source->name);
      self->state = C_PROTOCOL_ERROR;
      goto loop;
    } else if (result == 0) {
      // Try again when we get more data.
      logdebug ("'%s': need more data\n", source->name);
      mbuf_reset_read (mbuf);
      return;
    }
    logdebug ("Recieved [strm=%d seqno=%d ts=%f %d bytes]\n",
              msg.stream, msg.seqno, msg.timestamp, msg.length);
    mbuf_reset_read (mbuf);
    char *s = xstrndup (mbuf_rdptr (mbuf), msg.length);
    if (self->content == CONTENT_BINARY)
      logdebug ("%s\n", to_octets (s, msg.length));
    else
      logdebug ("'%s'\n", s);

    mbuf_read_skip (mbuf, msg.length + 1);
    mbuf_consume_message (mbuf);
    break;
  case C_PROTOCOL_ERROR:
    exit (1);
  default:
    logerror ("'%s': unknown client state '%d'\n", source->name, self->state);
    mbuf_clear (mbuf);
    return;
  }

  pthread_mutex_lock (&self->mutex);

  int available = self->recv_buffer->max_length - self->recv_buffer->current_size ;

  if (self->file != NULL) {
    if (available < buf_size) {
        fwrite(self->recv_buffer->buff,sizeof(char), self->recv_buffer->current_size, self->file);
        self->current_page += 1;
        self->recv_buffer->next = make_client_buffer(self->recv_buffer->max_length,
                                                     self->current_page);
        self->recv_buffer = self->recv_buffer->next;
    }
    memcpy(self->recv_buffer->current_pointer,  buf, buf_size);
    self->recv_buffer->current_pointer += buf_size;
    self->recv_buffer->current_size += buf_size;
  }

  if (proxyServer->state == ProxyState_SENDING)
    pthread_cond_signal (&self->condvar);

  pthread_mutex_unlock (&self->mutex);
}

/**
 * \brief Call back function when the status of the socket change
 * \param source the socket event
 * \param status the status of the socket
 * \param error the value of the error if there is
 * \param handle the Client handler structure
 */
void
status_callback(SockEvtSource *source, SocketStatus status, int error, void *handle)
{
  (void)error;
  switch (status) {
    case SOCKET_CONN_CLOSED: {
      Client* self = (Client*)handle;
      pthread_mutex_lock (&self->mutex);
      fwrite(self->recv_buffer->buff,sizeof(char), self->recv_buffer->current_size, self->file);
      fflush(self->file);
      fclose(self->file);
      self->file = NULL;
      /* signal the sender thread that this client closed the connection */
      self->recv_buffer = NULL;
      pthread_cond_signal (&self->condvar);
      pthread_mutex_unlock (&self->mutex);
      self->recv_socket_closed = 1;
      socket_close (source->socket);
      logdebug("socket '%s' closed\n", source->name);
      eventloop_socket_remove (source); // Note:  this free()'s source!
      break;
    default:
      break;
    }
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
