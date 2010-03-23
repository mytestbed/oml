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

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "proxy_client_handler.h"

#define DEF_TABLE_COUNT 10

extern ProxyServer* proxyServer;
/**
 * \fn ProxyClientBuffer* initPCB( int size, int number)
 * \brief Create and initialise a new +ProxyClientBuffer+ structure
 * \param size the maximum lenght of the buffer
 * \param number the page number
 * \return
 */
ProxyClientBuffer* initPCB( int size, int number){
    ProxyClientBuffer* self = ( ProxyClientBuffer*) malloc(sizeof(ProxyClientBuffer));
    memset(self, 0, sizeof(ProxyClientBuffer));
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
client_callback(SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
);

static void
status_callback(SockEvtSource* source,
  SocketStatus status,
  int err_no,  /* errno is not a good name as it expands to a fn ptr when unistd.h is included */
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

/**
 * \fn static void* thread_proxystart(void* handle)
 * \brief function that will try to send data to the real OML server
 * \param handle the proxy Client Handler
 */
static void* thread_proxystart(void* handle) {

  ProxyClientHandler* proxy = ( ProxyClientHandler* ) handle;
  //ProxyClientBuffer* buffer = proxy->currentBuffertoSend;
  while(1){
    ProxyClientBuffer* buffer = proxy->currentBuffertoSend;
  	if (strcmp(proxyServer->cmdSocket, "OMLPROXY-RESUME") == 0){
  	  //printf(" change %s\n",proxyServer->cmdSocket);
      //socket_sendto(proxy->socket_to_server,proxy->buffer->buff,proxy->buffer->currentSize);
      if((buffer->currentSize - buffer->byteAlreadySent)>0){


        if(socket_sendto(proxy->socket_to_server, (char*)buffer->buffToSend, (buffer->currentSize - buffer->byteAlreadySent))==0){
          buffer->buffToSend += (buffer->currentSize - buffer->byteAlreadySent);
          //printf(" test \n");
          buffer->byteAlreadySent = buffer->currentSize;
        }

      }
      if(buffer->next != NULL){

          proxy->currentBuffertoSend = buffer->next;

        }
       else{
         if(proxy->socketClosed == 1){
           socket_close(proxy->socket_to_server);
           proxy->cmdSocket = "OMLPROXY-STOP";
         }
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
  //printf(" cmdSocket %s\n",proxy->cmdSocket);


}
/**
 * \fn ProxyClientHandler* proxy_client_handler_new(Socket* newSock, int size_page, char* file_name, int portServer, char* addressServer)
 * \brief Create and initialise a +ProxyClientHandler+ structure
 * \param newSock the socket associated to the client transmition
 * \param size_page the size of the buffers
 * \param file_name the name of the file to save the measurements
 * \param portServer the destination port of the OML server
 * \param addressServer the address of the OML Server
 * \return a new ProxyClientHandler structure
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
  self->socketClosed = 0;

  self->file_name =  file_name;

  self->addressServer = addressServer;

  self->portServer = portServer;

  self->socket_to_server =  socket_tcp_out_new(file_name, addressServer, portServer);;
  pthread_create(&self->thread_pch, NULL, thread_proxystart, (void*)self);
  return self;
  //eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)self);
}

void
startLoopChannel(Socket* newSock, ProxyClientHandler* proxy){
  eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)proxy);
}

/**
 * \fn void client_callback(SockEvtSource* source, void* handle, void* buf,int buf_size)
 * \brief function called when the socket receive some data
 * \param source the socket event
 * \param handle the cleint handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
void
client_callback(
  SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
) {
  ProxyClientHandler* self = (ProxyClientHandler*)handle;
  int available = self->buffer->max_length - self->buffer->currentSize ;

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
 * \fn void status_callback( SockEvtSource* source, SocketStatus status, int errno, void* handle)
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
      ProxyClientHandler* self = (ProxyClientHandler*)handle;
      fwrite(self->buffer->buff,sizeof(char), self->buffer->currentSize, self->file);
      fflush(self->file);
      fclose(self->file);
      self->socketClosed = 1;
      while(strcmp(self->cmdSocket, "OMLPROXY-STOP") != 0)
        sleep(1);
      o_log(O_LOG_DEBUG, "socket '%s' closed\n", source->name);
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
