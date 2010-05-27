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
#include <config.h>
#include <popt.h>
#include <string.h>

#include <log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "version.h"
#include "proxy_server.h"
#include "proxy_client_handler.h"
#define DEF_PORT 3003
#define DEF_PORT_STR "3003"
#define DEFAULT_LOG_FILE "oml_proxy_server.log"
#define DEFAULT_RESULT_FILE "oml_result_proxy.res"
#define DEF_PAGE_SIZE 512
#define DEFAULT_SERVER_ADDRESS "localhost"

static int listen_port = DEF_PORT;

static int log_level = O_LOG_INFO;
static char* logfile_name = DEFAULT_LOG_FILE;
static char resultfile_name[128] = DEFAULT_RESULT_FILE;
static int page_size = DEF_PAGE_SIZE;
static int dstport = DEF_PORT;
static char* address_server = DEFAULT_SERVER_ADDRESS;

int numberSocket = 0;

ProxyServer* proxyServer = NULL;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen",      'l',  POPT_ARG_INT,    &listen_port,     0,   "Port to listen for TCP based clients", DEF_PORT_STR},
  { "debug-level", 'd',  POPT_ARG_INT,    &log_level,       0,   "Debug level - error:1 .. debug:4",     NULL},
  { "logfile",     '\0', POPT_ARG_STRING, &logfile_name,    0,   "File to log to",                       DEFAULT_LOG_FILE },
  { "version",     'v',  POPT_ARG_NONE,   NULL,             'v', "Print version information and exit",   NULL},
  { "resultfile",  'r',  POPT_ARG_STRING, &resultfile_name, 0,   "File to put the result",               DEFAULT_RESULT_FILE},
  { "size",        's',  POPT_ARG_INT,    &page_size,       0,   "Size of the bufferised data",          NULL},
  { "dstport",     'p',  POPT_ARG_INT,    &dstport,         0,   "Destination port of the server",       NULL},
  { "dstaddress",  'a',  POPT_ARG_STRING, &address_server,  0,   "Address of the OML Server",            DEF_PORT_STR },
  { NULL,          0,    0,               NULL,             0,   NULL,                                   NULL }
};

/**
 * \fn void on_connect(Socket* newSock, void* handle)
 * \brief Called when a node connects via TCP
 * \param
 * \return
 */
void
on_connect(
  Socket* newSock,
  void* handle
) {
  (void)handle;  // FIXME:  Why is this parameter unused?
//  Socket* outSock = (Socket*)handle;
  char integer_string[32];
  sprintf(integer_string, "%d", numberSocket);
  strcat(resultfile_name, integer_string);
  logdebug("New client connected\n");
  numberSocket++;
  ProxyClientHandler* proxy = proxy_client_handler_new(newSock, page_size, resultfile_name, dstport, address_server);

  proxy->next = proxyServer->current;
  proxyServer->current = proxy;


  startLoopChannel( newSock,  proxy);
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
}

void
stdin_handler(SockEvtSource* source, void* handle, void* buf, int buf_size)
{
  ProxyServer *proxy = (ProxyServer*)handle;
  char command[80];
  strncpy (command, buf, 80);

  if (buf_size < 80 && command[buf_size-1] == '\n')
    command[buf_size-1] = '\0';

  printf ("Received command: %s\n", command);

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
}


/**
 * \fn int main(int argc, const char *argv[])
 * \brief Start the proxy server
 * \param
 * \return
 */
int
main(int argc, const char *argv[])
{
  char c;

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
  o_set_log_file(logfile_name);
  o_set_log_level(log_level);

  if (c < -1) {
    /* an error occurred during option processing */
    fprintf(stderr, "%s: %s\n",
      poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
      poptStrerror(c));
    return -1;
  }

  loginfo (V_STRING, VERSION);
  loginfo (COPYRIGHT);

  prepare_stdin(NULL);

  eventloop_init();
   proxyServer = (ProxyServer*) malloc(sizeof(ProxyServer));
  memset(proxyServer, 0, sizeof(ProxyServer));

  proxyServer->state = ProxyState_PAUSED;

  Socket* serverSock;
  serverSock = socket_server_new("proxy_server", listen_port, on_connect, NULL);

  eventloop_on_stdin(stdin_handler, proxyServer);
  eventloop_run();

  return(0);
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
