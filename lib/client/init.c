/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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

#include <config.h>
#include <log.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#include <oml_value.h>
#include <mem.h>
#include <validate.h>
#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "filter/factory.h"
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

extern int parse_config(char* config_file);
extern void _o_set_simplified_logging (void);
extern void _o_set_default_logging (void);
extern int  _o_oldstyle_logging (void);

/**
 * @brief function called by the application to initialise the oml measurements
 * @param name the name of the application
 * @param pargc the argc of the command line of the application
 * @param argv the argv of the command line of the application
 * @param custom_oml_log the reference to the log file of oml
 * @return 0 on success, -1 on failure.
 */
int
omlc_init(const char* application, int* pargc, const char** argv, o_log_fn custom_oml_log)
{
  const char *app_name = validate_app_name (application);
  const char* name = NULL;
  const char* experimentId = NULL;
  const char* config_file = NULL;
  const char* local_data_file = NULL;
  const char* server_uri = NULL;
  int sample_count = 0;
  double sample_interval = 0.0;
  const char** arg = argv;

  if (!app_name) {
    logerror("Found illegal whitespace in application name '%s'\n", application);
    return -1;
  }

  omlc_instance = NULL;

  o_set_log_level(O_LOG_INFO);
  if (custom_oml_log)
    o_set_log (custom_oml_log);
  else
    _o_set_default_logging();

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
      } else if (strcmp(*arg, "--oml-exp-id") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-exp-id'\n");
          return -1;
        }
        experimentId = *++arg;
        *pargc -= 2;
      } else if (strcmp(*arg, "--oml-file") == 0) {
        if (--i <= 0) {
          logerror("Missing argument for '--oml-file'\n");
          return -1;
        }
        local_data_file = *++arg;
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
        server_uri = (char*)*++arg;
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
  if (experimentId == NULL)
    experimentId = getenv("OML_EXP_ID");
  if (config_file == NULL)
    config_file = getenv("OML_CONFIG");
  if (local_data_file == NULL && server_uri == NULL)
    server_uri = getenv("OML_SERVER");

  setup_features (getenv ("OML_FEATURES"));

  omlc_instance = &instance_storage;
  memset(omlc_instance, 0, sizeof(OmlClient));

  omlc_instance->app_name = app_name;
  omlc_instance->node_name = name;
  omlc_instance->experiment_id = experimentId;
  omlc_instance->sample_count = sample_count;
  omlc_instance->sample_interval = sample_interval;

  if (local_data_file != NULL) {
    // dump every sample into local_data_file
    if (local_data_file[0] == '-')
      local_data_file = "stdout";
    snprintf(omlc_instance->server_uri, SERVER_URI_MAX_LENGTH, "file:%s", local_data_file);
  } else if (server_uri != NULL) {
    strncpy(omlc_instance->server_uri, server_uri, SERVER_URI_MAX_LENGTH);
  }
  omlc_instance->config_file = config_file;

  register_builtin_filters ();

  loginfo ("OML Client V%s [Protocol V%d] %s\n",
           VERSION,
           OML_PROTOCOL_VERSION,
           OMLC_COPYRIGHT);

  if (_o_oldstyle_logging ()) {
    o_log (O_LOG_WARN,
           "Old style logging (\"default-log-mixed\") selected; "
           "OML v2.7.0 will make new style logging the default; "
           "see man liboml2(1) for details\n");
  }

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
    if (omlc_instance->server_uri == NULL) {
      logerror("Missing either --oml-file or --oml-server declaration.\n");
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
omlc_close()
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
  printf("  --oml-file file        .. Writes measurements to 'file'\n");
  printf("  --oml-id id            .. Name to identify this app instance\n");
  printf("  --oml-exp-id expId     .. Name to experiment DB\n");
  printf("  --oml-server uri       .. URI of server to send measurements to\n");
  printf("  --oml-config file      .. Reads configuration from 'file'\n");
  printf("  --oml-samples count    .. Default number of samples to collect\n");
  printf("  --oml-interval seconds .. Default interval between measurements\n");
  printf("  --oml-log-file file    .. Writes log messages to 'file'\n");
  printf("  --oml-log-level level  .. Log level used (error: 1 .. debug:4)\n");
  printf("  --oml-noop             .. Do not collect measurements\n");
  printf("  --oml-list-filters     .. List the available types of filters\n");
  printf("  --oml-help             .. Print this message\n");
  printf("\n");
  printf("Valid URI: tcp|udp:host:port:[bindAddr] or file:localPath\n");
  printf("    The optional 'bindAddr' is used for multicast conections\n");
  printf("\n");
  printf("The following environment variables are recognized:\n");
  printf("  OML_NAME=id            .. Name to identify this app instance (--oml-id)\n");
  printf("  OML_EXP_ID=expId       .. Name to experiment DB (--oml-exp-id)\n");
  printf("  OML_CONFIG=file        .. Read configuration from 'file' (--oml-config)\n");
  printf("  OML_SERVER=uri         .. URI of server to send measurments to (--oml-server)\n");
  printf("\n");
  printf("If the corresponding command line option is present, it overrides\n");
  printf("the environment variable.  Note that OML_SERVER accepts the file://\n");
  printf("protocol.\n");
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

/**
 * @brief Creates either a file writer or a network writer
 * @param server_uri the option file or server and the output
 * @return a writer
 */
OmlWriter*
create_writer(char* server_uri)
{
  if (omlc_instance == NULL){
    logerror("No omlc_instance:  OML client was not initialized properly.\n");
    return NULL;
  }

  OmlWriter* writer = NULL;
  char* p = server_uri;
  if (p == NULL) {
    logerror("Missing server definition (e.g. --oml-server)\n");
    return 0;
  }
  char* proto = p;
  while (*p != '\0' && *p != ':') p++;
  if (*p != '\0') *(p++) = '\0';
  if (strcmp(proto, "file") == 0) {
    writer = file_writer_new(p);
  } else {
    // Net writer needs NAME and EXPERIMENT_ID
    if (omlc_instance->node_name == NULL) {
      logerror("Missing '--oml-id' flag \n");
      return NULL;
    }
    if (omlc_instance->experiment_id == NULL) {
      logerror("Missing '--oml-exp-id' flag \n");
      return NULL;
    }
    writer = net_writer_new(proto, p);
  }
  if (writer != NULL) {
    writer->next = omlc_instance->first_writer;
    omlc_instance->first_writer = writer;
  } else {
    logwarn("Failed to create writer for URI %s\n", server_uri);
  }
  return writer;
}
/**
 * @brief Function called when creating a new stream of measurement
 * @param sample_interval the sample interval for the filter
 * @param sample_thres the threshold for the filter
 * @param mp the measurement point
 * @param writer the oml writer
 * @return the new measurement stream
 */
OmlMStream*
create_mstream(double sample_interval,
               int    sample_thres,
               OmlMP* mp,
               OmlWriter* writer)
{
  OmlMStream* ms = (OmlMStream*)malloc(sizeof(OmlMStream));
  memset(ms, 0, sizeof(OmlMStream));

  ms->sample_interval = sample_interval;
  ms->sample_thres = sample_thres;
  ms->mp = mp;
  ms->writer = writer;
  ms->next = NULL;
  if (mp->table_count++ > 0) {
    sprintf(ms->table_name, "%s_%s_%d", omlc_instance->app_name,
              mp->name, mp->table_count);
  } else {
    sprintf(ms->table_name, "%s_%s", omlc_instance->app_name, mp->name);
  }

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
  if ((writer = create_writer(omlc_instance->server_uri)) == NULL) {
    return -1;
  }

  int sample_count = omlc_instance->sample_count;
  if (sample_count == 0) {
    sample_count = omlc_instance->sample_count = 1;
  }
  double sample_interval = omlc_instance->sample_interval;

  OmlMP* mp = omlc_instance->mpoints;
  while (mp != NULL) {
    OmlMStream* ms = create_mstream(sample_interval, sample_count, mp, writer);
    mp->firstStream = ms;
    ms->mp = mp;

    create_default_filters(mp, ms);

    if (sample_interval > 0) {
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
    sprintf(s, "experiment-id: %s", omlc_instance->experiment_id);
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
    OmlMStream* ms = mp->firstStream;
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
  OmlFilter* filter = ms->firstFilter;
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
  { "default-log-simple", _o_set_simplified_logging },
  { "default-log-mixed", _o_set_default_logging },
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
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
