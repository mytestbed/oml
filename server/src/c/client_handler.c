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
/*!\file client_handler.c
  \brief Deals with a single connected client.
*/

#include <malloc.h>
#include <string.h>

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "marshall.h"
#include "client_handler.h"

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
/**
 * \fn void* client_handler_new(Socket* newSock)
 * \brief Create a client handler and associates it with the socket
 * \param newSock the socket from which the client is connected
 */
void*
client_handler_new(
    Socket* newSock
) {
  ClientHandler* self = (ClientHandler *)malloc(sizeof(ClientHandler));
  memset(self, 0, sizeof(ClientHandler));
  self->value_count = DEF_NUM_VALUES;
  self->state = C_HEADER;
  self->content = C_TEXT_DATA;
  self->socket = newSock;
  eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)self);
}
/**
 * \fn process_schema(ClientHandler* self,char* value)
 * \brief Process the data and put value inside the database
 * \param self the clienat handler
 * \param value the value to put in the database
 */
process_schema(
  ClientHandler* self,
  char* value
) {
  char* p = value;
  while (!(*p == ' ' || *p == '\0')) p++;
  if (*p == '\0') {
    o_log(O_LOG_ERROR, "While parsing 'schema'. Can't find index (%s)\n", value);
    return;
  }
  *(p++) = '\0';
  int index = atoi(value);
  DbTable* t = database_get_table(self->database, p);
  if (t == NULL) return;   // error parsing schema

  if (index >= self->table_size) {
    DbTable** old = self->tables;
    int old_count = self->table_size;

    self->table_size += DEF_TABLE_COUNT;
    self->tables = (DbTable**)malloc(self->table_size * sizeof(DbTable*));
    int i;
    for (i = old_count - 1; i >= 0; i--) {
      self->tables[i] = old[i];
    }
  }
  self->tables[index] = t;
}
/**
 * \fn static void process_meta(ClientHandler* self, char* key, char* value)
 * \brief Process a singel key/value pair contained in the header
 * \param self the client handler
 * \param key the key
 * \param value the value
 */

static void
process_meta(
  ClientHandler* self,
  char* key,
  char* value
) {
  o_log(O_LOG_DEBUG, "Meta <%s>:<%s>\n", key, value);
  if (strcmp(key, "protocol") == 0) {
    // TODO: Check protocol
  } else if (strcmp(key, "experiment-id") == 0) {
    self->database = database_find(value);
  } else if (strcmp(key, "content") == 0) {
    if (strcmp(value, "binary") == 0) {
      self->content = C_BINARY_DATA;
    } else if (strcmp(value, "text") == 0) {
      self->content = C_TEXT_DATA;
    } else {
      o_log(O_LOG_WARN, "Unknown content type '%s'\n", value);
    }
  } else if (strcmp(key, "app-name") == 0) {
    // IGNORE
    //strncpy(self->app_name, value, MAX_STRING_SIZE - 1);
  } else if (strcmp(key, "sender-id") == 0) {
    if (self->database == NULL) {
      o_log(O_LOG_WARN, "Meta 'sender-id' needs to come after 'experiment-id'.\n");
    } else {
      self->sender_id = self->database->add_sender_id(self->database, value);
    }
  } else if (strcmp(key, "schema") == 0) {
    process_schema(self, value);
  } else if (strcmp(key, "start_time") == 0) {
    if (self->database == NULL) {
      o_log(O_LOG_WARN, "Meta 'start-time' needs to come after 'experiment-id'.\n");
    } else {
      long start_time = atol(value);
      if (self->database->start_time == 0) {
        // seed it with a time in the past
        self->database->start_time = start_time - 100;
      }
      self->time_offset = start_time - self->database->start_time;
    }
  } else {
    o_log(O_LOG_WARN, "Unknown meta info '%s' (%s)\n", key, value);
  }
}
/**
 * \fn static int process_header(ClientHandler* self, OmlMBuffer* mbuf)
 * \brief analyse the header
 * \param self the client handler
 * \param mbuf the buffer that contain the header and the data
 * \return 1 if successful, 0 otherwise
 */
static int
process_header(
  ClientHandler* self,
  OmlMBuffer* mbuf
) {
  char* line = mbuf->curr_p;
  int remaining = mbuf->buffer_fill - (mbuf->curr_p - mbuf->buffer);
  while (remaining-- >= 0 && *(mbuf->curr_p++) != '\n');
  if (*(mbuf->curr_p - 1) != '\n') {
    // not enough data for entire line
    mbuf->curr_p = line;
    return 0;
  }
  int len = mbuf->curr_p - (unsigned char*)line;
  if (len == 1) {
    // empty line denotes separator between header and body
    while (*mbuf->curr_p == '\n') mbuf->curr_p++;  // skip additional empty lines
    self->state = self->content;
    return 0;
  }
  *(mbuf->curr_p - 1) = '\0';
  // separate key from value (check for ':')
  char* value = line;
  while (*(value++) != ':' && (void*)value < (void*)mbuf->curr_p);
  if (*(value - 1) == ':') {
    *(value - 1) = '\0';
    while (*(value++) == ' ' && (void*)value < (void*)mbuf->curr_p); // skip white space
    process_meta(self, line, value - 1);
 } else {
    o_log(O_LOG_WARN, "Illformated meta line in header: <%s>\n", line);
  }
  return 1; // still in header
}
/**
 * \fn static void process_data_message(ClientHandler* self)
 * \brief process a subset of the data
 * \param selfthe client handler
 */
static void
process_data_message(
  ClientHandler* self
) {
  OmlMBuffer* mbuf = &self->mbuf;
  int table_index;
  int seq_no;
  double ts;

  int cnt = unmarshall_measurements(mbuf, &table_index, &seq_no, &ts, self->values, self->value_count);
  ts += self->time_offset;
  if (table_index >= self->table_size || table_index < 0) {
    o_log(O_LOG_ERROR, "Table index '%d' out of bounds\n", table_index);
    return;
  }
  DbTable* table = self->tables[table_index];
  if (table == NULL) {
    o_log(O_LOG_ERROR, "Undefined table '%d'\n", table_index);
    return;
  }
  o_log(O_LOG_DEBUG, "TDEBUG - CALLING insert for seq no: %d \n", seq_no);
  self->database->insert(self->database, table, self->sender_id, seq_no, ts, self->values, cnt);
  //o_log(O_LOG_DEBUG, "Received %d values\n", cnt);

}
/**
 * \fn static int process_message( ClientHandler* self, OmlMBuffer*    mbuf)
 * \brief analyse the data from the buffer
 * \param self the client handler
 * \param mbuf the buffer that contain the data
 * \return 1 when successfull, 0 otherwise
 */
static int
process_message(
  ClientHandler* self,
  OmlMBuffer*    mbuf
) {
  OmlMsgType type;

  int res = unmarshall_init(mbuf, &type);
  if (res == 0) {
    o_log(O_LOG_ERROR, "OUT OF SYNC\n");
    mbuf->buffer_fill = 0;
    return;
  } else if (res < 0) {
    // not enough data
    return 0;
  }
  switch (type) {
    case OMB_DATA_P:
      process_data_message(self);
      break;
    default:
      o_log(O_LOG_ERROR, "Unsupported message type '%d'\n", type);
      // skip unknown message
      mbuf->curr_p += mbuf->buffer_remaining;
  }
  // move remaining buffer content to beginning
//  size_t remaining = mbuf->buffer_fill - (mbuf->curr_p - mbuf->buffer);
//  if (remaining > 0) {
//    memmove(mbuf->buffer, mbuf->curr_p, remaining);
//    mbuf->buffer_fill = remaining;
//  } else {
//    // nothing left
//    mbuf->buffer_fill = 0;
//  }
//  mbuf->curr_p = mbuf->buffer;
//  return remaining;
  return 1;
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
  ClientHandler* self = (ClientHandler*)handle;
  OmlMBuffer* mbuf = &self->mbuf;
  int available =  mbuf->buffer_length - mbuf->buffer_fill;
  if (available < buf_size) {
    marshall_resize(mbuf, mbuf->buffer_fill + buf_size);
  }
  memcpy(mbuf->buffer + mbuf->buffer_fill, buf, buf_size);
  mbuf->buffer_fill += buf_size;

  process:
  switch (self->state) {
    case C_HEADER:
      while (process_header(self, mbuf));
      if (self->state != C_HEADER) {
        //finished header, let someone else process rest of buffer
        goto process;
      }
      break;
    case C_BINARY_DATA:
      while (process_message(self, mbuf));
      break;
    default:
      o_log(O_LOG_ERROR, "Unknown client state '%d'\n", self->state);
      mbuf->curr_p = mbuf->buffer;
      mbuf->buffer_fill = 0; // reset data
      return;
  }


  // move remaining buffer content to beginning
  size_t remaining = mbuf->buffer_fill - (mbuf->curr_p - mbuf->buffer);
  if (remaining > 0) {
    memmove(mbuf->buffer, mbuf->curr_p, remaining);
    mbuf->buffer_fill = remaining;
  } else {
    // nothing left
    mbuf->buffer_fill = 0;
  }
  mbuf->curr_p = mbuf->buffer;
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
 int errno,
 void* handle
) {
  switch (status) {
    case SOCKET_CONN_CLOSED: {
      ClientHandler* self = (ClientHandler*)handle;
      database_release(self->database);
      free(self->tables);
      free(self);

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
