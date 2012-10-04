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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <log.h>
#include <mstring.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>

#include <oml_value.h>
#include <mem.h>
#include <validate.h>
#include <oml2/omlc.h>
#include <oml2/oml_filter.h>
#include <oml2/oml_writer.h>
#include "filter/factory.h"
#include "oml_util.h"
#include "client.h"
#include "version.h"

OmlClient* omlc_instance = NULL;

#define DEF_LOG_LEVEL = O_LOG_INFO;

static OmlClient instance_storage;

static void usage(void);
static void print_filters(void);
static int  default_configuration(void);
static int  write_meta(void);
static int  write_schema(OmlMStream* ms, int index);
static void termination_handler(int signum);
static void install_close_handler(void);
static void setup_features(const char * const features);

static char *default_uri(const char *app_name, const char *name, const char *domain);

extern int parse_config(char* config_file);

/** Initialise the measurement library.
 *
 * This functions parses the command line for --oml-* options and acts
 * accordingly when they are found. A side effect of this function is that
 * these options and their arguments are removed from argv, so the instrumented
 * application doesn't see spurious OML option it can't make sense of.
 *
 * \param name name of the application
 * \param pargc argc of the command line of the application
 * \param argv argv of the command line of the application
 * \param custom_oml_log custom format-based logging function
 * \return 0 on success, -1 on failure.
 *
 * \see o_log_fn, o_set_log()
 */
int
omlc_init(const char* application, int* pargc, const char** argv, o_log_fn custom_oml_log)
{
  const char *app_name = validate_app_name (application);
  const char* name = NULL;
  const char* domain = NULL;
  const char* config_file = NULL;
  const char* local_data_file = NULL;
  const char* collection_uri = NULL;
  enum StreamEncoding default_encoding = SE_None;
  int sample_count = 0;
  double sample_interval = 0.0;
  int max_queue = 0;
  const char** arg = argv;

  if (!app_name) {
    logerror("Found illegal whitespace in application name '%s'\n", application);
    return -1;
  }

  omlc_instance = NULL;

  o_set_log_level(O_LOG_INFO);
  if (custom_oml_log)
    o_set_log (custom_oml_log);

  if (pargc && arg) {
    int i;
    for (i = *pargc; i > 0; i--, arg++) {
      if (strcmp(*arg, "--oml-id") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-id'\n");
          return -1;
        }
        name = *++arg;
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-domain") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-domain'\n");
          return -1;
        }
        domain = *++arg;
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-exp-id") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-exp-id'\n");
          return -1;
        }
        domain = *++arg;
        logwarn("Option --oml-exp-id is getting deprecated; please use '--oml-domain %s' instead\n", domain);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-file") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-file'\n");
          return -1;
        }
        local_data_file = *++arg;
        logwarn("Option --oml-file is getting deprecated; please use '--oml-collect file:%s' instead\n", local_data_file);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-collect") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-collect'\n");
          return -1;
        }
        collection_uri = (char*)*++arg;
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-config") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-config'\n");
          return -1;
        }
        config_file = *++arg;
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-samples") == 0) {
        if (--i <= 0) {
          logerror("Missing argument to '--oml-samples'\n");
          return -1;
        }
        sample_count = atoi(*++arg);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-interval") == 0) {
        if (--i <= 0) {
          logerror("Missing argument to '--oml-interval'\n");
          return -1;
        }
        sample_interval = atof(*++arg);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-log-file") == 0) {
        if (--i <= 0) {
          logerror("Missing argument to '--oml-log-file'\n");
          return -1;
        }
        o_set_log_file((char*)*++arg);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-log-level") == 0) {
        if (--i <= 0) {
          logerror("Missing argument to '--oml-log-level'\n");
          return -1;
        }
        o_set_log_level(atoi(*++arg));
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-server") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-server'\n");
          return -1;
        }
        collection_uri = (char*)*++arg;
        logwarn("Option --oml-server is getting deprecated; please use '--oml-collect %s' instead\n", collection_uri);
        *pargc -= 2;
      } else if (strcmp (*arg, "--oml-text") == 0) {
        *pargc -= 1;
        default_encoding = SE_Text;
      } else if (strcmp (*arg, "--oml-binary") == 0) {
        *pargc -= 1;
        default_encoding = SE_Binary;
      } else if (strcmp(*arg, "--oml-bufsize") == 0) {
        if (--i <= 0) {
          logerror("Missing argument to '--oml-bufsize'\n");
          return -1;
        }
        max_queue = atoi(*++arg);
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-noop") == 0) {
        *pargc -= 1;
        omlc_instance = NULL;
        return 1;
      } else if (strcmp(*arg, "--oml-help") == 0) {
        usage();
        *pargc -= 1;
        exit(0);
      } else if (strcmp(*arg, "--oml-list-filters") == 0) {
        print_filters();
        *pargc -= 1;
        exit(0);
      } else {
        *argv++ = *arg;
      }
    }
  }

  if (name == NULL)
    name = getenv("OML_NAME");
  if (domain == NULL)
    domain = getenv("OML_EXP_ID");
  if (config_file == NULL)
    config_file = getenv("OML_CONFIG");
  if (local_data_file == NULL && collection_uri == NULL)
    if(!(collection_uri = getenv("OML_COLLECT")))
      if (collection_uri = getenv("OML_SERVER"))
        logwarn("Enviromnent variable OML_SERVER is getting deprecated; please use 'OML_COLLECT=\"%s\"' instead\n",
            collection_uri);
  if(collection_uri == NULL) {
    collection_uri = default_uri(app_name, name, domain);
  }

  setup_features (getenv ("OML_FEATURES"));

  omlc_instance = &instance_storage;
  memset(omlc_instance, 0, sizeof(OmlClient));

  omlc_instance->app_name = app_name;
  omlc_instance->node_name = name;
  omlc_instance->domain = domain;
  omlc_instance->sample_count = sample_count;
  omlc_instance->sample_interval = sample_interval;
  omlc_instance->default_encoding = default_encoding;
  omlc_instance->max_queue = max_queue;

  if (local_data_file != NULL) {
    // dump every sample into local_data_file
    if (local_data_file[0] == '-')
      local_data_file = "stdout";
    snprintf(omlc_instance->collection_uri, COLLECTION_URI_MAX_LENGTH, "file:%s", local_data_file);
  } else if (collection_uri != NULL) {
    strncpy(omlc_instance->collection_uri, collection_uri, COLLECTION_URI_MAX_LENGTH);
  }
  omlc_instance->config_file = config_file;

  register_builtin_filters ();

  loginfo ("OML Client V%s [Protocol V%d] %s\n",
           VERSION,
           OML_PROTOCOL_VERSION,
           OMLC_COPYRIGHT);

  return 0;
}

/**
 * @brief Register a measurement point. Needs to be called for every measurment point AFTER +omlc_init+ and before a final +omlc_start+.
 * @param  mp_name the name of the measurement point
 * @param mp_def the definition of the set of measurements in this measurement point
 * @return a new measurement point
 */
OmlMP*
omlc_add_mp (const char* mp_name, OmlMPDef* mp_def)
{
  if (omlc_instance == NULL) return NULL;
  if (!validate_name (mp_name))
    {
      logerror("Found illegal MP name '%s'.  MP will not be created\n", mp_name);
      return NULL;
    }

  OmlMP* mp = (OmlMP*)malloc(sizeof(OmlMP));
  memset(mp, 0, sizeof(OmlMP));

  mp->name = mp_name;
  mp->param_defs = mp_def;
  int pc = 0;
  OmlMPDef* dp = mp_def;
  while (dp != NULL && dp->name != NULL)
    {
      if (dp->param_types == OML_LONG_VALUE)
        {
          logwarn("Measurement Point '%s', field '%s':\n", mp->name, dp->name);
          logwarn("--> OML_LONG_VALUE is deprecated and should not be used in new code\n");
          logwarn("--> Values outside of [INT_MIN, INT_MAX] will be clamped!\n");
        }
      if (!validate_name (dp->name)) {
        logerror ("Found illegal field name '%s' in MP '%s'.  MP will not be created\n",
                  dp->name, mp_name);
        free (mp);
        return NULL;
      }
      pc++;
      dp++;
    }
  mp->param_count = pc;
  mp->active = 1;  // True if there is an attached MS.
  mp->next = omlc_instance->mpoints;
  omlc_instance->mpoints = mp;

  return mp;
}

/**
 * @brief Finalizes inital configurations and get ready for consecutive +omlc_process+ calls
 * @return 0 if successful, <0 otherwise
 */
int
omlc_start()
{
  if (omlc_instance == NULL) return -1;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  omlc_instance->start_time = tv.tv_sec;

  const char* config_file = omlc_instance->config_file;
  if (config_file) {
    if (parse_config((char*)config_file)) {
      logerror("Error while parsing configuration '%s'\n", config_file);
      omlc_instance = NULL;
      return -1;
    }
  } else {
    if (omlc_instance->collection_uri == NULL) {
      logerror("Missing --oml-collect declaration.\n");
      omlc_instance = NULL;
      return -2;
    }
    if (default_configuration()) {
      omlc_instance = NULL;
      return -3;
    }
  }
  install_close_handler();
  if (write_meta() == -1)
    return -1;
  return 0;
}

/**
 * @brief Close the oml process
 * @param signum the signal number
 */
static void
termination_handler(int signum)
{
  // SIGPIPE is handled by disabling the writer that caused it.
  if (signum != SIGPIPE)
    {
      logdebug("Closing OML (%d)\n", signum);
      omlc_close();
      exit(-1 * signum);
    }
}

/**
 * @brief start the signal handler
 */
static void
install_close_handler(void)
{
  struct sigaction new_action, old_action;

  /* Set up the structure to specify the new action. */
  new_action.sa_handler = termination_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;

  sigaction (SIGINT, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGINT, &new_action, NULL);

  sigaction (SIGHUP, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGHUP, &new_action, NULL);

  sigaction (SIGTERM, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGTERM, &new_action, NULL);

  sigaction (SIGPIPE, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGPIPE, &new_action, NULL);
}

/**
 * @brief Finalizes all open connections. Any futher calls to +omlc_process+ are being ignored
 * @return -1 if fails
 */
int
omlc_close(void)
{
  if (omlc_instance == NULL) return -1;

  OmlWriter* w = omlc_instance->first_writer;
  OmlMP* mp = omlc_instance->mpoints;
  omlc_instance = NULL;

  for (; mp != NULL; mp = mp->next) {
    if (!mp_lock(mp)) {
      mp->active = 0;
      mp_unlock(mp);
    }
  }

  for (; w != NULL; w = w->next) {
    if (w->close != NULL) w->close(w);
  }
  return 0;
}

/**
 * @brief print the possible OML parameters
 */
static void
usage(void)

{
  printf("OML Client V%s\n", VERSION);
  printf("OML Protocol V%d\n", OML_PROTOCOL_VERSION);
  printf("%s\n", OMLC_COPYRIGHT);
  printf("\n");
  printf("OML specific parameters:\n\n");
  printf("  --oml-id id            .. Name to identify this app instance\n");
  printf("  --oml-domain domain    .. Name of experimental domain\n");
  printf("  --oml-collect uri      .. URI of server to send measurements to\n");
  printf("  --oml-config file      .. Reads configuration from 'file'\n");
  printf("  --oml-samples count    .. Default number of samples to collect\n");
  printf("  --oml-interval seconds .. Default interval between measurements\n");
  printf("  --oml-text             .. Use text encoding for all output streams\n");
  printf("  --oml-binary           .. Use binary encoding for all output streams\n");
  printf("  --oml-bufsize size     .. Set size of internal buffers to 'size' bytes\n");
  printf("  --oml-log-file file    .. Writes log messages to 'file'\n");
  printf("  --oml-log-level level  .. Log level used (error: -2 .. info: 0 .. debug4: 4)\n");
  printf("  --oml-noop             .. Do not collect measurements\n");
  printf("  --oml-list-filters     .. List the available types of filters\n");
  printf("  --oml-help             .. Print this message\n");
  printf("\n");
  printf("Valid URI: [tcp:]host[:port], (file|flush):localPath\n");
  printf("\n");
  printf("The following environment variables are recognized:\n");
  printf("  OML_NAME=id            .. Name to identify this app instance (--oml-id)\n");
  printf("  OML_DOMAIN=domain      .. ame of experimental domain (--oml-domain)\n");
  printf("  OML_CONFIG=file        .. Read configuration from 'file' (--oml-config)\n");
  printf("  OML_COLLECT=uri        .. URI of server to send measurements to (--oml-collect)\n");
  printf("\n");
  printf("Obsolescent interfaces:\n\n");
  printf("  --oml-exp-id domain    .. Equivalent to --oml-domain domain\n");
  printf("  --oml-file localPath   .. Equivalent to --oml-collect file:localPath\n");
  printf("  --oml-server uri       .. Equivalent to --oml-collect uri\n");
  printf("  OML_EXP_ID=domain      .. Equivalent to OML_DOMAIN\n");
  printf("  OML_SERVER=uri         .. Equivalent to OML_COLLECT\n");
  printf("\n");
  printf("If the corresponding command line option is present, it overrides\n");
  printf("the environment variable.\n");
  printf("\n");
}

static void
print_filters(void)
{
  register_builtin_filters();

  printf("OML Client V%s\n", VERSION);
  printf("OML Protocol V%d\n", OML_PROTOCOL_VERSION);
  printf("%s\n", OMLC_COPYRIGHT);
  printf("\n");
  printf("OML filters available:\n\n");

  const char* filter = NULL;
  do
    {
      filter = next_filter_name();
      if (filter)
        printf ("\t%s\n", filter);
    }
  while (filter != NULL);
  printf ("\n");
}

int
parse_dest_uri (const char *uri, const char **protocol, const char **path, const char **port)
{
  const int MAX_PARTS = 3;
  char *parts[3] = { NULL, NULL, NULL };
  size_t lengths[3] = { 0, 0, 0 };
  int is_valid = 1;
  OmlURIType uri_type;

  if (uri) {
    int i, j;
    uri_type = oml_uri_type(uri);
    parts[0] = xstrndup (uri, strlen (uri));
    for (i = 1, j = 0; i < MAX_PARTS; i++, j = 0) {
      parts[i] = parts[i-1];
      while (*parts[i] != ':' && *parts[i] != '\0') {
        j++;
        parts[i]++;
      }
      if (*parts[i] == ':')
        *(parts[i]++) = '\0';
    }

    for (i = 0; i < MAX_PARTS; i++) {
      lengths[i] = parts[i] ? strlen (parts[i]) : 0;
    }

#define trydup(i) (parts[(i)] && lengths[(i)]>0 ? xstrndup (parts[(i)], lengths[(i)]) : NULL)
    *protocol = *path = *port = NULL;
    if (lengths[0] > 0 && lengths[1] > 0) {
      /* Case 1:  "abc:xyz" or "abc:xyz:123" -- if abc is a transport, use it; otherwise, it's a hostname/path */
      if (oml_uri_is_network(uri_type)) {
        *protocol = trydup (0);
        *path = trydup (1);
        *port = trydup (2);
      } else if (oml_uri_is_file(uri_type)) {
        *protocol = trydup (0);
        *path = trydup (1);
        *port = NULL;
      } else {
        *protocol = NULL;
        *path = trydup (0);
        *port = trydup (1);
      }
    } else if (lengths[0] > 0 && lengths[2] > 0) {
      /* Case 2:  "abc::123" -- not valid, as we can't infer a hostname/path */
      logwarn ("Server URI '%s' is invalid as it does not contain a hostname/path\n", uri);
      is_valid = 0;
    } else if (lengths[0] > 0) {
      *protocol = NULL;
      *path = trydup (0);
      *port = NULL;

      /* Look for potential user errors and issue a warning but proceed as normal */
      if (uri_type != OML_URI_UNKNOWN) {
        logwarn ("Server URI with unknown scheme, assuming 'tcp:%s'\n",
                 *path);
      }
    } else {
      logerror ("Server URI '%s' seems to be empty\n", uri);
      is_valid = 0;
    }
#undef trydup

    if (parts[0])
      xfree (parts[0]);
  }
  if (is_valid)
    return 0;
  else
    return -1;
}

/**
 * @brief Creates either a file writer or a network writer
 * @param uri the option file or server and the output
 * @param encoding the encoding to use for the stream output, either
 * SE_Text or SE_Binary.
 * @return a writer
 */
OmlWriter*
create_writer(const char* uri, enum StreamEncoding encoding)
{
  OmlURIType uri_type = oml_uri_type(uri);

  if (omlc_instance == NULL){
    logerror("No omlc_instance:  OML client was not initialized properly.\n");
    return NULL;
  }

  if (uri == NULL) {
    logerror ("Missing collection URI definition (e.g., --oml-collect)\n");
    return NULL;
  }
  if (omlc_instance->node_name == NULL) {
    logerror ("Missing '--oml-id' flag \n");
    return NULL;
  }
  if (omlc_instance->domain == NULL) {
    logerror ("Missing '--oml-domain' flag \n");
    return NULL;
  }

  const char *transport;
  const char *path;
  const char *port;

  if (parse_dest_uri (uri, &transport, &path, &port) == -1) {
    logerror ("Error parsing server destination URI '%s'; failed to create stream for this destination\n",
              uri);
    if (transport) xfree ((void*)transport);
    if (path) xfree ((void*)path);
    if (port) xfree ((void*)port);
    return NULL;
  }

  const char *filepath = NULL;
  const char *hostname = NULL;
  if (oml_uri_is_file(uri_type)) {
    /* 'file://path/to/file' is equivalent to unix path '/path/to/file' */
    if (strncmp (path, "//", 2) == 0)
      filepath = &path[1];
    else
      filepath = path;
  } else if (transport) {
    if (strncmp (path, "//", 2) == 0)
      hostname = &path[2];
    else
      hostname = path;
  } else {
    hostname = path; /* If no transport specified, it must be tcp */
  }

  /* Default transport is tcp if not specified */
  if (!transport)
    transport = xstrndup ("tcp", strlen ("tcp"));

  /* If not file transport, use the OML default port if unspecified */
  if (!port && !oml_uri_is_file(uri_type))
    port = xstrndup (DEF_PORT_STRING, strlen (DEF_PORT_STRING));

  OmlOutStream* out_stream;
  if (oml_uri_is_file(uri_type)) {
    out_stream = file_stream_new(filepath);
    if (encoding == SE_None) encoding = SE_Text; /* default encoding */
    if(OML_URI_FILE_FLUSH == uri_type) {
      file_stream_set_buffered(out_stream, 0);
    }
  } else {
    out_stream = net_stream_new(transport, hostname, port);
    if (encoding == SE_None) encoding = SE_Binary; /* default encoding */
  }
  if (out_stream == NULL) {
    logerror ("Failed to create stream for URI %s\n", uri);
    return NULL;
  }

  // Now create a write on top of the stream
  OmlWriter* writer = NULL;

  switch (encoding) {
  case SE_Text:   writer = text_writer_new (out_stream); break;
  case SE_Binary: writer = bin_writer_new (out_stream); break;
  case SE_None:
    logerror ("No encoding specified (this should never happen -- please report this as an OML bug)\n");
    // should cleanup streams
    return NULL;
  }
  if (writer == NULL) {
    logerror ("Failed to create writer for encoding '%s'.\n", encoding == SE_Binary ? "binary" : "text");
    return NULL;
  }
  writer->next = omlc_instance->first_writer;
  omlc_instance->first_writer = writer;

  return writer;
}

/**
 * @brief Find a named measurement point.
 *
 * @param name The name of the measurement point to search for.
 * @return the measurement point, or NULL if not found.
 */
OmlMP *
find_mp (const char *name)
{
  OmlMP *mp = omlc_instance->mpoints;
  while (mp && strcmp (name, mp->name))
    mp = mp->next;
  return mp;
}

/**
 * @brief Find a named field of an MP.
 *
 * @param name name of the field to look up.
 * @return a valid index into the OmlMP's param_defs array, or -1 if not found.
 */
int
find_mp_field (const char *name, OmlMP *mp)
{
  size_t i;
  size_t len = mp->param_count;
  OmlMPDef *fields = mp->param_defs;
  for (i = 0; i < len; i++)
    if (strcmp (name, fields[i].name) == 0)
      return i;
  return -1;
}

/**
 * @brief Create an MString containing a comma-separated list of the
 * fields of the MP.
 *
 * @param mp the MP to summarize
 * @return the fields summary for mp.  The caller is repsonsible for
 * calling mstring_delete() to deallocate the returned MString.
 */
MString *
mp_fields_summary (OmlMP *mp)
{
  int i;
  OmlMPDef *fields = mp->param_defs;
  MString *s = mstring_create();
  mstring_set (s, "'");

  if (!s) return NULL;

  for (i = 0; i < mp->param_count; i++) {
    mstring_cat (s, fields[i].name);
    if (i < mp->param_count - 1)
      mstring_cat (s, "', '");
  }
  mstring_cat (s, "'");

  return s;
}


/** Find a named MStream among the streams attached to an MP.
 *
 * \param name name of the stream to search for
 * \param mp MP in which to search for the stream
 * \return a pointer to the OmlMStream if found, or NULL if not (or name is NULL)
 */
OmlMStream *
find_mstream_in_mp (const char *name, OmlMP *mp)
{
  OmlMStream *ms = mp->streams;

  if (!name)
    return NULL;

  while (ms) {
    if (ms && ms->table_name && strcmp (name, ms->table_name) == 0)
      break;
    ms = ms->next;
  }

  return ms;
}

/**
 * @brief Find a measurement stream by name.  All measurement streams
 * must be named uniquely.
 *
 * @param name the name of the measurement stream to search for.
 * @return a pointer to the measurement stream or NULL if not found.
 */
OmlMStream *
find_mstream (const char *name)
{
  OmlMP *mp = omlc_instance->mpoints;
  OmlMStream *ms = NULL;
  while (mp) {
    ms = find_mstream_in_mp (name, mp);
    if (ms)
      break;
    mp = mp->next;
  }
  return ms;
}

/**
 * @brief Create a new stream of measurement samples from the inputs
 * to a given MP.
 *
 * @param sample_interval the sample interval for the filter
 * @param sample_thres the threshold for the filter
 * @param mp the measurement point
 * @param writer the destination for the stream output samples
 * @return the new measurement stream, or NULL if an error occurred.
 */
OmlMStream*
create_mstream (const char *name,
                OmlMP* mp,
                OmlWriter* writer,
                double sample_interval,
                int sample_thres)
{
  if (!mp || !writer)
    return NULL;
  OmlMStream* ms = (OmlMStream*)malloc(sizeof(OmlMStream));
  MString *namestr = mstring_create();
  char *stream_name = NULL;
  memset(ms, 0, sizeof(OmlMStream));

  ms->sample_interval = sample_interval;
  ms->sample_thres = sample_thres;
  ms->mp = mp;
  ms->writer = writer;
  ms->next = NULL;

  mstring_set (namestr, omlc_instance->app_name);
  mstring_cat (namestr, "_");
  if (name)
    mstring_cat (namestr, name);
  else
    mstring_cat (namestr, mp->name);
  stream_name = mstring_buf (namestr);

  if (find_mstream (stream_name)) {
    logerror ("Measurement stream '%s' already exists; cannot create duplicate in MP '%s':  %s\n",
              name ? name : mp->name,
              mp->name,
              name
              ? " Choose another name in the <stream name=\"...\"> attribute."
              : " Consider using the <stream name=\"...\"> attribute.");

    free (ms);
    mstring_delete (namestr);
    return NULL;
  }
  const size_t tnlen = sizeof(ms->table_name);
  strncpy (ms->table_name, stream_name, tnlen);
  if (ms->table_name[tnlen-1] != '\0')
    ms->table_name[tnlen-1] = '\0';
  mstring_delete (namestr);

  if (ms->sample_interval > 0) {
    if (mp->mutexP == NULL) {
      mp->mutexP = &mp->mutex;
      pthread_mutex_init(mp->mutexP, NULL);
    }
    ms->sample_interval = sample_interval;
    ms->sample_thres = 0;
  } else {
    ms->sample_thres = sample_thres;
  }
  return ms;
}
/**
 *
 * @brief Loop through registered measurment points and define sample
 * based filters with sampling rate '1' and 'FIRST' filters
 *
 * @return 0 if successful -1 otherwise
 */
static int
default_configuration(void)
{
  OmlWriter* writer;
  if ((writer = create_writer(omlc_instance->collection_uri,
                              omlc_instance->default_encoding)) == NULL) {
    return -1;
  }

  int sample_count = omlc_instance->sample_count;
  if (sample_count == 0)
    sample_count = omlc_instance->sample_count = 1;
  double sample_interval = omlc_instance->sample_interval;

  OmlMP* mp = omlc_instance->mpoints;
  while (mp != NULL) {
    OmlMStream* ms = create_mstream (NULL, mp, writer, sample_interval, sample_count );
    if (ms) {
      mp->streams = ms;
      ms->mp = mp;
      create_default_filters(mp, ms);
      if (sample_interval > 0)
        filter_engine_start(ms);
    }
    mp = mp->next;
  }
  return 0;
}
/**
 * @brief create the default filters
 * @param mp the associated measurement point
 * @param ns the associated measurement stream
 */
void
create_default_filters(OmlMP *mp, OmlMStream *ms)
{
  int param_count = mp->param_count;

  int j;
  OmlFilter* prev = NULL;
  for (j = 0; j < param_count; j++) {
    OmlMPDef def = mp->param_defs[j];
    OmlFilter* f = create_default_filter(&def, ms, j);
    if (f) {
      if (prev == NULL) {
        ms->firstFilter = f;
      } else {
        prev->next = f;
      }
      prev = f;
    } else {
      logerror("Unable to create default filter for MP %s.\n", mp->name);
    }
  }
}
/**
 * @brief Create a new filter for the measurement associated with the stream
 * @param def the oml definition
 * @param ms the stream to filter
 * @param index the index in the measurement point
 * @return a new filter
 */
OmlFilter*
create_default_filter(OmlMPDef *def, OmlMStream *ms, int index)
{
  const char* name = def->name;
  OmlValueT type = def->param_types;
  int multiple_samples = ms->sample_thres > 1 || ms->sample_interval > 0;

  char* fname;
  if (multiple_samples && omlc_is_numeric_type (type)) {
    fname = "avg";
  } else {
    fname = "first";
  }
  OmlFilter* f = create_filter(fname, name, type, index);
  return f;
}
/**
 * @brief write the meta data for the application
 * @return 0 if successful
 */
static int
write_meta(void)
{
  OmlWriter* writer = omlc_instance->first_writer;
  for (; writer != NULL; writer = writer->next) {
    char s[128];
    sprintf(s, "protocol: %d", OML_PROTOCOL_VERSION);
    writer->meta(writer, s);
    sprintf(s, "experiment-id: %s", omlc_instance->domain);
    writer->meta(writer, s);
    sprintf(s, "start_time: %d", (int)omlc_instance->start_time);
    writer->meta(writer, s);
    sprintf(s, "sender-id: %s", omlc_instance->node_name);
    writer->meta(writer, s);
    sprintf(s, "app-name: %s", omlc_instance->app_name);
    writer->meta(writer, s);
  }

  OmlMP* mp = omlc_instance->mpoints;
  int index = 1;
  while (mp != NULL) {
    OmlMStream* ms = mp->streams;
    for(; ms != NULL; ms = ms->next) {
      write_schema(ms, index++);
    }
    mp = mp->next;
  }

  writer = omlc_instance->first_writer;
  for (; writer != NULL; writer = writer->next) {
    writer->header_done(writer);   // end of header
  }
  return 0;
}


/**
 * @brief Write the different schemas of the application
 * @param ms the stream definition
 * @param index the index in the measurement points
 * @return 0 if successful
 */
#define DEFAULT_SCHEMA_LENGTH 512
static int
write_schema(OmlMStream *ms, int index)
{
  char s[DEFAULT_SCHEMA_LENGTH];
  const size_t bufsize = sizeof (s) / sizeof (s[0]);
  size_t schema_size = DEFAULT_SCHEMA_LENGTH;
  char* schema = (char*)malloc (schema_size * sizeof (char));
  size_t count = 0;
  size_t n = 0;
  OmlWriter* writer = ms->writer;

  ms->index = index;
  n = snprintf(s, bufsize, "schema: %d %s ", ms->index, ms->table_name);

  if (n >= bufsize)
    {
      logerror("Schema generation failed because the following table name was too long: %s\n", ms->table_name);
      free (schema);
      return -1;
    }

  strncpy (schema, s, n + 1);
  count += n;

  // Loop over all the filters
  OmlFilter* filter = ms->filters;
  for (; filter != NULL; filter = filter->next) {
    const char* prefix = filter->name;
    int j;
    for (j = 0; j < filter->output_count; j++) {
      char* name;
      OmlValueT type;
      if (filter->meta(filter, j, &name, &type) != -1) {
        char *type_s = oml_type_to_s(type);
        if (name == NULL) {
          n = snprintf(s, bufsize, "%s:%s ", prefix, type_s);
        } else {
          n = snprintf(s, bufsize, "%s_%s:%s ", prefix, name, type_s);
        }

        if (n >= bufsize)
          {
            logerror("One of the schema entries for table %s was too long:\n\t%s\t%s\n",
                   prefix, type_s);
            free (schema);
            return -1;
          }

        if (count + n >= schema_size)
          {
            schema_size += DEFAULT_SCHEMA_LENGTH;
            char* new = (char*)malloc (schema_size * sizeof (char));
            strncpy (new, schema, count);
            free (schema);
            schema = new;
          }
        strncpy (&schema[count], s, n + 1);
        count += n;
      } else {
        logwarn("Filter %s failed to provide meta information for index %d.\n",
              filter->name,
              j);
      }
    }
  }
  writer->meta(writer, schema);
  free (schema);
  return 0;
}

/**
 *  Validate the name of the application.
 *
 *  If the application name contains a '/', it is truncated to the
 *  sub-string following the final '/'.  If the application name
 *  contains any characters other than alphanumeric characters or an
 *  underscore, it is declared invalid.  The first character must not
 *  be a digit.  Whitespace is not allowed.  An empty string is also
 *  not allowed.
 *
 *  @param name the name of the application.
 *
 *  @return If the application name is valid, a pointer to the
 *  application name, possibly truncated, is returned.  If the
 *  application name is not valid, returns NULL.  If the returned
 *  pointer is not NULL, it may be different from name.
 *
 */
const char*
validate_app_name (const char* name)
{
  size_t len = strlen (name);
  const char* p = name + len - 1;

  while (p >= name) {
    if (*p == '/')
      break;
    p--;
  }
  /* Either 1 character before name, or '/': need to move forward 1 character */
  p++;

  if (validate_name (p))
    return p;
  else
    return NULL;
}

static struct {
  const char *name;
  void (*enable)(void);
} feature_table [] = {
  { "default-log-simple", o_set_simplified_logging },
};
static const size_t feature_count = sizeof (feature_table) / sizeof (feature_table[0]);

/*
 * Parse features and enable the ones that are recognized.  features
 * should be a semicolon-separated list of features.
 *
 */
static void
setup_features (const char * const features)
{
  size_t i = 0;
  const char *p = features, *q;

  if (features == NULL)
    return;

  while (p && *p) {
    size_t len;
    char *name;
    q    = strchr(p, ';');
    len  = q ? (size_t)(q - p) : strlen (p);
    name = xstrndup (p, len);
    p    = q ? (q + 1) : q;

    for (i = 0; i < feature_count; i++)
      if (strcmp (name, feature_table[i].name) == 0)
        feature_table[i].enable();
    xfree (name);
  }
}

/* 
 * Generate default file name to use when no output parameters are given.
 *
 * @param app_ame	the name of the application
 * @param name		the OML ID of the instance
 * @param domain	the experimental domain
 *
 * @return	A statically allocated buffer containing the URI of the output
 */
static char*
default_uri(const char *app_name, const char *name, const char *domain)
{
  /* Use a statically allocated buffer to avoid having to free it,
   * just like other URI sources in omlc_init() */
  static char uri[256];
  int remaining = sizeof(uri) - 1; /* reserve 1 for the terminating null byte */
  char *protocol = "file:";
  char time[25];
  struct timeval tv;

  gettimeofday(&tv, NULL);
  strftime(time, sizeof(time), "%Y-%m-%dt%H.%M.%S%z", localtime(&tv.tv_sec));

  *uri = 0;
  strncat(uri, protocol, remaining);
  remaining -= sizeof(protocol);

  strncat(uri, app_name, remaining);
  remaining -= strlen(app_name);

  if (name) {
    strncat(uri, "_", remaining);
    remaining--;
    strncat(uri, name, remaining);
    remaining -= strlen(name);
  }

  if (domain) {
    strncat(uri, "_", remaining);
    remaining--;
    strncat(uri, domain, remaining);
  }

  strncat(uri, "_", remaining);
  remaining--;
  strncat(uri, time, remaining);

  return uri;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
