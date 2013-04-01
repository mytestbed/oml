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

/** Main oml2-server functions */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <popt.h>
#include <signal.h>
#include <errno.h>

#include "oml2/oml_writer.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"
#include "mem.h"
#include "oml_util.h"
#include "hook.h"
#include "client_handler.h"
#include "database.h"
#include "sqlite_adapter.h"

#define V_STRING  "OML2 Server V%s\n"

#define COPYRIGHT "Copyright 2007-2013 NICTA\n"

#if HAVE_LIBPQ
#include <libpq-fe.h>
#include "psql_adapter.h"
#endif

void die (const char *fmt, ...)
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

static int listen_port = DEFAULT_PORT;
static int log_level = O_LOG_INFO;
static int socket_timeout = 60;
static char* logfile_name = NULL;
static char* uidstr = NULL;
static char* gidstr = NULL;

extern char* dbbackend;
extern char *sqlite_database_dir;
#if HAVE_LIBPQ
extern char *pg_host;
extern char *pg_port;
extern char *pg_user;
extern char *pg_pass;
extern char *pg_conninfo;
#endif /* HAVE_LIBPQ */

struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen", 'l', POPT_ARG_INT, &listen_port, 0, "Port to listen for TCP based clients", DEFAULT_PORT_STR},
  { "backend", 'b', POPT_ARG_STRING, &dbbackend, 0, "Database server backend", DEFAULT_DB_BACKEND},
  { "data-dir", 'D', POPT_ARG_STRING, &sqlite_database_dir, 0, "Directory to store database files (sqlite)", "DIR" },
#if HAVE_LIBPQ
  { "pg-host", '\0', POPT_ARG_STRING, &pg_host, 0, "PostgreSQL server host to connect to", DEFAULT_PG_HOST },
  { "pg-port", '\0', POPT_ARG_STRING, &pg_port, 0, "PostgreSQL server port to connect to", DEFAULT_PG_PORT },
  { "pg-user", '\0', POPT_ARG_STRING, &pg_user, 0, "PostgreSQL user to connect as", DEFAULT_PG_USER },
  { "pg-pass", '\0', POPT_ARG_STRING, &pg_pass, 'p', "Password of the PostgreSQL user", DEFAULT_PG_PASS },
  { "pg-connect", '\0', POPT_ARG_STRING, &pg_conninfo, 'c', "PostgreSQL connection info string", "\"" DEFAULT_PG_CONNINFO "\""},
#endif
  { "user", '\0', POPT_ARG_STRING, &uidstr, 0, "Change server's user id", "UID" },
  { "group", '\0', POPT_ARG_STRING, &gidstr, 0, "Change server's group id", "GID" },
  { "event-hook", 'H', POPT_ARG_STRING, &hook, 0, "Path to an event hook taking input on stdin", "HOOK" },
  { "timeout", 't', POPT_ARG_INT, &socket_timeout, 0, "Timeout after which idle receiving sockets are cleaned up to avoid resource exhaustion", "60"  },
  { "debug-level", 'd', POPT_ARG_INT, &log_level, 0, "Increase debug level", "{1 .. 4}"  },
  { "logfile", '\0', POPT_ARG_STRING, &logfile_name, 0, "File to log to", DEFAULT_LOG_FILE },
  { "version", 'v', POPT_ARG_NONE, NULL, 'v', "Print version information and exit", NULL },
  { NULL, 0, 0, NULL, 0, NULL, NULL }
};

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
static void logging_setup (char *logfile, int level)
{
  if (!logfile) {
    if (isatty (fileno (stderr)))
      logfile = "-";
    else
      logfile = DEFAULT_LOG_FILE;
  }

  o_set_log_file(logfile);
  o_set_log_level(level);
  o_set_simplified_logging ();
}

/** Signal handler
 *
 * Captures the following signals, and handles them thusly.
 * * SIGTERM: instruct the EventLoop to stop.
 *
 * \see eventloop_stop()
 */
static void sighandler(int signum)
{
  switch(signum) {
  case SIGINT:
  case SIGTERM:
    loginfo("Received signal %d, cleaning up and exiting\n", signum);
    eventloop_stop(signum);
    break;
  case SIGUSR1:
    eventloop_report(O_LOG_INFO);
    break;
  default:
    logwarn("Received unhandled signal %d\n", signum);
  }
}

/** XXX: Type of a signal handler, as I'm not sure __sighandler_t from <signal.h> is portable */
typedef void (*sh_t) (int);
/** Actually install a new signal handler.
 *
 * \see sigaction(3)
 */
static void signal_install(sh_t handler)
{
  struct sigaction sa;

  sa.sa_handler = (void(*) (int))handler;
  sigemptyset (&sa.sa_mask);
  sa.sa_flags = 0;

  if(sigaction(SIGTERM, &sa, NULL))
    logwarn("Unable to install SIGTERM handler: %s\n", strerror(errno));
  if(sigaction(SIGINT, &sa, NULL))
    logwarn("Unable to install SIGINT handler: %s\n", strerror(errno));
  if(sigaction(SIGUSR1, &sa, NULL))
    logwarn("Unable to install SIGUSR1 handler: %s\n", strerror(errno));
}

/** Set up the signal handler.
 *
 * \see signal_install, sighandler, sigaction(3)
 */
static void signal_setup (void)
{
  logdebug("Installing signal handlers\n");
  signal_install(sighandler);
}

/** Clean up the signal handler.
 *
 * \see signal_install, sigaction(3)
 */
static void signal_cleanup (void)
{
  logdebug("Restoring default signal handlers\n");
  signal_install((sh_t)SIG_DFL);
}

static void drop_privileges (const char *uidstr, const char *gidstr)
{
  if (gidstr && !uidstr)
    die ("--gid supplied without --uid\n");

  if (uidstr) {
    struct passwd *passwd = getpwnam (uidstr);
    gid_t gid;

    if (!passwd)
      die ("User '%s' not found\n", uidstr);
    if (!gidstr)
      gid = passwd->pw_gid;
    else {
      struct group *group = getgrnam (gidstr);
      if (!group)
        die ("Group '%s' not found\n", gidstr);
      gid = group->gr_gid;
    }

    struct group *group = getgrgid (gid);
    const char *groupname = group ? group->gr_name : "??";
    gid_t grouplist[] = { gid };

    if (setgroups (1, grouplist) == -1)
      die ("Couldn't restrict group list to just group '%s': %s\n", groupname, strerror (errno));

    if (setgid (gid) == -1)
      die ("Could not set group id to '%s': %s", groupname, strerror (errno));

    if (setuid (passwd->pw_uid) == -1)
      die ("Could not set user id to '%s': %s", passwd->pw_name, strerror (errno));

    if (setuid (0) == 0)
      die ("Tried to drop privileges but we seem able to become superuser still!\n");
  }
}

/** Callback called when a new connection is received on the listening Socket.
 *
 * This function creates a ClientHandler to manage the data from this Socket.
 * The listening Socket would have been created using socket_server_new().
 *
 * \param new_sock Socket object created by accept()ing the connection
 * \param handle pointer to opaque data passed when creating the listening Socket
 *
 * \see socket_server_new()
 * \see on_client_connect()
 */
static void on_connect(Socket* new_sock, void* handle)
{
  (void)handle;
  ClientHandler *client = client_handler_new(new_sock);
  logdebug("%s: New client connected\n", client->name);
}

int main(int argc, char **argv)
{
  int c;
#ifdef HAVE_LIBPQ
  int i;
#endif

  poptContext optCon = poptGetContext(NULL, argc, (const char**) argv, options, 0);

  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {
    case 'v':
      printf(V_STRING, VERSION);
      printf("OML Protocol V%d--%d\n", MIN_PROTOCOL_VERSION, MAX_PROTOCOL_VERSION);
      printf(COPYRIGHT);
      return 0;
    }
  }

#ifdef HAVE_LIBPQ
  /* Cleanup command line to avoid showing credentials in ps(1) output, amongst
   * others.
   *
   * XXX: This is a poor man's security measure, as this creates a race
   * condition where, prior to doing the following, the credentials are still
   * visible to everybody.
   */
  if (pg_pass || pg_conninfo) {
    for(c=1; c<argc; c++) {
      i = -1;

      if(pg_pass && !strncmp(argv[c],"--pg-pass", 9))
        i = 9; /* Offset to look for '=' */

      if(pg_conninfo && !strncmp(argv[c],"--pg-connect", 12))
        i = 12;

      if (i >= 0) {
        if (argv[c][i++] != '=') {
          c++;
          i = 0;
        }
        if (c<argc)
          for (;i<strlen(argv[c]); i++)
            argv[c][i] = 'X';
      }
    }
  }
#endif /* HAVE_LIBPQ */

  logging_setup (logfile_name, log_level);

  if (c < -1)
    die ("%s: %s\n", poptBadOption (optCon, POPT_BADOPTION_NOALIAS), poptStrerror (c));

  loginfo(V_STRING, VERSION);
  loginfo("OML Protocol V%d--%d\n", MIN_PROTOCOL_VERSION, MAX_PROTOCOL_VERSION);
  loginfo(COPYRIGHT);

  eventloop_init();
  eventloop_set_socket_timeout(socket_timeout);

  Socket* server_sock;
  server_sock = socket_server_new("server", listen_port, on_connect, NULL);

  if (!server_sock)
    die ("Failed to create listening socket on port %d\n", listen_port);

  drop_privileges (uidstr, gidstr);

  /* Important that this comes after drop_privileges(). */
  if(database_setup_backend(dbbackend)){
      logerror("Failed to setup database backend '%s'\n", dbbackend);
      exit(EXIT_FAILURE);
  }

  signal_setup();

  hook_setup();

  eventloop_run();

  signal_cleanup();

  hook_cleanup();

  xmemreport(O_LOG_INFO);

  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
