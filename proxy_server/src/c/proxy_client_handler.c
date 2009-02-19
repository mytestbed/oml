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

extern ProxyServer* proxyServer;
/**
 * \fn
 * \brief
 * \param
 * \return
 */
ProxyClientBuffer* initPCB( int size, int number){
    ProxyClientBuffer* self = ( ProxyClientBuffer*) malloc(sizeof(ProxyClientBuffer));
    memset(self, 0, sizeof(ProxyClientBuffer));
    self->max_length = size;
    self->pageNumber = number;
    self->currentSize = 0;
    self->byteAlreadySent = 0;
    self->buff = (char*) malloc(size*sizeof(char));

    memset(self->buff, 0, size*sizeof(char));
    self->buffToSend = self->buff;
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

int pauseConnection(){
    return 0;
}

int resumeConnection(){
    return 0;
}

int stopConnection(){
    return 0;
}

int startConnection(){
    return 0;
}


static void* thread_proxystart(void* handle) {

  ProxyClientHandler* proxy = ( ProxyClientHandler* ) handle;
  //ProxyClientBuffer* buffer = proxy->currentBuffertoSend;
  while(1){
    ProxyClientBuffer* buffer = proxy->currentBuffertoSend;
  	if (strcmp(proxyServer->cmdSocket, "OMLPROXY-RESUME") == 0){
  	  //printf(" change %s\n",proxyServer->cmdSocket);
      //socket_sendto(proxy->socket_to_server,proxy->buffer->buff,proxy->buffer->currentSize);
      if((buffer->currentSize - buffer->byteAlreadySent)>0){
        if(socket_sendto(proxy->socket_to_server, buffer->buffToSend, (buffer->currentSize - buffer->byteAlreadySent))==0){
          buffer->buffToSend += (buffer->currentSize - buffer->byteAlreadySent);
          //printf(" test \n");
          buffer->byteAlreadySent = buffer->currentSize;
        }

      }
      if(buffer->next != NULL){
          //printf(" change %s\n",proxyServer->cmdSocket);
          proxy->currentBuffertoSend = buffer->next;

        }
      //sleep(1);

  	}else if (strcmp(proxyServer->cmdSocket, "OMLPROXY-STOP") == 0){
  		//printf(" change %s\n",proxyServer->cmdSocket);

  	}else if (strcmp(proxyServer->cmdSocket, "OMLPROXY-PAUSE") == 0){
  	  //printf(" change %s\n",proxyServer->cmdSocket);
  	  sleep(1);
  	}

  }

}

void setCommand( ProxyClientHandler* proxy, char* cmd ){

  proxy->cmdSocket= cmd;
  printf(" cmdSocket %s\n",proxy->cmdSocket);


}
/**
 * \fn
 * \brief
 * \param
 * \return
 */
ProxyClientHandler*
proxy_client_handler_new(
    Socket* newSock,
    int size_page,
    char* file_name,
    int portServer,
    char* addressServer
) {
  ProxyClientHandler* self = (ProxyClientHandler *)malloc(sizeof(ProxyClientHandler));
  memset(self, 0, sizeof(ProxyClientHandler));

  self->socket = newSock;
  self->buffer = initPCB( size_page, 0); //TODO change it to integrate option of the command line
  self->firstBuffer = self->buffer;
  self->currentBuffertoSend = self->buffer;
  self->currentPageNumber = 0;
  self->file = fopen(file_name, "wa");
  self->cmdSocket = "pause";

  self->socket_to_server = socket_tcp_out_new(file_name,"localhost",3003);
  pthread_create(&self->thread_pch, NULL, thread_proxystart, (void*)self);
  return self;
  //eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)self);
}

void*
startLoopChannel(Socket* newSock, ProxyClientHandler* proxy){
  eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)proxy);
}

/**
 * \fn
 * \brief
 * \param
 * \return
 */
void
client_callback(
  SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
) {
  ProxyClientHandler* self = (ProxyClientHandler*)handle;
  OmlMBuffer* mbuf = &self->mbuf;
  int available = self->buffer->max_length - self->buffer->currentSize ;
  char * c = "\n";
  if(self->file == NULL)
	  ;
  else{
    if(available < buf_size){
        fwrite(self->buffer->buff,sizeof(char), self->buffer->currentSize, self->file);
        //fflush(self->queue);
        //socket_sendto(self->socket_to_server,self->buffer->buff,self->buffer->currentSize);

        self->currentPageNumber += 1;
        self->buffer->next = initPCB(self->buffer->max_length, self->currentPageNumber);
        self->buffer = self->buffer->next;

        //memcpy(self->buffer->current_pointer, buf, buf_size);

        //self->buffer->current_pointer += buf_size;
        //self->buffer->currentSize += buf_size;

    }



    memcpy(self->buffer->current_pointer,  buf, buf_size);
    self->buffer->current_pointer += buf_size;
    self->buffer->currentSize += buf_size;


  }

}

/**
 * \fn
 * \brief
 * \param
 * \return
 */
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
      fwrite(self->buffer->buff,sizeof(char), self->buffer->currentSize, self->file);
      fflush(self->file);
      fclose(self->file);
      while(strcmp(proxyServer->cmdSocket, "OMLPROXY-STOP") != 0)
        sleep(1);
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
