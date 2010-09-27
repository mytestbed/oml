#ifndef CLIENT_H__
#define CLIENT_H__

#include <stdio.h>
#include <pthread.h>
#include <mbuf.h>
#include <cbuf.h>
#include <headers.h>
#include <message.h>

#include <ocomm/o_socket.h>

#include "message_queue.h"

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

struct _clientBuffer;
struct _client;
struct _session;

typedef struct _client {
  //! Name used for debugging
  char        name[64];
  int         sender_id;
  char*       experiment_id;
  struct _session *session;

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
  struct msg_queue *messages;

  Socket*     recv_socket;
  Socket*     send_socket;
  int         recv_socket_closed;
  int         send_socket_closed;

  int    locking; // True if the client is using locking; mainly for testing (set locking = 0);

  /*
   * All the data members below must be locked and signalled properly
   * using the mutex and condvar
   */
  CBuffer    *cbuf;

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

Client* client_new (Socket* client_sock, int page_size, char* file_name,
                    int server_port, char* server_address);
void client_free (Client *client);

void free_client_buffer (ClientBuffer *buffer);
ClientBuffer* make_client_buffer (int size, int number);




#endif /* CLIENT_H__ */
