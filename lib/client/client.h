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
/*!\file client.h
  \brief Defines various structures and functions used by various parts.
*/

#ifndef OML_CLIENT_H_
#define OML_CLIENT_H_

#include <stdio.h>
#include <time.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include <oml2/oml_writer.h>
#include <oml2/oml_out_stream.h>

#include <mstring.h>

#define COLLECTION_URI_MAX_LENGTH 64

#define xstr(s) str(s)
#define str(s) #s
#define DEF_PORT 3003
#define DEF_PORT_STRING  xstr(DEF_PORT)

typedef struct _omlClient {
  const char* app_name;
  const char* domain;
  const char* node_name;

  OmlMP*       mpoints;

  char collection_uri[COLLECTION_URI_MAX_LENGTH + 1];

  OmlWriter*  first_writer;

  const char* config_file;

  time_t      start_time;    // unix epoch when started

  // The following are used for setting up default filters when
  // we don't have a config file.
  int         sample_count;  // default sample count
  double      sample_interval; // default sample interval
  int         max_queue; // Maximum number of buffers in the buffer queue for each writer
  enum StreamEncoding default_encoding; // Default wire encoding for network streams
} OmlClient;

extern OmlClient* omlc_instance;

// init.c

OmlMP * find_mp (const char *name);
int find_mp_field (const char *name, OmlMP *mp);
OmlMStream *find_mstream (const char *name);
OmlMStream *find_mstream_in_mp (const char *name, OmlMP *mp);

MString *mp_fields_summary (OmlMP *mp);

OmlMStream*
create_mstream(
    const char * name,
    OmlMP*     mp,
    OmlWriter* writer,
    double     sample_interval,
    int        sample_thres
);

void
create_default_filters(
    OmlMP*      mp,
    OmlMStream* ms
);

OmlFilter*
create_default_filter(
    OmlMPDef*   def,
    OmlMStream* ms,
    int         index
);

// filter.c

void
filter_engine_start(
  OmlMStream* mp
);

// misc.c

int
mp_lock(
  OmlMP* mp
);

void
mp_unlock(
  OmlMP* mp
);

int
oml_lock(
  pthread_mutex_t* mutexP,
  const char* mutexName
);

void
oml_unlock(
  pthread_mutex_t* mutexP,
  const char* mutexName
);



extern OmlWriter*
text_writer_new(OmlOutStream* out_stream);

extern OmlWriter*
bin_writer_new(OmlOutStream* out_stream);

extern OmlOutStream*
file_stream_new(const char *file);
int file_stream_set_buffered(OmlOutStream* hdl, int buffered);
int file_stream_get_buffered(OmlOutStream* hdl); 

extern OmlOutStream*
net_stream_new(const char *transport, const char *hostname, const char *port);

extern int
filter_process(OmlMStream* mp);

const char*
validate_app_name (const char* name);

const char*
validate_mp_name (const char* name);

#endif /*CLIENT_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
