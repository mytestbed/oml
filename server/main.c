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

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <popt.h>

#include <oml2/oml_writer.h>
#include <log.h>
#include <mem.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "util.h"
#include "version.h"
#include "hook.h"
#include "client_handler.h"
#include "sqlite_adapter.h"
#if HAVE_PG
#include "psql_adapter.h"
#include <libpq-fe.h>
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
#define DEFAULT_PG_CONNINFO "host=localhost"
#define DEFAULT_PG_USER "oml"
#define DEFAULT_DB_BACKEND "sqlite"

static int listen_port = DEFAULT_PORT;
char *sqlite_database_dir = NULL;
char *pg_conninfo = DEFAULT_PG_CONNINFO;
char *pg_user = DEFAULT_PG_USER;

static int log_level = O_LOG_INFO;
static char* logfile_name = NULL;
static char* backend = DEFAULT_DB_BACKEND;
static char* uidstr = NULL;
static char* gidstr = NULL;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen", 'l', POPT_ARG_INT, &listen_port, 0, "Port to listen for TCP based clients", DEFAULT_PORT_STR},
  { "backend", 'b', POPT_ARG_STRING, &backend, 0, "Database server backend", DEFAULT_DB_BACKEND},
#if HAVE_PG
  { "pg-user", '\0', POPT_ARG_STRING, &pg_user, 0, "PostgreSQL user to connect as", DEFAULT_PG_USER },
  { "pg-connect", '\0', POPT_ARG_STRING, &pg_conninfo, 0, "PostgreSQL connection info string", "\"" DEFAULT_PG_CONNINFO "\""},
#endif
  { "data-dir", 'D', POPT_ARG_STRING, &sqlite_database_dir, 0, "Directory to store database files (sqlite)", "DIR" },
  { "user", '\0', POPT_ARG_STRING, &uidstr, 0, "Change server's user id", "UID" },
  { "group", '\0', POPT_ARG_STRING, &gidstr, 0, "Change server's group id", "GID" },
  { "event-hook", 'H', POPT_ARG_STRING, &hook, 0, "Path to an event hook taking input on stdin", "HOOK" },
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
 * @brief Work out which directory to put sqlite databases in, and set
 * sqlite_database_dir to that directory.
 *
 * This works as follows: if the user specified --data-dir on the
 * command line, we use that value.  Otherwise, if OML_SQLITE_DIR
 * environment variable is set, use that dir.  Otherwise, use
 * PKG_LOCAL_STATE_DIR, which is a preprocessor macro set by the build
 * system (under Autotools defaults this should be
 * ${prefix}/var/oml2-server, but on a distro it should be something
 * like /var/lib/oml2-server).
 *
 */
void
setup_sqlite_database_dir (void)
{
  if (!sqlite_database_dir) {
    const char *oml_sqlite_dir = getenv ("OML_SQLITE_DIR");
    if (oml_sqlite_dir) {
      sqlite_database_dir = xstrndup (oml_sqlite_dir, strlen (oml_sqlite_dir));
    } else {
      sqlite_database_dir = PKG_LOCAL_STATE_DIR;
    }
  }
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
  o_set_simplified_logging ();
}

void
setup_backend_sqlite (void)
{
  setup_sqlite_database_dir ();

  /*
   * The man page says access(2) should be avoided because it creates
   * a race condition between calls to access(2) and open(2) where an
   * attacker could swap the underlying file for a link to a file that
   * the unprivileged user does not have permissions to access, which
   * the effective user does.  We don't use the access/open sequence
   * here. We check that we can create files in the
   * sqlite_database_dir directory, and fail if not (as the server
   * won't be able to do useful work otherwise).
   *
   * An attacker could potentially change the underlying file to a
   * link and still cause problems if oml2-server is run as root or
   * setuid root, but this check must happen after drop_privileges()
   * is called, so if oml2-server is not run with superuser privileges
   * such an attack would still fail.
   *
   * oml2-server should not be run as root or setuid root in any case.
   */
  if (access (sqlite_database_dir, R_OK | W_OK | X_OK) == -1)
    die ("Can't access SQLite database directory %s: %s\n",
         sqlite_database_dir, strerror (errno));

  loginfo ("Creating SQLite3 databases in %s\n", sqlite_database_dir);
}

void
setup_backend_postgresql (const char *conninfo, const char *user)
{
#if HAVE_PG
  loginfo ("Sending experiment data to PostgreSQL server with user '%s'\n",
           pg_user);
  MString *str = mstring_create ();
  mstring_sprintf (str, "%s user=%s dbname=postgres", conninfo, user);
  PGconn *conn = PQconnectdb (mstring_buf (str));

  if (PQstatus (conn) != CONNECTION_OK)
    die ("Could not connect to PostgreSQL database (conninfo \"%s\"): %s\n",
         conninfo, PQerrorMessage (conn));

  /* oml2-server must be able to create new databases, so check that
     our user has the required role attributes */
  mstring_set (str, "");
  mstring_sprintf (str, "SELECT rolcreatedb FROM pg_roles WHERE rolname='%s'", user);
  PGresult *res = PQexec (conn, mstring_buf (str));
  if (PQresultStatus (res) != PGRES_TUPLES_OK)
    die ("Failed to determine role privileges for role '%s': %s\n",
         user, PQerrorMessage (conn));
  char *has_create = PQgetvalue (res, 0, 0);
  if (strcmp (has_create, "t") == 0)
    logdebug ("User '%s' has CREATE DATABASE privileges\n", user);
  else
    die ("User '%s' does not have required role CREATE DATABASE\n", user);

  PQclear (res);
  PQfinish (conn);
#else
  (void)conninfo;
#endif
}

void
setup_backend (void)
{
  if (!database_create_function ())
    die ("Unknown database backend '%s' (valid backends: %s)\n",
         backend, valid_backends ());

  loginfo ("Database backend: '%s'\n", backend);

  const char *pg = "postgresql";
  const char *sq = "sqlite";
  if (!strcmp (backend, pg))
    loginfo ("PostgreSQL backend is still experimental!\n");
  if (!strcmp (backend, pg))
    setup_backend_postgresql (pg_conninfo, pg_user);
  if (!strcmp (backend, sq))
    setup_backend_sqlite ();
}


void
drop_privileges (const char *uidstr, const char *gidstr)
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

/**
 * \brief Called when a node connects via TCP
 * \param new_sock
 */
void
on_connect(Socket* new_sock, void* handle)
{
  (void)handle;
  ClientHandler *client = client_handler_new(new_sock);
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

  eventloop_init();

  Socket* server_sock;
  server_sock = socket_server_new("server", listen_port, on_connect, NULL);

  if (!server_sock)
    die ("Failed to create socket (port %d) to listen for client connections.\n", listen_port);

  drop_privileges (uidstr, gidstr);

  /* Important that this comes after drop_privileges().  See setup_backend_sqlite() */
  setup_backend ();

  hook_setup();

  eventloop_run();

  hook_cleanup();

  xmemreport();

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
