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
/*!\file client_handler.c
  \brief Deals with a single connected client.
*/

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>
#include <oml2/oml_writer.h>

#include <log.h>
#include <mem.h>
#include <marshal.h>
#include <mbuf.h>
#include <oml_value.h>
#include "client_handler.h"
#include "schema.h"
#include "util.h"

#define DEF_TABLE_COUNT 10

static void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

static void
status_callback(SockEvtSource* source, SocketStatus status, int errno, void* handle);

/**
 * \brief Create a client handler and associates it with the socket
 * \param newSock the socket from which the client is connected
 * \param hostname
 * \param user
 */
void*
client_handler_new(Socket* new_sock, char* hostname, char* user)
{
  ClientHandler* self = xmalloc(sizeof(ClientHandler));
  if (!self) return NULL;
  self->value_count = DEF_NUM_VALUES;
  self->values = (OmlValue*)xcalloc (self->value_count, sizeof (OmlValue));
  if (!self->values)
    {
      xfree (self);
      return NULL;
    }
  self->state = C_HEADER;
  self->content = C_TEXT_DATA;
  self->mbuf = mbuf_create ();
  self->socket = new_sock;
  self->DbHostname = hostname;
  self->DbUser = user;
  eventloop_on_read_in_channel(new_sock, client_callback, status_callback, (void*)self);
  return self;
}

void
client_handler_free (ClientHandler* self)
{
  if (self->database)
    database_release (self->database);
  if (self->tables)
    xfree (self->tables);
  if (self->seq_no_offset)
    xfree (self->seq_no_offset);
  mbuf_destroy (self->mbuf);
  int i = 0;
  for (i = 0; i < self->value_count; i++)
    {
      if (self->values[i].type == OML_STRING_VALUE)
        free (self->values[i].value.stringValue.ptr);
    }
  xfree (self->values);
  xfree (self);

  // FIXME:  What about destroying the socket data structure --> memory leak?
}

/**
 * \brief Process the data and put value inside the database
 * \param self the clienat handler
 * \param value the value to put in the database
 */
void
process_schema(ClientHandler* self, char* value)
{
  struct schema *schema = schema_from_meta (value);
  if (!schema) {
    logerror ("Schema parsing failed; disconnecting client.\n");
    logerror ("Failed schema: %s\n", value);
    self->state = C_PROTOCOL_ERROR;
    return;
  }
  int index = schema->index;
  DbTable* table = database_find_or_create_table(self->database, schema);
  schema_free (schema);
  if (table == NULL) {
    logerror("Can't find table '%s' or client schema doesn't match the existing table.\n",
             table->schema->name);
    logerror("Failed schema: %s\n", value);
    self->state = C_PROTOCOL_ERROR;
    return;
  }

  if (index >= self->table_size) {
    size_t new_size = self->table_size + DEF_TABLE_COUNT;
    DbTable **new_tables = xrealloc (self->tables, new_size * sizeof (DbTable*));
    int *new_seq_no_offset = xrealloc (self->seq_no_offset, new_size * sizeof (int));

    if (!new_tables || !new_seq_no_offset) {
      logerror ("Failed to allocate memory for client table vector (size %d)", new_size);
      return;
    }
    self->table_size = new_size;
    self->tables = new_tables;
    self->seq_no_offset = new_seq_no_offset;
  }

  self->tables[index] = table;
  // FIXME:  schema must come after sender-id
  // Look up the max sequence number for this sender in this table
  self->seq_no_offset[index] = self->database->get_max_seq_no (self->database, table, self->sender_id);

  /* Reallocate the values vector if this schema has more columns than can fit already. */
  if (table->schema->nfields > self->value_count) {
    int new_count = table->schema->nfields + DEF_NUM_VALUES;
    OmlValue *new = xrealloc (self->values, new_count * sizeof (OmlValue));
    if (!new)
      logwarn("Could not allocate values vector with %d elements\n", new_count);
    self->value_count = new_count;
    self->values = new;
  }
}

/**
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
  chomp (value);
  logdebug("Meta <%s>:<%s>\n", key, value);
  if (strcmp(key, "protocol") == 0) {
    int protocol = atoi (value);
    if (protocol != OML_PROTOCOL_VERSION)
      {
        logerror("Client connected with incorrect protocol version (%d), <%s>\n", protocol, value);
        self->state = C_PROTOCOL_ERROR;
        return;
      }
  } else if (strcmp(key, "experiment-id") == 0) {
    self->database = database_find(value,self->DbHostname,self->DbUser);
  } else if (strcmp(key, "content") == 0) {
    if (strcmp(value, "binary") == 0) {
      self->content = C_BINARY_DATA;
    } else if (strcmp(value, "text") == 0) {
      self->content = C_TEXT_DATA;
    } else {
      logwarn("Unknown content type '%s'\n", value);
    }
  } else if (strcmp(key, "app-name") == 0) {
    // IGNORE
    //strncpy(self->app_name, value, MAX_STRING_SIZE - 1);
  } else if (strcmp(key, "sender-id") == 0) {
    if (self->database == NULL) {
      logwarn("Meta 'sender-id' needs to come after 'experiment-id'.\n");
    } else {
      self->sender_id = self->database->add_sender_id(self->database, value);
    }
  } else if (strcmp(key, "schema") == 0) {
    process_schema(self, value);
  } else if (strcmp(key, "start_time") == 0) {
    if (self->database == NULL) {
      logwarn("Meta 'start-time' needs to come after 'experiment-id'.\n");
    } else {
      long start_time = atol(value);
      if (self->database->start_time == 0) {
        // seed it with a time in the past
        self->database->start_time = start_time - 100;
      }
      self->time_offset = start_time - self->database->start_time;
    }
  } else {
    logwarn("Unknown meta info '%s' (%s) ignored\n", key, value);
  }
}

/**
 * \brief analyse the header
 * \param self the client handler
 * \param mbuf the buffer that contain the header and the data
 * \return 1 if successful, 0 otherwise
 */
static int
read_line(char** line_p, int* length_p, MBuffer* mbuf)
{
  uint8_t* line = mbuf_rdptr (mbuf);
  int length = mbuf_find (mbuf, '\n');

  // No newline found
  if (length == -1)
    return 0;

  *line_p = (char*)line;
  *length_p = length;
  *(line + length) = '\0';
  return 1;
}

/**
 * \brief analyse the header
 * \param self the client handler
 * \param mbuf the buffer that contain the header and the data
 * \return 1 if successful, 0 otherwise
 */
static int
process_header(ClientHandler* self, MBuffer* mbuf)
{
  char* line;
  int len;

  if (read_line(&line, &len, mbuf) == 0)
    return 0;

  if (len == 0) {
    // empty line denotes separator between header and body
    int skip_count = mbuf_find_not (mbuf, '\n');
    mbuf_read_skip (mbuf, skip_count + 1);
    self->state = self->content;
    return 0;
  }

  // separate key from value (check for ':')
  char* value = line;
  int count = 0;
  while (*(value) != ':' && count < len)
    {
      value++;
      count++;
    }

  if (*value == ':')
    {
      *value++ = '\0';
      while (*(value) == ' ' && count < len)
        {
          value++;
          count++;
        }
      mbuf_read_skip (mbuf, len + 1);
      process_meta(self, line, value);
    }
  else
    {
      logerror("Malformed meta line in header: <%s>\n", line);
      self->state = C_PROTOCOL_ERROR;
    }

  // process_meta() might have signalled protocol error, so we have to check here.
  if (self->state == C_PROTOCOL_ERROR)
    return 0;
  else
    return 1; // still in header
}

static void
process_bin_data_message(ClientHandler* self, OmlBinaryHeader* header)
{
  MBuffer* mbuf = self->mbuf;
  int cnt = unmarshal_measurements(mbuf, header, self->values, self->value_count);

  /* Some error occurred in unmarshaling; can't continue */
  if (cnt < 0)
    return;

  double ts;

  ts = self->time_offset + header->timestamp;

  if (header->stream >= self->table_size || header->stream < 0) {
    logerror("Table index '%d' out of bounds\n", header->stream);
    self->state = C_PROTOCOL_ERROR;
    return;
  }
  DbTable* table = self->tables[header->stream];
  if (table == NULL) {
    logerror("Undefined table '%d'\n", header->stream);
    self->state = C_PROTOCOL_ERROR;
    return;
  }
  logdebug("bin_data - CALLING insert for seq no: %d \n", header->seqno);
  self->database->insert(self->database,
                         table,
                         self->sender_id,
                         header->seqno + self->seq_no_offset[header->stream],
                         ts,
                         self->values,
                         cnt);

  mbuf_consume_message (mbuf);
  //logdebug("Received %d values\n", cnt);
}

/**
 * \brief analyse the data from the buffer
 * \param self the client handler
 * \param mbuf the buffer that contain the data
 * \return 1 when successfull, 0 otherwise
 */
static int
process_bin_message(
  ClientHandler* self,
  MBuffer*    mbuf
) {
  OmlBinaryHeader header;

  unsigned char* sync = find_sync (mbuf->base, mbuf->fill);
  int sync_pos;
  if (sync == NULL)
    sync_pos = -1;
  else
    sync_pos = sync - mbuf->base;
  //  char* octets_str = to_octets (mbuf->base, mbuf->fill);
  //logdebug("Received %d octets (sync at %d):\t%s\n", mbuf->fill, sync_pos, octets_str);
  logdebug("Received %d octets (sync at %d)\n", mbuf->fill, sync_pos);
  //xfree (octets_str);

  int res = unmarshal_init(mbuf, &header);
  //  int res = -1;
  if (res == 0) {
    logerror("An error occurred while reading binary message header\n");
    mbuf_clear (mbuf);
    self->state = C_PROTOCOL_ERROR;
    return 0;
  } else if (res < 0) {
    // not enough data
    return 0;
  }
  switch (header.type) {
    case OMB_DATA_P:
      process_bin_data_message(self, &header);
      break;
    default:
      logerror("Unsupported message type '%d'\n", header.type);
      self->state = C_PROTOCOL_ERROR;
      return 0;
  }

  return 1;
}

/**
 * \brief process a single measurement
 * \param self the client handler
 * \param msg a single message encoded in a string
 * \param length length of msg
 */
static void
process_text_data_message(ClientHandler* self, char** msg, int size)
{
  if (size < 3) {
    logerror("Not enough parameters in text data message\n");
    return;
  }

  double ts = atof(msg[0]);
  long table_index = atol(msg[1]);
  long seq_no = atol(msg[2]);

  ts += self->time_offset;
  if (table_index >= self->table_size || table_index < 0) {
    logerror("Table index '%d' out of bounds\n", table_index);
    return;
  }
  DbTable* table = self->tables[table_index];
  if (table == NULL) {
    logerror("Undefined table '%d'\n", table_index);
    return;
  }

  struct schema *schema = table->schema;
  if (schema->nfields != size - 3) {
    logerror("Data item mismatch for table '%s'\n", schema->name);
    return;
  }

  int i;
  OmlValue* v = self->values;
  char** val_ap = msg + 3;
  for (i= 0; i < schema->nfields; i++, v++, val_ap++) {
    char* val = *val_ap;
    v->type = schema->fields[i].type;
    if (oml_value_from_s (v, val) == -1)
      logerror("Error converting value of type %d from string '%s'\n", v->type, val);
  }

  self->database->insert(self->database, table, self->sender_id, seq_no,
                         ts, self->values, size - 3);
}

/**
 * \brief analyse the data from the buffer as text protocol
 * \param self the client handler
 * \param mbuf the buffer that contain the data
 * \return 1 when successfull, 0 otherwise
 */
static int
process_text_message(
  ClientHandler* self,
  MBuffer*    mbuf
) {
  char* line;
  int len;

  while (1) {
    if (read_line(&line, &len, mbuf) == 0) {
      return 0;
    }

    // split line into array
    char* a[DEF_NUM_VALUES];
    int    a_size = 0;
    char* p = line;
    int rem = len;

    while (rem > 0) {
      char* param = p;
      for (; rem > 0; rem--, p++) {
    if (*p == '\t') {
      *(p++) = '\0';
      rem--;
      break;
    }
      }
      a[a_size++] = param;
      if (a_size >= DEF_NUM_VALUES) {
    logerror("Too many parameters in data message <%s>\n", line);
    return 0;
      }
    }
    process_text_data_message(self, a, a_size);
  }
}

/**
 * \brief function called when the socket receive some data
 * \param source the socket event
 * \param handle the cleint handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
#include <stdio.h>
void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size)
{
  ClientHandler* self = (ClientHandler*)handle;
  MBuffer* mbuf = self->mbuf;

  int result = mbuf_write (mbuf, buf, buf_size);

  if (result == -1) {
    logerror("Failed to write message from client into message buffer (mbuf_write())\n");
    return;
  }

  process:
  switch (self->state)
    {
    case C_HEADER:
      while (process_header(self, mbuf));
      if (self->state != C_HEADER) {
        //finished header, let someone else process rest of buffer
        goto process;
      }
      break;

    case C_BINARY_DATA:
      while (process_bin_message(self, mbuf));
      break;

    case C_TEXT_DATA:
      while (process_text_message(self, mbuf));
      break;

    case C_PROTOCOL_ERROR:
      // Protocol error:  close the client connection and teardown all
      // of it's allocated data.
      socket_close (self->socket);
      client_handler_free (self);
      /*
       * The mbuf is also freed by client_handler_free(), and there's
       * no point repacking the buffer in that case, so just return.
       */
      return;
    default:
      logerror("Client: %s: unknown client state '%d'\n", source->name, self->state);
      mbuf_clear (mbuf);
      return;
    }

  // move remaining buffer content to beginning
  mbuf_repack_message (mbuf);
}
/**
 * \brief Call back function when the status of the socket change
 * \param source the socket event
 * \param status the status of the socket
 * \param errno the value of the error if there is
 * \param handle the Client handler structure
 */
void
status_callback(SockEvtSource* source, SocketStatus status, int errno, void* handle)
{
  logdebug("Socket status changed to %s(%d) on source '%s'; error code is %d\n",
           socket_status_string (status), status,
           source->name, errno);
  switch (status)
    {
    case SOCKET_WRITEABLE:
      break;
    case SOCKET_CONN_CLOSED:
      {
        /* Client closed the connection */
        ClientHandler* self = (ClientHandler*)handle;
        client_handler_free (self);
        socket_close (source->socket);

        logdebug("socket '%s' closed\n", source->name);
        break;
      }
    case SOCKET_CONN_REFUSED:
      break;
    case SOCKET_DROPPED:
      break;
    case SOCKET_UNKNOWN:
    default:
      break;
    }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
