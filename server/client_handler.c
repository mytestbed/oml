/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
#include <validate.h>
#include "client_handler.h"
#include "schema.h"
#include "util.h"

#define DEF_TABLE_COUNT 10

/* XXX: This cannot be static anymore if we want to test it... */
void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size);

static void
status_callback(SockEvtSource* source, SocketStatus status, int errcode, void* handle);

const char *
client_state_to_s (CState state)
{
  static const char *states [] = {
    "C_HEADER",
    "C_BINARY_DATA",
    "C_TEXT_DATA",
    "C_PROTOCOL_ERROR"
  };
  return states[state];
}

/*
 *  Allocate data structures for the client's tables.
 *
 *  The following need to be allocated:
 *
 *  self->tables -- the DbTables themselves
 *  self->seq_no_offsets -- the seq_no offet of each table
 *  self->values_vectors -- values vectors -- one for each table
 *  self->values_vector_counts -- size of each of the values_vectors
 *
 *  There should be ntables of each of these.  For the size of each of
 *  the values_vectors[i], see client_realloc_values().
 *
 *  self->table_count is set to ntables at the end of this, as long as
 *  there are not already enough tables to accomodate ntables.
 */
int
client_realloc_tables (ClientHandler *self, int ntables)
{
  if (!self || ntables <= 0)
    return -1;

  if (ntables > self->table_count) {
    int error = 0;
    DbTable **new_tables = xrealloc (self->tables, ntables*sizeof(DbTable*));
    int *new_so = xrealloc (self->seqno_offsets, ntables*sizeof(int));
    OmlValue **new_vv = xrealloc (self->values_vectors, ntables*sizeof(OmlValue*));
    int *new_vv_counts = xrealloc (self->values_vector_counts, ntables * sizeof (int));

    if (!new_tables || !new_so || !new_vv || !new_vv_counts) {
      logerror ("Failed to allocate memory for %d more client tables (current %d)\n",
                (ntables - self->table_count), self->table_count);
      // Don't free anything because whatever got successfully xrealloc'd is still ok
      error = -1;
    }

    /* Keep whatever succeeded */
    if (new_tables) self->tables = new_tables;
    if (new_so) self->seqno_offsets = new_so;
    if (new_vv) self->values_vectors = new_vv;
    if (new_vv_counts) self->values_vector_counts = new_vv_counts;

    /* If the values vectors succeeded, we need to create the new ones */
    if (new_vv && new_vv_counts) {
      int i = 0;
      for (i = self->table_count; i < ntables; i++) {
        self->values_vectors[i] = xcalloc (DEF_NUM_VALUES, sizeof (OmlValue));
        if (self->values_vectors[i]) {
          self->values_vector_counts[i] = DEF_NUM_VALUES;
        } else {
          self->values_vector_counts[i] = -1;
          logerror ("Error:  could not allocate memory for values vector for table %d\n", i);
          error = -1;
        }
      }
    }
    if (!error)
      self->table_count = ntables;
    return error;
  }

  return 0;
}

/*
 *  (Re)allocate the values vector for the table with the given index,
 *  so that it is expanded or contracted to have nvalues elements.
 */
int
client_realloc_values (ClientHandler *self, int index, int nvalues)
{
  if (!self || index < 0 || index > self->table_count || nvalues <= 0)
    return -1;

  if (nvalues > self->values_vector_counts[index]) {
    OmlValue *new_values = xrealloc (self->values_vectors[index], nvalues * sizeof (OmlValue));
    if (!new_values)
      return -1;
    self->values_vectors[index] = new_values;
    self->values_vector_counts[index] = nvalues;
  }

  return 0;
}

/**
 * \brief Create a client handler and associates it with the socket
 * \param newSock the socket from which the client is connected
 * \param hostname
 * \param user
 */
void*
client_handler_new(Socket* new_sock)
{
  ClientHandler* self = xmalloc(sizeof(ClientHandler));
  if (!self) return NULL;

  self->state = C_HEADER;
  self->content = C_TEXT_DATA;
  self->mbuf = mbuf_create ();
  self->socket = new_sock;
  self->event = eventloop_on_read_in_channel(new_sock, client_callback,
                                             status_callback, (void*)self);
  strncpy (self->name, self->event->name, MAX_STRING_SIZE);
  return self;
}

void
client_handler_free (ClientHandler* self)
{
  if (self->database)
    database_release (self->database);
  if (self->tables)
    xfree (self->tables);
  if (self->seqno_offsets)
    xfree (self->seqno_offsets);
  mbuf_destroy (self->mbuf);
  int i, j;
  for (i = 0; i < self->table_count; i++) {
    for (j = 0; j < self->values_vector_counts[i]; j++) {
      if (self->values_vectors[i][j].type == OML_STRING_VALUE)
        free (self->values_vectors[i][j].value.stringValue.ptr);
    }
    xfree (self->values_vectors[i]);
  }
  xfree (self->values_vectors);
  xfree (self->values_vector_counts);
  if (self->sender_name)
    xfree (self->sender_name);
  free (self->socket);
  xfree (self);

  //  xmemreport ();
}

static int
validate_schema_names (struct schema *schema, char **invalid)
{
  if (schema == NULL || invalid == NULL)
    return 0;

  if (!validate_name (schema->name)) {
    *invalid = schema->name;
    return 0;
  }

  int i;
  for (i = 0; i < schema->nfields; i++) {
    if (!validate_name (schema->fields[i].name)) {
      *invalid = schema->fields[i].name;
      return 0;
    }
  }
  return 1;
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
    logerror ("'%s': Schema parsing failed; disconnecting client.\n", self->name);
    logerror ("'%s': Failed schema: %s\n", self->name, value);
    self->state = C_PROTOCOL_ERROR;
    return;
  }

  char *invalid = NULL;
  if (!validate_schema_names (schema, &invalid)) {
    logerror ("'%s': The following schema contained an invalid name '%s'\n", self->name, invalid);
    logerror ("'%s': Failed schema: %s\n", self->name, value);
    self->state = C_PROTOCOL_ERROR;
    schema_free (schema);
    return;
  }

  int index = schema->index;
  DbTable* table = database_find_or_create_table(self->database, schema);
  if (table == NULL) {
    logerror("'%s': Can't find table '%s' or client schema doesn't match the existing table.\n",
             self->name, schema->name);
    logerror("'%s': Failed schema: %s\n", self->name, value);
    self->state = C_PROTOCOL_ERROR;
    schema_free (schema);
    return;
  }
  schema_free (schema);

  if (client_realloc_tables (self, index + 1) == -1) {
    logerror ("Failed to get memory for table index %d --> can't continue\n", index);
    return;
  }

  self->tables[index] = table;

  /* Reallocate the values vector if this schema has more columns than can fit already. */
  if (client_realloc_values (self, index, table->schema->nfields) == -1) {
    logwarn ("'%s': could not allocate values vector of size %d for table %d\n",
             self->name, table->schema->nfields, index);
  }
}

#define MAX_PROTOCOL_VERSION OML_PROTOCOL_VERSION
#define MIN_PROTOCOL_VERSION 1

/**
 * \brief Process a singel key/value pair contained in the header
 * \param self the client handler
 * \param key the key
 * \param value the value
 */
static void
process_meta(ClientHandler* self, char* key, char* value)
{
  chomp (value);
  logdebug("'%s': Meta <%s>:<%s>\n", self->name, key, value);
  if (strcmp(key, "protocol") == 0) {
    int protocol = atoi (value);
    if (protocol < MIN_PROTOCOL_VERSION || protocol > MAX_PROTOCOL_VERSION)
      {
        logerror("'%s': Client connected with incorrect protocol version (%d), <%s>\n",
                 self->name, protocol, value);
        logerror("'%s':    this oml2-server supports protocol versions %d and lower;\n",
                 self->name, MAX_PROTOCOL_VERSION);
        logerror("'%s':    maybe the client was built with a newer version of OML?\n",
                 self->name);
        self->state = C_PROTOCOL_ERROR;
        return;
      }
  } else if (strcmp(key, "experiment-id") == 0) {
    self->database = database_find(value);
    if (!self->database)
      self->state = C_PROTOCOL_ERROR;
  } else if (strcmp(key, "content") == 0) {
    if (strcmp(value, "binary") == 0) {
      self->content = C_BINARY_DATA;
    } else if (strcmp(value, "text") == 0) {
      self->content = C_TEXT_DATA;
    } else {
      logwarn("'%s': unknown content type '%s'\n", self->name, value);
    }
  } else if (strcmp(key, "app-name") == 0) {
    // IGNORE
    //strncpy(self->app_name, value, MAX_STRING_SIZE - 1);
  } else if (strcmp(key, "sender-id") == 0) {
    if (self->database == NULL) {
      logwarn("'%s': Meta 'sender-id' needs to come after 'experiment-id'.\n",
              self->name);
    } else {
      self->sender_id = self->database->add_sender_id(self->database, value);
      self->sender_name = xstrndup (value, strlen (value));
    }
  } else if (strcmp(key, "schema") == 0) {
    process_schema(self, value);
  } else if (strcmp(key, "start_time") == 0) {
    if (self->database == NULL) {
      logwarn("'%s': Meta 'start-time' needs to come after 'experiment-id'.\n",
              self->name);
    } else {
      long start_time = atol(value);
      if (self->database->start_time == 0) {
        // seed it with a time in the past
        self->database->start_time = start_time - 100;
      }
      self->time_offset = start_time - self->database->start_time;
    }
  } else {
    logwarn("'%s': Unknown meta info '%s' (%s) ignored\n", self->name, key, value);
  }
}

/**
 * \brief analyse the header
 * \param line_p a pointer to the line to read into the buffer
 * \param length_p a pointer to the length of the line
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
    mbuf_consume_message (mbuf);
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
  int index = header->stream;
  MBuffer* mbuf = self->mbuf;
  OmlValue *values = self->values_vectors[index];
  int count = self->values_vector_counts[index];
  int cnt = unmarshal_measurements(mbuf, header, values, count);

  /* Some error occurred in unmarshaling; can't continue */
  if (cnt < 0)
    return;

  double ts;

  ts = self->time_offset + header->timestamp;
  DbTable* table = self->tables[index];
  if (table == NULL) {
    logerror("Undefined table '%d'\n", index);
    self->state = C_PROTOCOL_ERROR;
    return;
  }
  logdebug("'%s': bin - sender '%s' insert into table %d, seq no: %d, ts: %f\n",
           self->name, self->sender_name, index, header->seqno);
  self->database->insert(self->database,
                         table,
                         self->sender_id,
                         header->seqno,
                         ts,
                         self->values_vectors[index],
                         cnt);

  mbuf_consume_message (mbuf);
}

/**
 * \brief analyse the data from the buffer
 * \param self the client handler
 * \param mbuf the buffer that contain the data
 * \return 1 when successfull, 0 otherwise
 */
static int
process_bin_message(ClientHandler* self, MBuffer* mbuf)
{
  OmlBinaryHeader header;

  unsigned char* sync = find_sync (mbuf_rdptr (mbuf),
                                   mbuf_remaining (mbuf));
  int sync_pos;
  if (sync == NULL)
    sync_pos = -1;
  else
    sync_pos = sync - mbuf->base;
  //logdebug("Received %d octets (sync at %d)\n", mbuf->fill, sync_pos);

  if (sync > mbuf->rdptr) {
    mbuf_read_skip (mbuf, sync - mbuf->rdptr);
    mbuf_consume_message (mbuf);
  }

  int res = unmarshal_init(mbuf, &header);
  if (res == 0) {
    logerror("An error occurred while reading binary message header\n");
    mbuf_clear (mbuf);
    self->state = C_PROTOCOL_ERROR;
    return 0;
  } else if (res < 0) {
    // not enough data
    logdebug("'%s': remaining bytes not enough to process a measurement\n",
             self->name, mbuf->fill);
    return 0;
  }
  switch (header.type) {
  case OMB_DATA_P:
  case OMB_LDATA_P:
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
  if (table_index >= self->table_count || table_index < 0) {
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
  OmlValue* v = self->values_vectors[table_index];
  char** val_ap = msg + 3;
  for (i= 0; i < schema->nfields; i++, v++, val_ap++) {
    char* val = *val_ap;
    v->type = schema->fields[i].type;
    if (oml_value_from_s (v, val) == -1)
      logerror("Error converting value of type %d from string '%s'\n", v->type, val);
  }

  logdebug("'%s': text - sender '%s' insert into table %d, seq no: %d, ts: %f\n",
           self->name, self->sender_name, index, seq_no, ts);
  self->database->insert(self->database, table, self->sender_id, seq_no,
                         ts, self->values_vectors[table_index], size - 3);
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
 * \param handle the client handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
void
client_callback(SockEvtSource* source, void* handle, void* buf, int buf_size)
{
  ClientHandler* self = (ClientHandler*)handle;
  MBuffer* mbuf = self->mbuf;

  logdebug("'%s': %s: received data, %d\n",
           source->name,
           client_state_to_s (self->state),
           buf_size);

  int result = mbuf_write (mbuf, buf, buf_size);

  if (result == -1) {
    logerror("'%s': Failed to write message from client into message buffer (mbuf_write())\n",
             source->name);
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
      // Protocol error:  close the client connection
      socket_close (self->socket);
      logerror("'%s': disconnecting client (fatal error)\n",
               source->name);
      eventloop_socket_release (self->event);
      client_handler_free (self);
      /*
       * Protocol error --> no need to repack buffer, so just return;
       */
      return;
    default:
      logerror("'%s': unknown client state '%d'\n", source->name, self->state);
      mbuf_clear (mbuf);
      return;
    }

  if (self->state == C_PROTOCOL_ERROR)
    goto process;

  // move remaining buffer content to beginning
  mbuf_repack_message (mbuf);
  logdebug("'%s': buffer repacked to %d bytes (end of this pass)\n",
           source->name, mbuf->fill);
}
/**
 * \brief Call back function when the status of the socket change
 * \param source the socket event
 * \param status the status of the socket
 * \param errno the value of the error if there is
 * \param handle the Client handler structure
 */
void
status_callback(SockEvtSource* source, SocketStatus status, int errcode, void* handle)
{
  logdebug("'%s': Socket status changed to %s(%d); error code is %d\n",
           source->name, socket_status_string (status), status, errcode);
  switch (status)
    {
    case SOCKET_WRITEABLE:
      break;
    case SOCKET_CONN_CLOSED:
      {
        /* Client closed the connection */
        loginfo("'%s': client closed socket connection\n", source->name);
        ClientHandler* self = (ClientHandler*)handle;
        socket_close (source->socket);
        eventloop_socket_release (self->event);
        client_handler_free (self);
        break;
      }
    case SOCKET_CONN_REFUSED:
      logdebug ("Unhandled condition CONN_REFUSED on socket '%s'\n", source->name);
      break;
    case SOCKET_DROPPED:
      logdebug ("Unhandled condition DROPPED on socket '%s'\n", source->name);
      break;
    case SOCKET_UNKNOWN:
    default:
      logdebug ("Unhandled condition UNKNOWN on socket '%s'\n", source->name);
      break;
    }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
