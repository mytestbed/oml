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
#include <headers.h>
#include <mbuf.h>
#include <message.h>

struct _clientBuffer;

struct _client;

#define MAX_STRING_SIZE 64

typedef struct _clientBuffer{
    int page_number;
    unsigned char* current_pointer;
    unsigned char* buff_to_send;
    int max_length;
    int current_size;
    int bytes_already_sent;
    unsigned char* buff;
    struct _clientBuffer* next;
} ClientBuffer;

enum ContentType {
  CONTENT_NONE,
  CONTENT_BINARY,
  CONTENT_TEXT
};

enum ClientState {
  C_HEADER,
  C_CONFIGURE,
  C_DATA,
  C_PROTOCOL_ERROR
};

typedef struct _client {
  //! Name used for debugging
  char        name[64];
  int         sender_id;
  char*       experiment_id;

  /*
   * The following data members are manipulated without locking in the
   * main thread; they should not be modified from the reader thread.
   *
   * state, content, mbuf, and msg_start should only be manipulated in
   * the main thread.  headers and header_table can safely be read
   * from the other threads without locking if state == C_DATA.
   */
  enum ClientState state;
  enum ClientState content;
  struct header *headers;
  struct header *header_table[H_max];
  MBuffer *mbuf;
  msg_start_fn msg_start; // Pointer to function for reading message boundaries

  Socket*     recv_socket;
  Socket*     send_socket;
  int         recv_socket_closed;
  int         send_socket_closed;

  /*
   * All the data members below must be locked and signalled properly
   * using the mutex and condvar
   */
  int         bytes_sent;
  int         current_page;
  struct _clientBuffer* first_buffer;
  struct _clientBuffer* recv_buffer;
  struct _clientBuffer* send_buffer;

  FILE *      file;
  int         fd_file;
  char*       file_name;

  pthread_t       thread;
  pthread_mutex_t mutex;
  pthread_cond_t  condvar;

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
  struct _client* first_client;

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
