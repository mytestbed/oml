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
/*!\file net_writer.c
  \brief Implements a writer which sends results over the network
*/

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>

#include "oml2/omlc.h"
#include "client.h"
#include "oml2/oml_writer.h"
#include "oml2/oml_filter.h"
#include "marshall.h"

#define DEF_PROTOCOL "tcp"
#define DEF_PORT 3003

typedef struct _omlNetWriter {

  oml_writer_meta meta;
  oml_writer_header_done header_done;

  //! Called before and after writing individual
  // filter results with 'out'
  oml_writer_row_start row_start;
  oml_writer_row_end row_end;

  //! Writing a result.
  oml_writer_out out;

  oml_writer_close close;

  OmlWriter* next;

  /**********************/

  int        first_row;
  Socket*    socket;
  OmlMBuffer mbuf;

  int        stream_count; // used to give each stream an ID

} OmlNetWriter;

static int write_meta(OmlWriter* writer, char* str);
static int header_done(OmlWriter* writer);

static int write_schema(OmlWriter* writer, OmlMStream* ms);
static int row_start(OmlWriter* writer, OmlMStream* ms, double now);
static int out(OmlWriter* writer, OmlValue* values, int value_count);
static int row_end(OmlWriter* writer, OmlMStream* ms);
static int close(OmlWriter* writer);


OmlWriter*
net_writer_new(
  char* protocol,
  char* location
) {
  OmlNetWriter* self = (OmlNetWriter *)malloc(sizeof(OmlNetWriter));
  memset(self, 0, sizeof(OmlNetWriter));

  char* host;
  char* p = location;
  if (! (strcmp(protocol, "tcp") == 0 || strcmp(protocol, "udp") == 0)) {
    host = protocol;
    protocol = DEF_PROTOCOL;
  } else {
    while (*p != '\0' && *p == '/') p++; // skip leading "//"
    host = p;
    while (*p != '\0' && *p != ':') p++;
    if (*p != '\0') *(p++) = '\0';
  }
  char* portS = p;
  while (*p != '\0' && *p != ':') p++;
  if (*p != '\0') *(p++) = '\0';
  int port = (*portS == '\0') ? DEF_PORT : atoi(portS);

  char* localBind = p;  // This would be for multicast, ignore right now

  o_log(O_LOG_INFO, "Net proto: <%s> host: <%s> port: <%d>\n", protocol, host, port);

  socket_set_non_blocking_mode(0);
  if (protocol == NULL || strcmp(protocol, "tcp") == 0) {
    if ((self->socket = socket_tcp_out_new("sock", host, port)) == NULL) {
      return NULL;
    }
  }
  self->meta = write_meta;
  self->header_done = header_done;

  self->row_start = row_start;
  self->row_end = row_end;
  self->out = out;
  self->close = close;

  self->first_row = 1;

//  char s[128];
//  sprintf(s, "protocol: %d", OML_PROTOCOL_VERSION);
//  write_meta(self, s);
//  sprintf(s, "experiment-id: %s", omlc_instance->experiment_id);
//  write_meta(self, s);
//  sprintf(s, "content: binary");
//  write_meta(self, s);
//  sprintf(s, "sender-id: %s", omlc_instance->node_name);
//  write_meta(self, s);
//  sprintf(s, "app-name: %s", omlc_instance->app_name);
//  write_meta(self, s);

  return (OmlWriter*)self;
}

static int
write_meta(
  OmlWriter* writer,
  char* str
) {
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;
  char s[512];

  sprintf(s, "%s\n", str);
  int len = strlen(s);
  return (socket_sendto(self->socket, s, len) == len);
}

static int
header_done(
  OmlWriter* writer
) {
  write_meta(writer, "content: binary");
  write_meta(writer, "");
}

//int
//write_schema(
//  OmlWriter* writer,
//  OmlMStream* ms
//) {
//  OmlNetWriter* self = (OmlNetWriter*)writer;
//  char s[512];
//
//  ms->index = self->stream_count++;
//  sprintf(s, "schema: %d %s ", ms->index, ms->table_name);
//
//  // Loop over all the filters
//  //OmlMPDef2* def = ms->mp->param_defs;
//  OmlMP* mp = ms->mp;
//  int i;
//  for (i = 0; i < mp->param_count; i++) {
//    const char* prefix = mp->param_defs[i].name;
//    OmlFilter* filter = ms->filters[i];
//    int j;
//    for (j = 0; j < filter->output_cnt; j++) {
//      char* name;
//      OmlValueT type;
//      if (filter->meta(filter, j, &name, &type)) {
//        char* type_s = oml_type_to_s(type);
//        if (name == NULL) {
//          sprintf(s, "%s %s:%s", s, prefix, type_s);
//        } else {
//          sprintf(s, "%s %s_%s:%s", s, prefix, name, type_s);
//        }
//      }
//    }
//  }
//  write_meta(self, s);
//  return 1;
//}

static int
out(
  OmlWriter* writer, //! pointer to writer instance
  OmlValue*  values,  //! type of sample
  int        value_count //! size of above array
) {
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;

  int cnt = marshall_values(&self->mbuf, values, value_count);
  return cnt == value_count;
}

int
row_start(
  OmlWriter* writer,
  OmlMStream* ms,
  double now
) {
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;

//  if (self->first_row) {
//    // need to add eoln to separate from header
//    char e = '\n';
//    socket_sendto(self->socket, &e, 1);
//    self->first_row = 0;
//  }

  marshall_measurements(&self->mbuf, ms, now);
  return 1;
}

int
row_end(
  OmlWriter* writer,
  OmlMStream* ms
) {
  OmlNetWriter* self = (OmlNetWriter*)writer;
  if (self->socket == NULL) return 1;

  marshall_finalize(&self->mbuf);
  OmlMBuffer* buf = &self->mbuf;
  int len = buf->buffer_length - buf->buffer_remaining;
  o_log(O_LOG_DEBUG, "Sending message of size '%d'\n", len);
  socket_sendto(self->socket, (char*)buf->buffer, len);
  return 1;
}

static int
close(
  OmlWriter* writer
) {
  OmlNetWriter* self = (OmlNetWriter*)writer;

  if (self->socket != 0) {
    socket_close(self->socket);
    self->socket = NULL;
  }
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
