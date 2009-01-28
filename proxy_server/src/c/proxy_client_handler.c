/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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

#include <malloc.h>
#include <string.h>

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "marshall.h"
#include "proxy_client_handler.h"

#define DEF_TABLE_COUNT 10


static void
client_callback(SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
);

static void
status_callback(
  SockEvtSource* source,
  SocketStatus status,
  int errno,
  void* handle
);

void*
proxy_client_handler_new(
    Socket* newSock
) {
  ProxyClientHandler* self = (ProxyClientHandler *)malloc(sizeof(ProxyClientHandler));
  memset(self, 0, sizeof(ProxyClientHandler));
  self->value_count = DEF_NUM_VALUES;
  self->state = C_HEADER;
  self->content = C_TEXT_DATA;
  self->socket = newSock;
  self->queue = fopen("./testproxy", "wa");
  eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)self);
}

void
client_callback(
  SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
) {
  ProxyClientHandler* self = (ProxyClientHandler*)handle;
  OmlMBuffer* mbuf = &self->mbuf;
  int available =  mbuf->buffer_length - mbuf->buffer_fill;

  if(self->queue==NULL)
	  ;
  else{
    fputs(buf, self->queue);
    fflush(self->queue);
  }

}

void
status_callback(
 SockEvtSource* source,
 SocketStatus status,
 int errno,
 void* handle
) {
  switch (status) {
    case SOCKET_CONN_CLOSED: {
      ProxyClientHandler* self = (ProxyClientHandler*)handle;

      fclose(self->queue);

      o_log(O_LOG_DEBUG, "socket '%s' closed\n", source->name);
      break;
    }
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
