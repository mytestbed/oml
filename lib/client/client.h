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
/*!\file client.h
  \brief Defines various structures and functions used by various parts.
*/

#ifndef CLIENT_H_
#define CLIENT_H_

#include <stdio.h>
#include <time.h>
#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "oml2/oml_writer.h"

#define CONFIG_ROOT_NAME "omlc"
#define NODE_ID_ATTR "id"
#define EXP_ID_ATTR "exp_id"

#define COLLECT_EL "collect"
#define MP_EL "mp"
#define FILTER_EL "f"
#define FILTER_NAME_ATTR "fname"
#define FILTER_STREAM_NAME_ATTR "sname"
#define FILTER_PARAM_NAME_ATTR "pname"
#define FILTER_MULTI_PARAM_ATTR "multi_pnames"
#define FILTER_PROPERTY_EL "fp"
#define FILTER_PROPERTY_NAME_ATTR "name"
#define FILTER_PROPERTY_TYPE_ATTR "type"  // optional [string, long, double]

#define SERVER_URI_MAX_LENGTH 64

typedef struct _omlClient {
  const char* app_name;
  const char* experiment_id;
  const char* node_name;

  OmlMP*       mpoints;

  char server_uri[SERVER_URI_MAX_LENGTH + 1];

  OmlWriter*  first_writer;

  const char* config_file;

  time_t      start_time;    // unix epoch when started

  // The following are used for setting up default filters when
  // we don't have a config file.
  int         sample_count;  // default sample count
  double      sample_interval; // default sample interval
} OmlClient;

extern OmlClient* omlc_instance;

// init.c

OmlMStream*
create_mstream(
    double     sample_interval,
    int        sample_thres,
    OmlMP*     mp,
    OmlWriter* writer
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

extern OmlWriter*
file_writer_new(char* file);

extern OmlWriter*
net_writer_new(char* protocol, char* location);

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
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
