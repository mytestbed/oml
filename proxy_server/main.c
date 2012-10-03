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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <popt.h>

#include <log.h>
#include <mstring.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "version.h"
#include "session.h"
#include "client.h"

#define DEF_PORT 3003
#define DEF_PORT_STR "3003"
#define DEFAULT_LOG_FILE "oml_proxy_server.log"
#define DEFAULT_RESULT_FILE "oml_result_proxy.res"
#define DEF_PAGE_SIZE 1024
#define DEFAULT_SERVER_ADDRESS "localhost"

static int listen_port = DEF_PORT;

static int log_level = O_LOG_INFO;
static char* logfile_name = NULL;
static char resultfile_name[128] = DEFAULT_RESULT_FILE;
static int page_size = DEF_PAGE_SIZE;
static int downstream_port = DEF_PORT;
static char* downstream_address = DEFAULT_SERVER_ADDRESS;
int sigpipe_flag = 0; // Set to 'true' by signal handler.

Session* session = NULL;
extern void *client_send_thread (void* handle);


struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen",      'l',  POPT_ARG_INT,    &listen_port,     0,   "Port to listen for TCP based clients", DEF_PORT_STR},
  { "debug-level", 'd',  POPT_ARG_INT,    &log_level,       0,   "Debug level - error:1 .. debug:4",     NULL},
  { "logfile",     '\0', POPT_ARG_STRING, &logfile_name,    0,   "File to log to",                       DEFAULT_LOG_FILE },
  { "version",     'v',  POPT_ARG_NONE,   NULL,             'v', "Print version information and exit",   NULL},
  { "resultfile",  'r',  POPT_ARG_STRING, &resultfile_name, 0,   "File name for storing received data",  DEFAULT_RESULT_FILE},
  { "size",        's',  POPT_ARG_INT,    &page_size,       0,   "Page size for buffering measurements", NULL},
  { "dstport",     'p',  POPT_ARG_INT,    &downstream_port, 0,   "Downstream OML server port",       NULL},
  { "dstaddress",  'a',  POPT_ARG_STRING, &downstream_address,  0,   "Downstream OML server address",    DEFAULT_SERVER_ADDRESS },
  { NULL,          0,    0,               NULL,             0,   NULL,                                   NULL }
};

void
proxy_message_loop (const char *client_id, Client *client, void *buf, size_t size);

/**
 * \brief function called when the socket receive some data
 * \param source the socket event
 * \param handle the cleint handler
 * \param buf data received from the socket
 * \param bufsize the size of the data set from the socket
 */
void
client_callback(SockEvtSource *source, void *handle, void *buf, int buf_size)
{
  (void)source;
  Client* self = (Client*)handle;

  //  logdebug ("'%s': received %d octets of data\n", source->name, buf_size);
  //  logdebug ("'%s': %s\n", source->name, to_octets (buf, buf_size));

  proxy_message_loop (source->name, self, buf, buf_size);

  mbuf_repack_message (self->mbuf);

  if (self->state == C_PROTOCOL_ERROR) {
    socket_close (self->recv_socket);
    logerror("'%s': protocol error, proxy server will disconnect upstream client\n", source->name);
    eventloop_socket_remove (self->recv_event);  // Note:  this free()'s source!
    self->recv_event = NULL;
  }

  fwrite (buf, sizeof (char), buf_size, self->file);

  pthread_mutex_lock (&self->mutex);
  pthread_cond_signal (&self->condvar);
  pthread_mutex_unlock (&self->mutex);
}

/**
 * \brief Call back function when the status of the socket change
 * \param source the socket event
 * \param status the status of the socket
 * \param error the value of the error if there is
 * \param handle the Client handler structure
 */
void
status_callback(SockEvtSource *source, SocketStatus status, int error, void *handle)
{
  (void)error;
  switch (status) {
    case SOCKET_CONN_CLOSED: {
      Client* self = (Client*)handle;
      if (self->recv_event != NULL) {
        socket_close (source->socket);
        logdebug("socket '%s' closed\n", source->name);
        eventloop_socket_remove (source); // Note:  this free()'s source!
      }

      /* Signal the sender thread for this client that the client disconnected */
      pthread_mutex_lock (&self->mutex);
      self->state = C_DISCONNECTED;
      pthread_cond_signal (&self->condvar);
      pthread_mutex_unlock (&self->mutex);
      break;
    default:
      break;
    }
  }
}

/**
 * \brief Called when a node connects via TCP
 * \param
 * \return
 */
void
on_connect (Socket* client_sock, void* handle)
{
  (void)handle;  // This parameter is unused
  MString *mstr = mstring_create ();

  mstring_sprintf (mstr,"%s.%d", resultfile_name, session->client_count);
  logdebug("New client (index %d) connected\n", session->client_count);
  session->client_count++;

  Client* client = client_new(client_sock, page_size, mstring_buf (mstr),
                              downstream_port, downstream_address);

  mstring_delete (mstr);

  session_add_client (session, client);
  client->session = session;

  client->recv_event = eventloop_on_read_in_channel(client_sock, client_callback,
                                                    status_callback, (void*)client);

  pthread_create (&client->thread, NULL, client_send_thread, (void*)client);
}

/*
 *  A hack to work around Mac OSX's broken implementation of poll(2),
 *  which does not allow polling stdin.  This hack dup()'s stdin to
 *  another file descriptor and creates a pipe(2) which it then
 *  dup2()'s into the normal stdin file descriptor.  If this succeeds,
 *  it then spawns a thread whose only function is to read(2) from the
 *  original stdin and write into the pipe.  The read end of the pipe,
 *  which is now file descriptor 0, is then useable in a call to
 *  poll(2).
 */
void*
prepare_stdin (void *handle)
{
  (void)handle;
#ifdef __APPLE__
  static int thread_started = 0;
  static int stdin_dup;
  static int stdin_pipe[2];
  static pthread_t self;
  if (!thread_started) {
    int result;
    result = pipe (stdin_pipe);
    if (result) {
      logerror ("Could not create pipe for stdin duplication: %s\n", strerror(errno));
      exit(EXIT_FAILURE);
    }

    stdin_dup = dup(fileno(stdin));
    dup2(stdin_pipe[0], fileno(stdin));

    thread_started = 1;
    result = pthread_create (&self, NULL, prepare_stdin, NULL);
    if (result == -1) {
      logerror ("Error creating thread for reading stdin: %s\n", strerror (errno));
      exit (EXIT_FAILURE);
    }
  } else {
    char buf [512];
    while (1) {
      ssize_t count = read (stdin_dup, buf, 512);
      if (count < 0)
        logerror ("Error reading from stdin: %s\n", strerror(errno));
      if (count <= 0)
        return NULL;
      write (stdin_pipe[1], buf, count);
    }
  }
#endif
  return NULL;
}

void
stdin_handler(SockEvtSource* source, void* handle, void* buf, int buf_size)
{
  Session *proxy = (Session*)handle;
  char command[80];
  (void)source;
  strncpy (command, buf, 80);

  if (buf_size < 80 && command[buf_size-1] == '\n')
    command[buf_size-1] = '\0';

  printf ("Received command: %s\n", command);
  logdebug ("Received command: %s\n", command);

  if ((strcmp (command, "OMLPROXY-RESUME") == 0) ||
      (strcmp (command, "RESUME") == 0)) {
    proxy->state = ProxyState_SENDING;
  } else if (strcmp (command, "OMLPROXY-STOP") == 0 ||
             strcmp (command, "STOP") == 0) {
    proxy->state = ProxyState_STOPPED;
  } else if (strcmp (command, "OMLPROXY-PAUSE") == 0 ||
             strcmp (command, "PAUSE") == 0) {
    proxy->state = ProxyState_PAUSED;
  }

  /*
   * If we're in sending state, wake up the client sender threads
   * so that they will start sending to the downstream server.  We do
   * this even if the state was already ProxyState_SENDING because
   * some of the clients might have dropped back to idle due to
   * disconnection from the upstream server.
   */
  if (session->state == ProxyState_SENDING) {
    Client *current = session->clients;
    while (current) {
      pthread_mutex_lock (&current->mutex);
      pthread_cond_signal (&current->condvar);
      pthread_mutex_unlock (&current->mutex);
      current = current->next;
    }
  }
}

void
on_control_connect (Socket *sock, void *handle)
{
  (void)handle; // This parameter is unused
  logdebug ("New control connection\n");
  eventloop_on_read_in_channel (sock, stdin_handler, NULL, (void*)session);
}

void
sigpipe_handler(int signum)
{
  if (signum == SIGPIPE)
    sigpipe_flag = 1;
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

int
main(int argc, const char *argv[])
{
  Socket* serverSock, *controlSock;
  int c;

  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp(optCon, "configFile");

  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {
    case 'v':
      printf(V_STRING, VERSION);
      printf(COPYRIGHT);
      return 0;
    }
  }

  setup_logging (logfile_name, log_level);

  if (c < -1) {
    /* an error occurred during option processing */
    fprintf(stderr, "%s: %s\n",
      poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
      poptStrerror(c));
    return -1;
  }

  loginfo (V_STRING, VERSION);
  loginfo (COPYRIGHT);

  struct sigaction new_action, old_action;
  new_action.sa_handler = sigpipe_handler;
  sigemptyset (&new_action.sa_mask);
  new_action.sa_flags = 0;
  sigaction (SIGPIPE, NULL, &old_action);
  if (old_action.sa_handler != SIG_IGN)
    sigaction (SIGPIPE, &new_action, NULL);

  prepare_stdin(NULL);

  eventloop_init();
  session = (Session*) malloc(sizeof(Session));
  memset(session, 0, sizeof(Session));

  session->state = ProxyState_PAUSED;

  serverSock = socket_server_new("proxy_server", listen_port, on_connect, NULL);
  controlSock = socket_server_new("proxy_server_control", listen_port + 1, on_control_connect, NULL);

  eventloop_on_stdin(stdin_handler, session);
  eventloop_run();

  socket_free(serverSock);
  socket_free(controlSock);

  return(0);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
