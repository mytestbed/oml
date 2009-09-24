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

#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_

//#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>


struct _proxyClientBuffer;

struct _proxyClientHandler;

typedef enum _cstate {
  C_HEADER,       // processing header info
  C_BINARY_DATA,  // data is of binary format
  C_TEXT_DATA
} CState;

#define DEF_NUM_VALUES 30
#define MAX_STRING_SIZE 64



typedef struct _proxyClientBuffer{
    int pageNumber;

    unsigned char* current_pointer;

    unsigned char* buffToSend;

    int max_length;

    int currentSize;

    int byteAlreadySent;

    unsigned char* buff;

    struct _proxyClientBuffer* next;


}ProxyClientBuffer;

typedef struct _proxyClientHandler {
  //! Name used for debugging
  char name[64];

  int         sender_id;

  Socket*     socket;

  int currentPageNumber;

  struct _proxyClientBuffer* buffer;
  struct _proxyClientBuffer* firstBuffer;

  struct _proxyClientBuffer* currentBuffertoSend;

  FILE *      file;

  int         fd_file;

  struct _proxyClientHandler* next;

  pthread_t thread_pch;

  char* cmdSocket;

  int socketClosed;

  char* file_name;

  int portServer;

  char* addressServer;

  Socket*     socket_to_server;

}ProxyClientHandler;

typedef struct _proxyServer{

  struct _proxyClientHandler* first;

  struct _proxyClientHandler* current;

  char* cmdSocket;

  pthread_t thread_stdin;

}ProxyServer;

ProxyClientBuffer* initPCB(
        int size,
        int number
        );

void setCommand( ProxyClientHandler* proxy, char* cmd );

ProxyClientHandler*
proxy_client_handler_new(
    Socket* newSock,
    int size_page,
    char* file_name,
    int portServer,
    char* addressServer
);

void
startLoopChannel(Socket* newSock, ProxyClientHandler* proxy);

int pauseConnection();

int resumeConnection();

int stopConnection();

int startConnection();

#endif /*CLIENT_HANDLER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
