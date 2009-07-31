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
/*!\file init.c
  \brief Implements OML's client side initialization
*/

#include <ocomm/o_log.h>
#include <string.h>
#include <malloc.h>
#include <errno.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>

#include "oml2/oml.h"
#include "oml2/oml_filter.h"
#include "filter/factory.h"
#include "client.h"
#include "version.h"

OmlClient* omlc_instance = NULL;

#define DEF_LOG_LEVEL = O_LOG_INFO;

static OmlClient instance_storage;

static void usage(void);
static void print_filters(void);
//static int configure(char* configFile);
static int default_configuration();
static int write_meta();
static int write_schema(
  OmlMStream* ms,
  int index
);

static void termination_handler(int signum);
static void install_close_handler();

extern int parse_config(char* configFile);

/**
 * \fn int omlc_init( const char* appName, int* argcPtr, const char** argv, oml_log_fn custom_oml_log)
 * \brief function called by the application to initialise the oml measurements
 * \param appName the name of the application
 * \param argcPtr the argc of the command line of the application
 * \param agrv the argv of the command line of the application
 * \param custom_oml_log the reference to the log file of oml
 * \return 0 when finished
 */
int
omlc_init(
  const char* appName,
  int* argcPtr,
  const char** argv,
  oml_log_fn custom_oml_log
) {

  o_set_log((o_log_fn)custom_oml_log);
  //  o_set_log_level(DEF_LOG_LEVEL);
  o_set_log_level(3);


  const char* appliName = appName;
  const char* p = appliName + strlen(appName);
  while (! (p == appliName || *p == '/')) p--;
  if (*p == '/') p++;
  appliName = p;



  omlc_instance = NULL;
  const char* name = NULL;
  const char* experimentId = NULL;
  const char* configFile = NULL;
  const char* localDataFile = NULL;
  const char* serverUri = NULL;
  int sample_count = 0;
  double sample_interval = 0.0;
  const char** argvPtr = argv;
  int i;
  for (i = *argcPtr; i > 0; i--, argvPtr++) {
    if (strcmp(*argvPtr, "--oml-id") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument for '--oml-id'\n");
        return 0;
      }
      name = *++argvPtr;
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-exp-id") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument for '--oml-exp-id'\n");
        return 0;
      }
      experimentId = *++argvPtr;
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-file") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument for '--oml-file'\n");
        return 0;
      }
      localDataFile = *++argvPtr;
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-config") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument for '--oml-config'\n");
        return 0;
      }
      configFile = *++argvPtr;
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-samples") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument to '--oml-samples'\n");
        return 0;
      }
      sample_count = atoi(*++argvPtr);
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-interval") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument to '--oml-interval'\n");
        return 0;
      }
      sample_interval = atof(*++argvPtr);
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-log-file") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument to '--oml-log-file'\n");
        return 0;
      }
      o_set_log_file((char*)*++argvPtr);
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-log-level") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument to '--oml-log-level'\n");
        return 0;
      }
      o_set_log_level(atoi(*++argvPtr));
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-server") == 0) {
      if (--i <= 0) {
        o_log(O_LOG_ERROR, "Missing argument for '--oml-server'\n");
        return 0;
      }
      serverUri = (char*)*++argvPtr;
      *argcPtr -= 2;
    } else if (strcmp(*argvPtr, "--oml-noop") == 0) {
      *argcPtr -= 1;
      omlc_instance = NULL;
      return 1;
    } else if (strcmp(*argvPtr, "--oml-help") == 0) {
      usage();
      *argcPtr -= 1;
      exit(0);
    } else if (strcmp(*argvPtr, "--oml-list-filters") == 0) {
      print_filters();
      *argcPtr -= 1;
      exit(0);
    } else {
      *argv++ = *argvPtr;
    }
  }

  //oml_marshall_test();


  o_log(O_LOG_INFO, "OML Client V%d.%d.%d %s\n",
  OMLC_MAJOR_VERSION, OMLC_MINOR_VERSION, OMLC_REVISION, OMLC_COPYRIGHT);

  if (name == NULL)
    name = getenv("OML_NAME");
  if (experimentId == NULL)
    experimentId = getenv("OML_EXP_ID");
  if (configFile == NULL)
    configFile = getenv("OML_CONFIG");

//  if ((name == NULL) && (configFile == NULL) && (localDataFile == NULL)) {
//    o_log(O_LOG_ERROR,
//    "Could not get either OML_NAME or OML_CONFIG from env\n");
//    return 0;
//  }

  omlc_instance = &instance_storage;
  memset(omlc_instance, 0, sizeof(OmlClient));

  omlc_instance->app_name = appliName;
  omlc_instance->node_name = name;
  omlc_instance->experiment_id = experimentId;
  omlc_instance->sample_count = sample_count;
  omlc_instance->sample_interval = sample_interval;



  if (localDataFile != NULL) {
    // dump every sample into localDataFile
    if (localDataFile[0] == '-')
      localDataFile = "stdout";
    snprintf(omlc_instance->serverUri, SERVER_URI_MAX_LENGTH, "file:%s", localDataFile);
  } else if (serverUri != NULL) {
    strncpy(omlc_instance->serverUri, serverUri, SERVER_URI_MAX_LENGTH);
  }
  omlc_instance->configFile = configFile;

  register_builtin_filters ();

  return 0;
}

/**
 * \fn OmlMP* omlc_add_mp(const char* mp_name, OmlMPDef*   mp_def)
 * \brief Register a measurement point. Needs to be called for every measurment point AFTER +omlc_init+ and before a final +omlc_start+.
 * \param  mp_name the name of the measurement point
 * \param mp_def the definition of the set of measurements in this measurement point
 * \return a new measurement point
 */
OmlMP*
omlc_add_mp(
  const char* mp_name,
  OmlMPDef*   mp_def
) {
  if (omlc_instance == NULL) return NULL;

  OmlMP* mp = (OmlMP*)malloc(sizeof(OmlMP));
  memset(mp, 0, sizeof(OmlMP));

  mp->name = mp_name;
  mp->param_defs = mp_def;
  int pc = 0;
  OmlMPDef* dp = mp_def;
  while (dp != NULL && dp->name != NULL) {
    pc++;
    dp++;
  }
  mp->param_count = pc;
  mp->active = 1;  // should most likely only be set if there is an
                    // attached MStream
  mp->next = omlc_instance->mpoints;
  omlc_instance->mpoints = mp;

  return mp;
}

/**
 * \fn int omlc_start()
 * \brief Finalizes inital configurations and get ready for consecutive +omlc_process+ calls
 * \return 0 if successful, <0 otherwise
 */
int
omlc_start()

{
  if (omlc_instance == NULL) return -1;


  struct timeval tv;
  gettimeofday(&tv, NULL);
  omlc_instance->start_time = tv.tv_sec;

//  omlc_instance->mp_count = mp_count;
//  omlc_instance->mp_definitions = mp_definitions;


  const char* configFile = omlc_instance->configFile;
  if (configFile) {
    if (parse_config((char*)configFile)) {
      o_log(O_LOG_ERROR, "Error while parsing configuration '%s'\n", configFile);
      omlc_instance = NULL;
      return -1;
    }
  } else {
    if (omlc_instance->serverUri == NULL) {
      o_log(O_LOG_ERROR, "Missing either --oml-file or --oml-server declaration.\n");
      omlc_instance = NULL;
      return -2;
    }
    if (default_configuration()) {
      omlc_instance = NULL;
      return -3;
    }
  }
  install_close_handler();
  write_meta();
  return 0;
}

/**
 * \fn static void termination_handler( int signum)
 * \brief Close the oml process
 * \param signum the signal number
 */
static void
termination_handler(
  int signum
) {
  o_log(O_LOG_DEBUG, "Closing OML (%d)\n", signum);
  omlc_close();
  exit(-1 * signum);
}

/**
 * \fn static void install_close_handler()
 * \brief start the signal handler
 */
static void
install_close_handler()

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
}

/**
 * \fn int omlc_close()
 * \brief Finalizes all open connections. Any futher calls to +omlc_process+ are being ignored
 * \return -1 if fails
 */
int
omlc_close()

{
  if (omlc_instance == NULL) return -1;

  OmlWriter* w = omlc_instance->firstWriter;
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
 * \fn static void usage()
 * \brief print the possible OML parameters
 */
static void
usage()

{
  printf("OML Client V%d.%d.%d\n", OMLC_MAJOR_VERSION, OMLC_MINOR_VERSION, OMLC_REVISION);
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
}

static void
print_filters(void)
{
  register_builtin_filters();

  printf("OML Client V%d.%d.%d\n", OMLC_MAJOR_VERSION, OMLC_MINOR_VERSION, OMLC_REVISION);
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
 * \fn OmlWriter* create_writer( char* serverUri)
 * \brief Creates either a file writer or a network writer
 * \param serverUri the option file or server and the output
 * \return a writer
 */
OmlWriter*
create_writer(
  char* serverUri
) {
  if (omlc_instance == NULL){
    o_log(O_LOG_ERROR,"no omlc\n");

    return NULL;
  }

  OmlWriter* writer = NULL;
  char* p = serverUri;
  if (p == NULL) {
    o_log(O_LOG_ERROR, "Missing server definition (e.g. --oml-server)\n");
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
      o_log(O_LOG_ERROR, "Missing '--oml-id' flag \n");
      return NULL;
    }
    if (omlc_instance->experiment_id == NULL) {
      o_log(O_LOG_ERROR, "Missing '--oml-exp-id' flag \n");
      return NULL;
    }

    writer = net_writer_new(proto, p);
  }
  if (writer != NULL) {
    writer->next = omlc_instance->firstWriter;
    omlc_instance->firstWriter = writer;
  }
  return writer;
}
/**
 * \fn OmlMStream* create_mstream(double sample_interval, int    sample_thres, OmlMP* mp, OmlWriter* writer)
 * \brief Function called when creating a new stream of measurement
 * \param sample_interval the sample interval for the filter
 * \param sample_thres the threshold for the filter
 * \param mp the measurement point
 * \param writer the oml writer
 * \return the new measurement stream
 */
OmlMStream*
create_mstream(
    double sample_interval,
    int    sample_thres,
    OmlMP* mp,
    OmlWriter* writer
) {
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
    pthread_cond_init(&ms->condVar, NULL);
    ms->sample_interval = sample_interval;
    ms->sample_thres = 0;
    //filter_engine_start(ms);
  } else {
    ms->sample_thres = sample_thres;
  }
  return ms;
}
/**
 * \fn static int default_configuration()
 *
 * \brief Loop through registered measurment points and define sample
 * based filters with sampling rate '1' and 'FIRST' filters
 *
 * \return 0 if successful -1 otherwise
 */
static int
default_configuration()

{
  OmlWriter* writer;
  if ((writer = create_writer(omlc_instance->serverUri)) == NULL) {
    return -1;
  }

  int sample_count = omlc_instance->sample_count;
  if (sample_count == 0) {
    sample_count = omlc_instance->sample_count = 1;
  }
  double sample_interval = omlc_instance->sample_interval;


  OmlMP* mp = omlc_instance->mpoints;
  while (mp != NULL) {
    //OmlMPDef* dp = omlc_instance->mp_definitions[i];
    OmlMStream* ms = create_mstream(sample_interval, sample_count, mp, writer);
    mp->firstStream = ms;
    ms->mp = mp;

    createDefaultFilters(mp, ms);

    if (sample_interval > 0) {
      filter_engine_start(ms);
    }


    mp = mp->next;
    // Create thread to write measurements to file
    //pthread_create(&thread, NULL, slow_producer, (void*)ms);

  }
  return 0;
}
/**
 * \fn void createDefaultFilters(OmlMP*      mp, OmlMStream* ms)
 * \brief create the default filters
 * \param mp the associated measurement point
 * \param ns the associated measurement stream
 */
void
createDefaultFilters(
    OmlMP*      mp,
    OmlMStream* ms
) {
  int param_count = mp->param_count;
  //ms->filters = (OmlFilter**)malloc(param_count * sizeof(OmlFilter*));
  int j;
  OmlFilter* prev = NULL;
  for (j = 0; j < param_count; j++) {
    OmlMPDef def = mp->param_defs[j];
    OmlFilter* f = createDefaultFilter(&def, ms, j);
    if (f) {
      if (prev == NULL) {
	ms->firstFilter = f;
      } else {
	prev->next = f;
      }
      prev = f;
    } else {
      o_log(O_LOG_ERROR, "Unable to create default filter for MP %s.\n", mp->name);
    }
  }
}
/**
 * \fn OmlFilter* createDefaultFilter(OmlMPDef*   def, OmlMStream* ms, int index)
 * \brief Create a new filter for the measurement associated with the stream
 * \param def the oml definition
 * \param ms the stream to filter
 * \param index the index in the measurement point
 * \return a new filter
 */
OmlFilter*
createDefaultFilter(
    OmlMPDef*   def,
    OmlMStream* ms,
    int         index
) {
  const char* name = def->name;
  OmlValueT type = def->param_types;
  int multiple_samples = ms->sample_thres > 1 || ms->sample_interval > 0;

  char* fname;
  if (multiple_samples && (type == OML_LONG_VALUE || type == OML_DOUBLE_VALUE)) {
  //if (type == OML_LONG_VALUE || type == OML_DOUBLE_VALUE) {
    fname = "avg";
  } else {
    fname = "first";
  }
  OmlFilter* f = create_filter(fname, name, type, index);
  return f;
}
/**
 * \fn static int write_meta()
 * \brief write the meta data for the application
 * \return 0 if successful
 */
static int
write_meta()

{
  OmlWriter* writer = omlc_instance->firstWriter;
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

  writer = omlc_instance->firstWriter;
  for (; writer != NULL; writer = writer->next) {
    writer->header_done(writer);   // end of header
  }
  return 0;
}


/**
 * \fn static int write_schema(OmlMStream* ms, int index)
 * \brief Write the different schemas of the application
 * \param ms the stream definition
 * \param index the index in the measurement points
 * \return 0 if successful
 */
static int
write_schema(
  OmlMStream* ms,
  int index
) {
  char s[512];
  OmlWriter* writer = ms->writer;

  ms->index = index;
  sprintf(s, "schema: %d %s ", ms->index, ms->table_name);

  // Loop over all the filters
  OmlFilter* filter = ms->firstFilter;
  for (; filter != NULL; filter = filter->next) {
    const char* prefix = filter->name;
    int j;
    for (j = 0; j < filter->output_count; j++) {
      char* name;
      OmlValueT type;
      if (filter->meta(filter, j, &name, &type) != -1) {
        char* type_s = oml_type_to_s(type);
        if (name == NULL) {
          sprintf(s, "%s %s:%s", s, prefix, type_s);
        } else {
          sprintf(s, "%s %s_%s:%s", s, prefix, name, type_s);
        }
      } else {
	o_log(O_LOG_WARN, "Filter %s failed to provide meta information for index %d.\n",
	      filter->name,
	      j);
      }
    }
  }
  writer->meta(writer, s);
  return 0;
}



/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
