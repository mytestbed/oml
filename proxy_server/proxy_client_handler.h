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

#ifndef CLIENT_HANDLER_H_
#define CLIENT_HANDLER_H_

//#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdlib.h>
#include <errno.h>
#include <pthread.h>

#include <ocomm/o_socket.h>

struct _clientBuffer;

struct _client;

typedef enum _cstate {
  C_HEADER,       // processing header info
  C_BINARY_DATA,  // data is of binary format
  C_TEXT_DATA
} CState;

#define DEF_NUM_VALUES 30
#define MAX_STRING_SIZE 64

typedef struct _clientBuffer{
    int pageNumber;
    unsigned char* current_pointer;
    unsigned char* buffToSend;
    int max_length;
    int currentSize;
    int byteAlreadySent;
    unsigned char* buff;
    struct _clientBuffer* next;
} ClientBuffer;

typedef struct _client {
  //! Name used for debugging
  char        name[64];
  int         sender_id;
  Socket*     recv_socket;
  Socket*     send_socket;
  int         recv_socket_closed;
  int         send_socket_closed;

  int         currentPageNumber;
  struct _clientBuffer* buffer;
  struct _clientBuffer* firstBuffer;
  struct _clientBuffer* currentBuffertoSend;

  FILE *      file;
  int         fd_file;
  char*       file_name;

  pthread_t   thread;

  struct _client* next;
} Client;

enum ProxyState {
  ProxyState_PAUSED,
  ProxyState_SENDING,
  ProxyState_STOPPED
};

typedef struct _proxyServer{
  enum ProxyState state;

  int client_count;
  struct _client* first;

  int  server_port;
  char* server_address;
} ProxyServer;

ClientBuffer*
make_client_buffer (int size, int number);

Client*
client_new(Socket* sock, int page_size, char *file,
           int server_port, char *server_address);

void
client_socket_monitor(Socket* sock, Client* client);

#endif /*CLIENT_HANDLER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
