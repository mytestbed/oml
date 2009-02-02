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


ProxyClientBuffer* initPCB( int size, int number){
    ProxyClientBuffer* self = ( ProxyClientBuffer*) malloc(sizeof(ProxyClientBuffer));
    memset(self, 0, sizeof(ProxyClientBuffer));
    self->sizeBuf = size;
    self->pageNumber = number;
    self->currentBufSize = 0;

    self->buff = (char*) malloc(size*sizeof(char));

    memset(self->buff, 0, size*sizeof(char));

    self->current_pointer = self->buff;
    self->next = NULL;

    return self;
}


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
    Socket* newSock,
    int size_page,
    char* file_name
) {
  ProxyClientHandler* self = (ProxyClientHandler *)malloc(sizeof(ProxyClientHandler));
  memset(self, 0, sizeof(ProxyClientHandler));

  self->socket = newSock;
  self->buffer = initPCB( size_page, 0); //TODO change it to integrate option of the command line
  self->firstBuffer = buffer;
  self->currentPageNumber = 0;
  self->queue = fopen(file_name, "wa");
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
  int available = self->buffer->sizeBuf - self->buffer->currentBufSize ;

  if(self->queue == NULL)
	  ;
  else{
    if(available >= buf_size){

        strncpy(self->buffer->current_pointer, (const char*) buf, buf_size);

        self->buffer->current_pointer += buf_size;
        self->buffer->currentBufSize += buf_size;

    }
    else{


        fwrite(self->buffer->buff,sizeof(char), self->buffer->currentBufSize, self->queue);
        fflush(self->queue);
        self->currentPageNumber += 1;
        self->buffer->next = initPCB(self->buffer->sizeBuf, self->currentPageNumber);
        self->buffer = self->buffer->next;

        strcpy(self->buffer->current_pointer, (const char*) buf);
        self->buffer->current_pointer += buf_size;
        self->buffer->currentBufSize = buf_size;

    }
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
       fwrite(self->buffer->buff,sizeof(char), self->buffer->currentBufSize, self->queue);
        fflush(self->queue);
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
