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
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "proxy_client_handler.h"

#define DEF_TABLE_COUNT 10

extern ProxyServer* proxyServer;

/**
 * \brief Create and initialise a new +ClientBuffer+ structure
 * \param size the maximum lenght of the buffer
 * \param number the page number
 * \return
 */
ClientBuffer* make_client_buffer( int size, int number)
{
    ClientBuffer* self = (ClientBuffer*) malloc(sizeof(ClientBuffer));
    memset(self, 0, sizeof(ClientBuffer));
    self->max_length = size;
    self->pageNumber = number;
    self->currentSize = 0;
    self->byteAlreadySent = 0;
    self->buff = (unsigned char*) malloc(size*sizeof(char));

    memset(self->buff, 0, size*sizeof(char));
    self->buffToSend = self->buff;
    self->current_pointer = self->buff;
    self->next = NULL;

    return self;
}


static void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

/* errno is not a good name for an variable as it expands to a fn ptr
   when unistd.h is included */
static void
status_callback(SockEvtSource* source, SocketStatus status, int err_no,
                void* handle);

/**
 * \brief function that will try to send data to the real OML server
 * \param handle the proxy Client Handler
 */
static void* client_send_thread (void* handle)
{
  Client *client = (Client*)handle;

  while(1) {
    ClientBuffer* buffer = client->currentBuffertoSend;
    switch (proxyServer->state) {
    case ProxyState_SENDING:
      if ((buffer->currentSize - buffer->byteAlreadySent) > 0) {
        if(socket_sendto(client->send_socket,
                         (char*)buffer->buffToSend,
                         (buffer->currentSize - buffer->byteAlreadySent))==0) {
          buffer->buffToSend += (buffer->currentSize - buffer->byteAlreadySent);
          buffer->byteAlreadySent = buffer->currentSize;
        }
      }
      if(buffer->next != NULL){
        client->currentBuffertoSend = buffer->next;
      } else {
        if (client->recv_socket_closed == 1) {
          socket_close (client->send_socket);
          client->send_socket_closed = 1;
        }
      }
      break;
    case ProxyState_STOPPED:
      break;
    case ProxyState_PAUSED:
      sleep (1);
      break;
    default:
      logerror ("Unknown ProxyState %d\n", proxyServer->state);
    }
  }
}

/**
 * \brief Create and initialise a +Client+ structure
 * \param newSock the socket associated to the client transmition
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

  self->buffer = make_client_buffer (page_size, 0); //TODO change it to integrate option of the command line

  self->firstBuffer = self->buffer;
  self->currentBuffertoSend = self->buffer;
  self->currentPageNumber = 0;

  self->file = fopen(file_name, "wa");
  self->file_name =  file_name;

  self->recv_socket = client_sock;
  self->recv_socket_closed = 0;

  self->send_socket =  socket_tcp_out_new(file_name, server_address, server_port);
  self->send_socket_closed = 0;

  pthread_create(&self->thread, NULL, client_send_thread, (void*)self);

  return self;
  //eventloop_on_read_in_channel(client_sock, client_callback, status_callback, (void*)self);
}

void
client_socket_monitor (Socket* client_sock, Client* client)
{
  eventloop_on_read_in_channel(client_sock, client_callback, status_callback, (void*)client);
}

/**
 * \brief function called when the socket receive some data
 * \param source the socket event
 * \param handle the cleint handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size)
{
  Client* self = (Client*)handle;
  int available = self->buffer->max_length - self->buffer->currentSize ;

  if (self->file != NULL) {
    if (available < buf_size) {
        fwrite(self->buffer->buff,sizeof(char), self->buffer->currentSize, self->file);
        self->currentPageNumber += 1;
        self->buffer->next = make_client_buffer(self->buffer->max_length,
                                                self->currentPageNumber);
        self->buffer = self->buffer->next;
    }
    memcpy(self->buffer->current_pointer,  buf, buf_size);
    self->buffer->current_pointer += buf_size;
    self->buffer->currentSize += buf_size;
  }
}

/**
 * \brief Call back function when the status of the socket change
 * \param source the socket event
 * \param status the status of the socket
 * \param errno the value of the error if there is
 * \param handle the Client handler structure
 */
void
status_callback(
 SockEvtSource* source,
 SocketStatus status,
 int err_no,
 void* handle
) {
  switch (status) {
    case SOCKET_CONN_CLOSED: {
      Client* self = (Client*)handle;
      fwrite(self->buffer->buff,sizeof(char), self->buffer->currentSize, self->file);
      fflush(self->file);
      fclose(self->file);
      /* signal the sender thread that this client closed the connection */
      self->recv_socket_closed = 1;
      logdebug("socket '%s' closed\n", source->name);
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
