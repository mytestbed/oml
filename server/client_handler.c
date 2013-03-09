/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "oml2/oml_writer.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"
#include "mem.h"
#include "mbuf.h"
#include "oml_value.h"
#include "oml_util.h"
#include "validate.h"
#include "marshal.h"
#include "binary.h"
#include "schema.h"
#include "client_handler.h"

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

/**  Allocate data structures for the ClientHandler's tables.
 *
 *  (Re)Allocate the following:
 *  * self->tables -- the DbTables themselves
 *  * self->seq_no_offsets -- the seq_no offet of each table
 *  * self->values_vectors -- values vectors -- one for each table
 *  * self->values_vector_counts -- size of each of the values_vectors
 *
 *  There should be at least ntables of each of these.  For the size of each of
 *  the values_vectors[i], see client_realloc_values().
 *
 *  If reallocating, only *grow* the structures: a call to
 *  client_realloc_tables(ch, n) then client_realloc_tables(ch, m) with m<n is
 *  safe and won't lose any data.
 *  self->table_count is set to ntables at the end of this, as long as
 *  there are not already enough tables to accomodate ntables.
 *
 * \param self the ClientHandler
 * \param ntables the maximal number of tables to handle
 *
 * \return 0 on success, -1 otherwise
 */
int
client_realloc_tables (ClientHandler *self, int ntables)
{
  if (!self || ntables <= 0) {
    return -1;
  }

  if (ntables > self->table_count) {
    int error = 0;
    DbTable **new_tables = xrealloc (self->tables, ntables*sizeof(DbTable*));
    int *new_so = xrealloc (self->seqno_offsets, ntables*sizeof(int));
    OmlValue **new_vv = xrealloc (self->values_vectors, ntables*sizeof(OmlValue*));
    int *new_vv_counts = xrealloc (self->values_vector_counts, ntables * sizeof (int));

    if (!new_tables || !new_so || !new_vv || !new_vv_counts) {
      logdebug ("%s: Failed to allocate memory for %d more client tables (current %d)\n",
          self->name, (ntables - self->table_count), self->table_count);
      // Don't free anything because whatever got successfully xrealloc'd is still ok
      error = -1;
    }

    /* Keep whatever succeeded */
    if (new_tables) {
      self->tables = new_tables;
      /* Initialise all new tables to NULL so we know they are not allocated yet */
      memset(&self->tables[self->table_count], 0, (ntables - self->table_count) * sizeof(DbTable*));
    }
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
          oml_value_array_init(self->values_vectors[i], self->values_vector_counts[i]);
        } else {
          self->values_vector_counts[i] = -1;
          logdebug ("%s: Could not allocate memory for values vector for table index %d\n", self->name, i);
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

/** (Re)allocate the values vector for the table with the given index,
 *  so that it is expanded (never reduced) to hold nvalues elements.
 *
 *  \param self ClientHandler holding the vectors
 *  \param idx index of the vector to reallocate
 *  \param nvalues number of values to allow in that vector
 *  \return 0 on success, -1 otherwise
 *
 *  \see client_realloc_tables
 */
int
client_realloc_values (ClientHandler *self, int idx, int nvalues)
{
  int curnvalues;

  if (!self || idx < 0 || idx > self->table_count || nvalues <= 0)
    return -1;

  curnvalues = self->values_vector_counts[idx];

  if (nvalues > curnvalues) {
    OmlValue *new_values = xrealloc (self->values_vectors[idx],
        nvalues * sizeof (OmlValue));
    if (!new_values) {
      logwarn("%s: Could not reallocate memory for values for table %d\n",
          self->name, idx);
      return -1;
    }

    oml_value_array_init(&new_values[curnvalues], nvalues - curnvalues);

    self->values_vectors[idx] = new_values;
    self->values_vector_counts[idx] = nvalues;
  }

  return 0;
}

/** Create a client handler and associates it with a Socket object.
 *
 * \param new_sock Socket object created by accept()ing the connection 
 * \return a pointer to the newly created ClientHandler
 *
 * \see on_client_connect()
 * \see on_connect()
 */
  ClientHandler*
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

void client_handler_free (ClientHandler* self)
{
  if (self->event)
    eventloop_socket_release (self->event);
  if (self->database)
    database_release (self->database);
  if (self->socket)
    socket_free (self->socket);
  if (self->tables)
    xfree (self->tables);
  if (self->seqno_offsets)
    xfree (self->seqno_offsets);
  mbuf_destroy (self->mbuf);
  int i, j;
  for (i = 0; i < self->table_count; i++) {
    for (j = 0; j < self->values_vector_counts[i]; j++) {
      oml_value_reset(&self->values_vectors[i][j]);
    }
    xfree (self->values_vectors[i]);
  }
  xfree (self->values_vectors);
  xfree (self->values_vector_counts);
  if (self->sender_name)
    xfree (self->sender_name);
  if (self->app_name)
    xfree (self->app_name);
  xfree (self);

  //  xmemreport ();
}

void client_handler_update_name(ClientHandler *self)
{
  if (self->database && self->sender_name && self->app_name) {
    snprintf(self->name, MAX_STRING_SIZE, "%s:%s:%s", self->database->name, self->sender_name, self->app_name);
    self->name[MAX_STRING_SIZE-1] = 0;
  } else if (self->event) {
    logwarn("%s: Some identification fields (domain, sender-id or app-name) were missing in the headers\n", self->event->name);
  } else {
    logerror("Unitialised fields in ClientHandler after end of headers; this is probably a bug\n");
  }
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

/** Process schema from string.
 *
 * \param self pointer to ClientHandler processing the schema
 * \param value schema to process
 * \return 1 if successful, 0 otherwise
 */
void
process_schema(ClientHandler* self, char* value)
{
  struct schema *schema = schema_from_meta (value);
  if (!schema) {
    logerror ("%s: Failure parsing schema '%s'; disconnecting client.\n", self->name, value);
    self->state = C_PROTOCOL_ERROR;
    return;
  }

  char *invalid = NULL;
  if (!validate_schema_names (schema, &invalid)) {
    logerror ("%s: Invalid name '%s' in schema '%s'\n", self->name, invalid, value);
    self->state = C_PROTOCOL_ERROR;
    schema_free (schema);
    return;
  }

  int idx = schema->index;
  loginfo("%s: New MS schema %s\n", self->name, value); /* Value contains the index */

  DbTable* table = database_find_or_create_table(self->database, schema);
  if (table == NULL) {
    logerror("%s: Can't find table '%s' or client schema '%s' doesn't match any of the existing tables.\n",
        self->name, schema->name, value);
    self->state = C_PROTOCOL_ERROR;
    schema_free (schema);
    return;
  }
  schema_free (schema);

  if (client_realloc_tables (self, idx + 1) == -1) {
    logerror ("%s: Failed to allocate memory for table index %d\n", self->name, idx);
    return;
  }

  if (self->tables[idx] != NULL) {
    logwarn("%s: Replacing existing MS schema %s with new schema %s\n",
        self->name, self->tables[idx]->schema->name, value);
  }
  self->tables[idx] = table;

  /* Reallocate the values vector if this schema has more columns than can fit already. */
  if (client_realloc_values (self, idx, table->schema->nfields) == -1) {
    logwarn ("%s: Could not allocate values vector of size %d for table index %d\n",
        self->name, table->schema->nfields, idx);
  }
}

/** \privatesection Process a single key/value pair contained in the header.
 *
 * \param self ClientHandler
 * \param key key
 * \param value value
 * \return 0 if meta was process successfully, <0 on error, or >0 if key was not recognised
 */
static int
process_meta(ClientHandler* self, char* key, char* value)
{
  int protocol;
  int start_time;

  chomp (value);
  logdebug("%s: Meta '%s':'%s'\n", self->name, key, value);

  if (strcmp(key, "protocol") == 0) {
    protocol = atoi (value);
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else if (protocol < MIN_PROTOCOL_VERSION || protocol > MAX_PROTOCOL_VERSION) {
      logerror("%s: Client connected with incorrect protocol version (%s; %d > %d)\n",
          self->name, value, protocol, MAX_PROTOCOL_VERSION);
      logdebug("%s:    Maybe the client was built with a newer version of OML.\n",
          self->name);
      self->state = C_PROTOCOL_ERROR;
      return -2;

    } else {
      return 0;
    }

  } else if (strcmp(key, "domain") == 0 || strcmp(key, "experiment-id") == 0) {
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else if (!(self->database = database_find(value))) {
      self->state = C_PROTOCOL_ERROR;
      return -2;

    } else {
      return 0;
    }

  } else if (strcmp(key, "start-time") == 0 || strcmp(key, "start_time") == 0) {
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else if (self->database == NULL) {
      logerror("%s: Meta 'start-time' needs to come after 'domain'.\n",
          self->name);
      self->state = C_PROTOCOL_ERROR;
      return -2;

    } else {

      /** \publicsection \page timestamps OML Timestamping
       *
       * OML provides a timestamping mechanism based on each reporting node's
       * time (`oml_ts_client`). Each server remaps the MSs they receive to an
       * experiment-wide timebase (`oml_ts_server`) which allows some time
       * comparisons to be made between measurements from different machines.
       * This mechanism however does not remove the need for a good time
       * synchronisation between the involved experimental nodes.
       *
       * Upon connection, the clients send headers to the server as key--value
       * pairs. One of the keys is the `start-time` (or `start_time`) which
       * indicates the client timestamp at which their messages has been
       * generated (`oml_ts_client` of the sample, \f$start_\mathrm{client}\f$
       * in the following). This gives the server an indication about the time
       * difference between its local clock (reading
       * \f$cur_\mathrm{server}\f$), and each of the clients'.
       *
       * To allow for some sort of time comparison between samples from
       * different clients, the server maintains a separate timestamp
       * (`oml_ts_server`) to which it remaps all client timestamps, based on
       * this difference.
       *
       * When the client connects, the server calculates:
       *
       *   \f[\delta = (start_\mathrm{client} - cur_\mathrm{server})\f]
       *
       * and stores it in the per-client data structure. For each packet from
       * the client, it takes the client's timestamp ts_client and calculates
       * ts_server as:
       *
       *   \f[ts_\mathrm{server} = ts_\mathrm{client} + \delta\f]
       *
       * When the first client for a yet unknown experimental domain connects,
       * the server also uses its timestamps to create a reference start time
       * for the samples belonging to that domain, with an arbitrary offset of
       * -100s to account for badly synchronised clients and avoid negative
       *  timestamps.
       *
       *   \f[start_\mathrm{server} = start_\mathrm{client} - 100\f]
       *
       * This server start date is stored in the database (in the
       * `_experiment_metadata` table) to enable experiment restarting.
       *
       * The key observation is that the exact reference datum on the client
       * and server doesn't matter; all that matters is that the server knows
       * what the client's date is (the client tells the server in its headers
       * when it connects).  Then the client specifies measurement packet
       * timestamps relative to the datum.  In our case the datum is tv_sec +
       * 1e-6 * tv_usec, where tv_sec and tv_usec are the corresponding members
       * of the struct timeval that gettimeofday(2) fills out.
       *
       * For this scheme to provide accurate time measurements, the clocks of
       * the client and server must be synchronized to the same reference (and
       * timezone). The precision of the time measurements is then governed by
       * the precision of gettimeofday(2).
       */

      start_time = atoi(value);
      if (self->database->start_time == 0) {
        // seed it with a time in the past
        self->database->start_time = start_time;// - 100;
        char s[64];
        snprintf (s, LENGTH(s), "%u", start_time);
        self->database->set_metadata (self->database, "start_time", s);
      }
      self->time_offset = start_time - self->database->start_time;
      return 0;
    }

  } else if (strcmp(key, "sender-id") == 0) {
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else if (self->database == NULL) {
      logerror("%s: Meta 'sender-id' needs to come after 'domain'.\n",
          self->name);
      self->state = C_PROTOCOL_ERROR;
      return -2;

    } else {
      self->sender_id = self->database->add_sender_id(self->database, value);
      self->sender_name = xstrndup (value, strlen (value));
      return 0;
    }

  } else if (strcmp(key, "app-name") == 0) {
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else {
      self->app_name = xstrndup (value, strlen (value));
      return 0;
    }

  } else if (strcmp(key, "schema") == 0) {
    process_schema(self, value);
    return 0;

  } else if (strcmp(key, "content") == 0) {
    if (self->state != C_HEADER) {
      logwarn("%s: Meta '%s' is only valid in the headers, ignoring\n",
          self->name, key);
      return -1;

    } else if (strcmp(value, "binary") == 0) {
      logdebug("%s: Switching to binary mode\n", self->name);
      self->content = C_BINARY_DATA;
      return 0;

    } else if (strcmp(value, "text") == 0) {
      self->content = C_TEXT_DATA;
      return 0;

    } else {
      logerror("%s: Unknown content type '%s'\n", self->name, value);
      self->state = C_PROTOCOL_ERROR;
      return -2;
    }

  } else {
    /* Unknown key, let the caller deal with it */
    return 1;
  }

  return -3;
}

/** Read a \n-terminated line from an MBuffer
 *
 * The caller is responsible of advancing (by *length_p+1) the MBuffer and/or
 * consuming the message to make sure it can be properly repacked.
 *
 * \param line_p[out] pointer to pointer to be set to point to the beginning of the string in the MBuffer
 * \param length_p[out] pointer to variable to be filled with the line lenght
 * \param mbuf MBuffer containing header and/or data
 *
 * \return 1 if successful, 0 otherwise
 * \see mbuf_read_skip, mbuf_consume_message
 */
static int
read_line(char** line_p, int* length_p, MBuffer* mbuf)
{
  uint8_t* line = mbuf_rdptr (mbuf);
  int length = mbuf_find (mbuf, '\n');

  // No newline found
  if (length == -1) {
    return 0;
  }

  *line_p = (char*)line;
  *length_p = length;
  *(line + length) = '\0';
  return 1;
}

/** Process one line of header.
 *
 * \param self pointer to ClientHandler processing the header
 * \param mbuf MBuffer containing the header
 * \return 1 if successful, 0 otherwise
 */
static int
process_header(ClientHandler* self, MBuffer* mbuf)
{
  char* line;
  int len;

  if (read_line(&line, &len, mbuf) == 0) {
    return 0;
  }

  if (len == 0) {
    // empty line denotes separator between header and body
    int skip_count = mbuf_find_not (mbuf, '\n');
    mbuf_read_skip (mbuf, skip_count + 1);
    mbuf_consume_message (mbuf);
    self->state = self->content;
    client_handler_update_name(self);
    loginfo("%s: Client '%s' ready to send data\n", self->event->name, self->name);
    return 0;
  }

  // separate key from value (check for ':')
  char* value = line;
  int count = 0;
  while (*(value) != ':' && count < len) {
    value++;
    count++;
  }

  if (*value == ':') {
    *value++ = '\0';
    while (*(value) == ' ' && count < len) {
      value++;
      count++;
    }
    mbuf_read_skip (mbuf, len + 1);
    process_meta(self, line, value);
  } else {
    logerror("%s: Malformed meta line in header: '%s'\n", self->name, line);
    self->state = C_PROTOCOL_ERROR;
  }

  // process_meta() might have signalled protocol error, so we have to check here.
  if (self->state == C_PROTOCOL_ERROR)
    return 0;
  else
    return 1; // still in header
}

/** Process contents of a message for which the header has already been
 * extracted by the marshalling code.
 *
 * If the stream ID index is 0, this is some metadata, otherwise, insert data
 * into the storage backend.
 * \param self ClientHandler
 * \param header OmlBinaryHeader of the message
 * \see process_bin_message, unmarshal_init
 */
static void
process_bin_data_message(ClientHandler* self, OmlBinaryHeader* header)
{
  double ts = self->time_offset + header->timestamp;
  int table_index = header->stream;
  int seqno = header->seqno;
  MBuffer* mbuf = self->mbuf;
  OmlValue *values, meta[2];
  int count;

  if (header->stream < 0 || table_index >= self->table_count) {
    logwarn("%s(bin): Table index %d out of bounds, discarding sample %d\n",
        self->name, table_index, seqno);
    return;
  }

  if (0 == header->stream) {
    /* Header mode: two strings: k/v */
    logdebug("%s(bin): Client sending metadata at %f\n", self->name, ts);

    oml_value_array_init(meta, 2);
    count = unmarshal_measurements(mbuf, header, meta, 2);

    if (count < 0) {
      oml_value_array_reset(meta, 2);
      return;
    } else if (count > 2) {
      logwarn("%s(bin): Expecting metadata, but the number of elements is invalid (%d), ignoring\n",
          self->name, count);
      oml_value_array_reset(meta, 2);
      return;
    } else if (oml_value_get_type(&meta[0]) != OML_STRING_VALUE ) {
      logwarn("%s(bin): Expecting metadata, but key is not a string (%d), ignoring\n",
          self->name, oml_value_get_type(&meta[0]));
    } else if (oml_value_get_type(&meta[1]) != OML_STRING_VALUE ) {
      logwarn("%s(bin): Expecting metadata, but value for key %s is not a string (%d), ignoring\n",
          self->name, omlc_get_string_ptr(*oml_value_get_value(&meta[0])), oml_value_get_type(&meta[1]));
    } else {
      process_meta(self,
          omlc_get_string_ptr(*oml_value_get_value(&meta[0])),
          omlc_get_string_ptr(*oml_value_get_value(&meta[1])));
    }

    oml_value_array_reset(meta, 2);

  } else {
    values = self->values_vectors[table_index];
    count = self->values_vector_counts[table_index];
    count = unmarshal_measurements(mbuf, header, values, count);

    /* Some error occurred in unmarshaling; can't continue */
    if (count < 0) {
      return;
    }

    DbTable* table = self->tables[table_index];
    if (table == NULL) {
      logerror("%s(bin): Undefined table index %d\n", self->name, table_index);
      self->state = C_PROTOCOL_ERROR;
      return;
    }
    logdebug("%s(bin): Inserting data into table index %d (seqno=%d, ts=%f)\n",
        self->name, table_index, seqno, ts);
    self->database->insert(self->database,
        table,
        self->sender_id,
        header->seqno,
        ts,
        self->values_vectors[table_index],
        count);
  }

  mbuf_consume_message (mbuf);
}

/** Read binary data from an MBuffer
 *
 * \param self client handler
 * \param mbuf MBuffer that contain the data
 * \return 1 when successfull, 0 otherwise
 */
static int
process_bin_message(ClientHandler* self, MBuffer* mbuf)
{
  int res;
  OmlBinaryHeader header;

  res = bin_find_sync(mbuf);
  if(res>0) {
    logwarn("%s(bin): Skipped %d bytes of data searching for a new message\n", self->name, res);
  } else if (res == -1 && mbuf_remaining(mbuf)>=2) {
    logdebug("%s(bin): No message found in binary packet; packet follows\n%s\n", self->name,
        to_octets(mbuf_rdptr(mbuf), mbuf_remaining(mbuf)));
    return 0;
  }

  res = unmarshal_init(mbuf, &header);
  if (res == 0) {
    logwarn("%s(bin): Could not find message header\n", self->name);
    return 0;
  } else if (res < 0 && mbuf->fill>0) {
    // not enough data
    logdebug("%s(bin): Not enough data (%dB) for a new measurement yet (at least %dB missing)\n",
        self->name, mbuf_remaining(mbuf), -res);
    return 0;
  }
  switch (header.type) {
  case OMB_DATA_P:
  case OMB_LDATA_P:
    process_bin_data_message(self, &header);
    if (self->state != C_BINARY_DATA)
      return 0;
    break;
  default:
    logwarn("%s(bin): Ignoring unsupported message type '%d'\n", self->name, header.type);
    /* XXX: Assume we could read the full header, just skip it
     * FIXME: We might have to skip the data too
     self->state = C_PROTOCOL_ERROR;
     */
    return 0;
  }

  return 1;
}

/** Process split data.
 *
 * The data would have been split by process_text_message.
 *
 * \param self pointer to ClientHandler processing the data
 * \param msg pointer to array of strings corresponding to the fields
 * \param size length of msg
 *
 * \see process_text_message
 */
static void
process_text_data_message(ClientHandler* self, char** msg, int size)
{
  if (size < 3) {
    return;
  }

  double ts;
  long table_index;
  long seq_no;

  ts = atof(msg[0]);
  table_index = atol(msg[1]) - 1;
  seq_no = atol(msg[2]);

  ts += self->time_offset;
  if (-1 == table_index) { /* Stream 0: Metadata */
    if (size==5)
      process_meta(self, msg[3], msg[4]);
    else
      logwarn("%s(txt): received metadata with incorrect number of elements (%d)\n", self->name, size-3);
    return;
  } else if (table_index >= self->table_count || table_index < 0) {
    logwarn("%s(txt): Table index %d out of bounds, discarding sample %d\n", self->name, table_index, seq_no);
    return;
  }
  DbTable* table = self->tables[table_index];
  if (table == NULL) {
    logerror("%s(txt): Undefined table index %d\n", self->name, table_index);
    return;
  }

  struct schema *schema = table->schema;
  if (schema->nfields != size - 3) {
    logerror("%s(txt): Data item number mismatch for schema '%s' (expected %d, got %d)\n",
        self->name, schema->name, schema->nfields, size-3);
    return;
  }

  int i;
  OmlValue* v = self->values_vectors[table_index];
  char** val_ap = msg + 3;
  for (i= 0; i < schema->nfields; i++, v++, val_ap++) {
    char* val = *val_ap;
    oml_value_set_type(v, schema->fields[i].type);
    if (oml_value_from_s (v, val) == -1)
      logerror("%s(txt): Error converting value of type %d from string '%s'\n", self->name, oml_value_get_type(v), val);
    /* FIXME: Do something here */
  }

  logdebug("%s(txt): Inserting data into table index %d (seqno=%d, ts=%f)\n",
      self->name, table_index, seq_no, ts);
  self->database->insert(self->database, table, self->sender_id, seq_no,
      ts, self->values_vectors[table_index], size - 3);
}

/** Process as many lines of data as possible from an MBuffer.
 *
 * \param self pointer to ClientHandler processing the data
 * \param mbuf MBuffer containing the header
 * \return 1 if successful, 0 otherwise
 */
static int
process_text_message(ClientHandler* self, MBuffer* mbuf)
{
  char* line;
  int len;

  while (C_TEXT_DATA == self->state) {
    if (read_line(&line, &len, mbuf) == 0) {
      return 0;
    }
    mbuf_read_skip(mbuf, len+1);
    mbuf_consume_message(mbuf);

    // split line into array
    char* a[DEF_NUM_VALUES];
    int    a_size = 0;
    char* p = line;
    int rem = len;
    int lastempty = 0;

    while (rem > 0 || lastempty) {
      char* param = p;
      lastempty = 0;
      for (; rem > 0; rem--, p++) {
        if (*p == '\t') {
          *(p++) = '\0';
          rem--;
          // Set if there was an empty string at the end of the line
          lastempty = (0 == rem && '\0' == *(p-1) && !lastempty);
          break;
        }
      }
      a[a_size++] = param;
      if (a_size >= DEF_NUM_VALUES) {
        logerror("%s(txt): Too many parameters (%d>=%d) in sample '%s'\n", self->name, a_size, DEF_NUM_VALUES, line);
        return 0;
      }
    }
    /* XXX: This message belongs in process_text_data_message(),
     * however putting it here allows to access line, for nicer logging*/
    if (a_size < 3) {
      logerror("%s(txt): Not enough parameters (%d<3) in sample '%s'\n", self->name, a_size, line);
      return 0;
    }
    process_text_data_message(self, a, a_size);
  }
  /* Never reached */
  return 0;
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

  logdebug("%s(%s): Received %d bytes of data\n",
      source->name,
      client_state_to_s (self->state),
      buf_size);

  int result = mbuf_write (mbuf, buf, buf_size);

  if (result == -1) {
    logerror("%s: Failed to write message from client into message buffer\n",
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
    logerror("%s: Fatal error, disconnecting client\n",
        source->name);
    client_handler_free (self);
    /*
     * Protocol error --> no need to repack buffer, so just return;
     */
    return;
  default:
    logerror("%s: Unknown client state %d\n", source->name, self->state);
    mbuf_clear (mbuf);
    return;
  }

  if (self->state == C_PROTOCOL_ERROR)
    goto process;

  // move remaining buffer content to beginning
  mbuf_repack_message (mbuf);
  logdebug("%s: Buffer repacked to %d bytes\n",
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
  ClientHandler* self = (ClientHandler*)handle;

  logdebug("%s: Socket status changed to %s (%d); error code is %d\n",
           source->name, socket_status_string (status), status, errcode);

  switch (status)
    {
    case SOCKET_WRITEABLE:
      break;
    case SOCKET_CONN_CLOSED:
      /* Client closed the connection */
      loginfo("%s: Client '%s' closed connection\n", source->name, self->name);
      client_handler_free (self);
      break;
    case SOCKET_IDLE:
      /* Server dropped idle connection */
      loginfo("%s: Client '%s' dropped due to idleness\n", source->name, self->name);
      client_handler_free (self);
      break;
    case SOCKET_UNKNOWN:
    case SOCKET_CONN_REFUSED:
    case SOCKET_DROPPED:
    default:
      logwarn ("%s: Client '%s' received unhandled condition %s (%d)\n", source->name,
          self->name, socket_status_string (status), status);
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
