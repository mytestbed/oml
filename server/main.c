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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <popt.h>

#include <oml2/oml_writer.h>
#include <log.h>
#include <mem.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "util.h"
#include "version.h"
#include "client_handler.h"
#include "sqlite_adapter.h"
#ifdef HAVE_PG
#include "psql_adapter.h"
#endif

void
die (const char *fmt, ...)
{
  va_list va;
  va_start (va, fmt);
  o_vlog (O_LOG_ERROR, fmt, va);
  va_end (va);
  exit (EXIT_FAILURE);
}

#define DEFAULT_PORT 3003
#define DEFAULT_PORT_STR "3003"
#define DEFAULT_LOG_FILE "oml_server.log"
#define DEFAULT_DB_HOST "localhost"
#define DEFAULT_DB_USER "dbuser"
#define DEFAULT_DB_BACKEND "sqlite"

static int listen_port = DEFAULT_PORT;
char* g_database_data_dir = ".";

static int log_level = O_LOG_INFO;
static char* logfile_name = NULL;
static char* hostname = DEFAULT_DB_HOST;
static char* user = DEFAULT_DB_USER;
static char* backend = DEFAULT_DB_BACKEND;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen", 'l', POPT_ARG_INT, &listen_port, 0, "Port to listen for TCP based clients", DEFAULT_PORT_STR},
  { "db", 'b', POPT_ARG_STRING, &backend, 0, "Database server backend", DEFAULT_DB_BACKEND},
  { "hostname", 'h', POPT_ARG_STRING, &hostname, 0, "Database server hostname", DEFAULT_DB_HOST},
  { "user", 'u', POPT_ARG_STRING, &user, 0, "Database server username", DEFAULT_DB_USER},
  { "data-dir", '\0', POPT_ARG_STRING, &g_database_data_dir, 0, "Directory to store database files (sqlite)", "DIR" },
  { "debug-level", 'd', POPT_ARG_INT, &log_level, 0, "Increase debug level", "{1 .. 4}"  },
  { "logfile", '\0', POPT_ARG_STRING, &logfile_name, 0, "File to log to", DEFAULT_LOG_FILE },
  { "version", 'v', 0, 0, 'v', "Print version information and exit", NULL },
  { NULL, 0, 0, NULL, 0, NULL, NULL }
};

static struct db_backend
{
  const char * const name;
  db_adapter_create fn;
} backends [] =
  {
    { "sqlite", sq3_create_database },
#if HAVE_PG
    { "postgresql", psql_create_database },
#endif
  };

const char *
valid_backends ()
{
  static char s[256] = {0};
  int i;
  if (s[0] == '\0') {
    char *p = s;
    for (i = LENGTH (backends) - 1; i >= 0; i--) {
      int n = sprintf(p, "%s", backends[i].name);
      if (i) {
        p[n++] = ',';
        p[n++] = ' ';
      }
      p[n] = '\0';
      p += n;
    }
  }
  return s;
}

db_adapter_create
database_create_function ()
{
  size_t i = 0;
  for (i = 0; i < LENGTH (backends); i++)
    if (!strncmp (backend, backends[i].name, strlen (backends[i].name)))
      return backends[i].fn;

  return NULL;
}

/**
 * @brief Set up the logging system.
 *
 * This function sets up the server logging system to log to file
 * logfile, with the given log verbosity level.  All messages with
 * severity less than or equal to level will be logged; all others
 * will be discarded (lower levels are more important).
 *
 * If logfile is not NULL then the named file is opened for logging.
 * If logfile is NULL and the application's stderr stream is not
 * attached to a tty, then the file DEF_LOG_FILE is opened for
 * logging; otherwise, if logfile is NULL and stderr is attached to a
 * tty then log messages will sent to stderr.
 *
 * @param logfile the file to open
 * @param level the severity level at which to log
 */
void
setup_logging (char *logfile, int level)
{
  if (!logfile) {
    if (isatty (fileno (stderr)))
      logfile = "-";
    else
      logfile = DEFAULT_LOG_FILE;
  }

  o_set_log_file(logfile);
  o_set_log_level(level);
  extern void _o_set_simplified_logging (void);
  _o_set_simplified_logging ();
}

/**
 * \brief Called when a node connects via TCP
 * \param new_sock
 */
void
on_connect(Socket* new_sock, void* handle)
{
  (void)handle;
  ClientHandler *client = client_handler_new(new_sock,hostname,user);
  loginfo("'%s': new client connected\n", client->name);
}

int
main(int argc, const char **argv)
{
  char c;
  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);

  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {
    case 'v':
      printf(V_STRING, VERSION);
      printf("OML Protocol V%d\n", OML_PROTOCOL_VERSION);
      printf(COPYRIGHT);
      return 0;
    }
  }

  setup_logging (logfile_name, log_level);

  if (c < -1)
    die ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

  loginfo(V_STRING, VERSION);
  loginfo("OML Protocol V%d\n", OML_PROTOCOL_VERSION);
  loginfo(COPYRIGHT);

  if (!database_create_function ())
      die ("Unknown database backend '%s' (valid backends: %s)\n",
           backend, valid_backends ());

  loginfo ("Database backend: '%s'\n", backend);
  const char *pg = "postgresql";
  const char *sq = "sqlite";
  if (!strcmp (backend, pg))
    loginfo ("PostgreSQL backend is still experimental!\n");
  if (strcmp (backend, sq))
    loginfo ("Database server: %s with %s\n", hostname, user);

  eventloop_init();

  Socket* server_sock;
  server_sock = socket_server_new("server", listen_port, on_connect, NULL);

  if (!server_sock)
    die ("Failed to create socket (port %d) to listen for client connections.\n", listen_port);

  eventloop_run();

  xmemreport();

  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
