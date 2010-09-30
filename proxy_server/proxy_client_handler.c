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
#include "client.h"

#define DEF_TABLE_COUNT 10

static void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

/* errno is not a good name for an variable as it expands to a fn ptr
   when unistd.h is included */
static void
status_callback(SockEvtSource* source, SocketStatus status, int err_no,
                void* handle);

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
client_callback(SockEvtSource *source, void *handle, void *buf, int buf_size)
{
  (void)source;
  Client* self = (Client*)handle;

  logdebug ("'%s': received %d octets of data\n", source->name, buf_size);
  logdebug ("'%s': %s\n", source->name, to_octets (buf, buf_size));

  proxy_message_loop (source->name, self, buf, buf_size);

#if 0
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
#endif
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
