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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>
#include <oml2/oml_writer.h>

#include <marshal.h>
#include <mbuf.h>
#include "client_handler.h"
#include "util.h"

#define DEF_TABLE_COUNT 10

static void
client_callback(SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
);

static void
status_callback(SockEvtSource* source,
  SocketStatus status,
  int errno,
  void* handle
);


/**
 * \brief Create a client handler and associates it with the socket
 * \param newSock the socket from which the client is connected
 * \param hostname
 * \param user
 */
void*
client_handler_new(
		   Socket* newSock,
		   char* hostname,
		   char* user
) {
  ClientHandler* self = (ClientHandler *)malloc(sizeof(ClientHandler));
  if (!self) return NULL;
  memset(self, 0, sizeof(ClientHandler));
  self->value_count = DEF_NUM_VALUES;
  self->values = (OmlValue*)malloc (self->value_count * sizeof (OmlValue));
  if (!self->values)
	{
	  free (self);
	  return NULL;
	}
  memset (self->values, 0, self->value_count * sizeof (OmlValue));
  self->state = C_HEADER;
  self->content = C_TEXT_DATA;
  self->mbuf = mbuf_create ();
  self->socket = newSock;
  self->DbHostname = hostname;
  self->DbUser = user;
  eventloop_on_read_in_channel(newSock, client_callback, status_callback, (void*)self);
  return self;
}

void
client_handler_free (ClientHandler* self)
{
  if (self->database)
	database_release (self->database);
  if (self->tables)
	free (self->tables);
  if (self->seq_no_offset)
	free (self->seq_no_offset);
  mbuf_destroy (self->mbuf);
  int i = 0;
  for (i = 0; i < self->value_count; i++)
	{
	  if (self->values[i].type == OML_STRING_VALUE)
		free (self->values[i].value.stringValue.ptr);
	}
  free (self->values);
  free (self);

  // FIXME:  What about destroying the socket data structure --> memory leak?
}

/**
 * \brief Process the data and put value inside the database
 * \param self the clienat handler
 * \param value the value to put in the database
 */
void
process_schema(
  ClientHandler* self,
  char* value
) {
  char* p = value;
  while (!(*p == ' ' || *p == '\0')) p++;
  if (*p == '\0') {
    o_log(O_LOG_ERROR, "While parsing 'schema'. Can't find index (%s)\n", value);
	self->state = C_PROTOCOL_ERROR;
    return;
  }
  *(p++) = '\0';
  int index = atoi(value);
  DbTable* t = database_get_table(self->database, p);
  if (t == NULL)
	{
	  o_log(O_LOG_ERROR, "While parsing schema '%s'.  Can't find table '%s' or the client declared a schema that doesn't match the previous declaration.\n", value, p);
	  self->state = C_PROTOCOL_ERROR;
	  return;   // error parsing schema
	}

  if (index >= self->table_size) {
    DbTable** old = self->tables;
	int* old_seq_no_offset = self->seq_no_offset;
    int old_count = self->table_size;

    self->table_size += DEF_TABLE_COUNT;
    self->tables = (DbTable**)malloc(self->table_size * sizeof(DbTable*));
	self->seq_no_offset = (int*)malloc(self->table_size * sizeof (int));
    int i;
    for (i = old_count - 1; i >= 0; i--) {
      self->tables[i] = old[i];
	  self->seq_no_offset[i] = old_seq_no_offset[i];
    }
  }
  self->tables[index] = t;
  // FIXME:  schema must come after sender-id
  // Look up the max sequence number for this sender in this table
  self->seq_no_offset[index] = self->database->get_max_seq_no (self->database, t, self->sender_id);

  /* Reallocate the values vector if this schema has more columns than can fit already. */
  if (t->col_size > self->value_count)
	{
	  free (self->values);
	  self->value_count = t->col_size + DEF_NUM_VALUES;
	  self->values = (OmlValue*) malloc (self->value_count * sizeof (OmlValue));
	  if (self->values == NULL)
		{
		  o_log (O_LOG_WARN, "Could not allocate values vector with %d elements\n",
				 self->value_count);
		}
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
  o_log(O_LOG_DEBUG, "Meta <%s>:<%s>\n", key, value);
  if (strcmp(key, "protocol") == 0) {
	int protocol = atoi (value);
	if (protocol != OML_PROTOCOL_VERSION)
	  {
		o_log (O_LOG_ERROR, "Client connected with incorrect protocol version (%d), <%s>\n", protocol, value);
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
    o_log(O_LOG_WARN, "Unknown meta info '%s' (%s) ignored\n", key, value);
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
	  o_log(O_LOG_ERROR, "Malformed meta line in header: <%s>\n", line);
	  self->state = C_PROTOCOL_ERROR;
	}

  // process_meta() might have signalled protocol error, so we have to check here.
  if (self->state == C_PROTOCOL_ERROR)
	return 0;
  else
	return 1; // still in header
}
/**
 * \brief process a subset of the data
 * \param selfthe client handler
 */
static void
process_bin_data_message(
  ClientHandler* self,
  OmlBinaryHeader* header
) {
  MBuffer* mbuf = self->mbuf;
  int cnt = unmarshal_measurements(mbuf, header, self->values, self->value_count);

  /* Some error occurred in unmarshaling; can't continue */
  if (cnt < 0)
	return;

  double ts;

  ts = self->time_offset + header->timestamp;

  if (header->stream >= self->table_size || header->stream < 0) {
    o_log(O_LOG_ERROR, "Table index '%d' out of bounds\n", header->stream);
	self->state = C_PROTOCOL_ERROR;
    return;
  }
  DbTable* table = self->tables[header->stream];
  if (table == NULL) {
    o_log(O_LOG_ERROR, "Undefined table '%d'\n", header->stream);
	self->state = C_PROTOCOL_ERROR;
    return;
  }
  o_log(O_LOG_DEBUG, "bin_data - CALLING insert for seq no: %d \n", header->seqno);
  self->database->insert(self->database,
						 table,
						 self->sender_id,
						 header->seqno + self->seq_no_offset[header->stream],
						 ts,
						 self->values,
						 cnt);

  mbuf_consume_message (mbuf);
  //o_log(O_LOG_DEBUG, "Received %d values\n", cnt);
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
  char* octets_str = to_octets (mbuf->base, mbuf->fill);
  //o_log (O_LOG_DEBUG, "Received %d octets (sync at %d):\t%s\n", mbuf->fill, sync_pos, octets_str);
  o_log (O_LOG_DEBUG, "Received %d octets (sync at %d)\n", mbuf->fill, sync_pos);
  free (octets_str);

  int res = unmarshal_init(mbuf, &header);
  //  int res = -1;
  if (res == 0) {
    o_log(O_LOG_ERROR, "An error occurred while reading binary message header\n");
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
      o_log(O_LOG_ERROR, "Unsupported message type '%d'\n", header.type);
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
process_text_data_message(
  ClientHandler* self,
  char** msg,
  int    size
) {
  if (size < 3) {
    o_log(O_LOG_ERROR, "Not enough parameters in text data message\n");
    return;
  }

  double ts = atof(msg[0]);
  long table_index = atol(msg[1]);
  long seq_no = atol(msg[2]);

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

  if (table->col_size != size - 3) {
    o_log(O_LOG_ERROR, "Data item mismatch for table '%s'\n", table->name);
    return;
  }

  int i;
  DbColumn** cols = table->columns;
  OmlValue* v = self->values;
  char** val_ap = msg + 3;
  for (i= 0; i < table->col_size; i++, cols++, v++, val_ap++) {
    DbColumn* col = (*cols);
    char* val = *val_ap;

    switch(col->type) {
    case OML_LONG_VALUE: v->value.longValue = atol(val); break;
    case OML_DOUBLE_VALUE: v->value.doubleValue = (double)atof(val); break;
    case OML_STRING_VALUE: v->value.stringValue.ptr = val; break;
    default:
      o_log(O_LOG_ERROR, "Bug: Unknown type %d in col '%s'\n",
	    col->type, col->name);
    }
    v->type = col->type;
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
	o_log(O_LOG_ERROR, "Too many parameters in data message <%s>\n", line);
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
void
client_callback(
  SockEvtSource* source,
  void* handle,
  void* buf,
  int buf_size
) {
  ClientHandler* self = (ClientHandler*)handle;
  MBuffer* mbuf = self->mbuf;

  int result = mbuf_write (mbuf, buf, buf_size);

  if (result == -1)
	{
	  o_log (O_LOG_ERROR, "Failed to write message from client into message buffer (mbuf_write())\n");
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
      o_log(O_LOG_ERROR, "Client: %s: unknown client state '%d'\n", source->name, self->state);
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
status_callback(
 SockEvtSource* source,
 SocketStatus status,
 int errno,
 void* handle
) {
  o_log(O_LOG_DEBUG, "Socket status changed to %s(%d) on source '%s'; error code is %d\n",
		socket_status_string (status),
		status,
		source->name,
		errno);
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

		o_log(O_LOG_DEBUG, "socket '%s' closed\n", source->name);
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
*/
