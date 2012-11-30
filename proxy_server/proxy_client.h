/*
 * Copyright 2010-2012 National ICT Australia (NICTA), Australia
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

#ifndef CLIENT_H__
#define CLIENT_H__

#include <stdio.h>
#include <pthread.h>
#include <mbuf.h>
#include <cbuf.h>
#include <headers.h>
#include <message.h>

#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>
#include "message_queue.h"

enum ContentType {
  CONTENT_NONE,
  CONTENT_BINARY,
  CONTENT_TEXT
};

enum ClientState {
  C_HEADER,
  C_CONFIGURE,
  C_DATA,
  C_PROTOCOL_ERROR,
  C_DISCONNECTED
};

struct _client;
struct _session;

typedef struct _client {
  //! Name used for debugging
  char        name[64];
  int         sender_id;
  char*       domain;
  char*       downstream_addr;
  int         downstream_port;
  struct _session *session;

  /*
   * The following data members are manipulated without locking in the
   * main thread; they should not be modified from the sender thread.
   *
   * state, content, mbuf, and msg_start should only be manipulated in
   * the main thread.  headers and header_table can safely be read
   * from the other threads without locking if state == C_DATA.
   */
  enum ClientState state;
  enum ContentType content;
  struct header *headers;
  struct header *header_table[H_max];
  MBuffer *mbuf;
  msg_start_fn msg_start; // Pointer to function for reading message boundaries

  SockEvtSource *recv_event;
  Socket*     recv_socket;
  int         send_socket;
  int         sender_connected;

  /*
   * All the data members below must be locked and signalled properly
   * using the mutex and condvar
   */
  struct msg_queue *messages;
  CBuffer    *cbuf;

  FILE *      file;
  int         fd_file;
  char*       file_name;

  pthread_t       thread; // The sender thread, see sender.c
  pthread_mutex_t mutex;
  pthread_cond_t  condvar;

  struct _client* next;
} Client;

Client* client_new (Socket* client_sock, int page_size, char* file_name,
                    int server_port, char* server_address);
void client_free (Client *client);


#endif /* CLIENT_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
