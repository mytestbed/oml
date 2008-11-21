/*
 * Copyright 2007-2008 National ICT Australia (NICTA), Australia
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
/*!\file main.c
  \brief This file is the starting point.
*/

#include <popt.h>
#include <stdio.h>

#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"

// Network information
#define DEFAULT_PORT 9008

 //************************
static int port = DEFAULT_PORT;

#define DEFAULT_LOG_FILE "server.log"
static int log_level = O_LOG_INFO;
static char* logfile_name = DEFAULT_LOG_FILE;
static FILE* logfile;

//extern int o_log_level;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "port", 'p', POPT_ARG_INT, &port, 0,
        "Port to listen on"   },

  { "debug-level", 'd', POPT_ARG_INT, &log_level, 0,
        "Debug level - error:1 .. debug:4"  },
  { "logfile", 'l', POPT_ARG_STRING, &logfile_name, 0,
        "File to log to", DEFAULT_LOG_FILE },
  { NULL, 0, 0, NULL, 0 }
};

/********************************************/

void
on_client(
  SockEvtSource * client,
  void* handle,
  void* buf,
  int buf_size
) {
	char b[512];
	char* reply = b;
	int len;

	sprintf(reply, "ECHO: %s", buf);
	len = strlen(reply);

  o_log(O_LOG_DEBUG, "sending reply(%d): <%s>\n", len, reply);
  socket_sendto(client->socket, reply, len);
}


void
on_status(
  SockEvtSource * sock,
  SocketStatus status,
  int errno,
  void* handle
) {
  switch (status) {
  case SOCKET_CONN_CLOSED:
    o_log(O_LOG_INFO, "Connection '%s' closed\n", sock->name);
    socket_close(sock->socket);
    break;
  default:
    o_log(O_LOG_INFO, "Unknown status '%d' for socket '%s'.\n", status, sock->name);
  }
}

void
on_connect(
  Socket* newSock,
  void* handle
) {
  o_log(O_LOG_INFO, "New client connected\n");
  eventloop_on_read_in_channel(newSock, &on_client, &on_status, NULL);
}

void
process_args(
  int argc,
  const char *argv[]
) {
  char c;

  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp(optCon, "configFile");

  while ((c = poptGetNextOpt(optCon)) >= 0);
  o_set_log_file(logfile_name);
  o_set_log_level(log_level);
  setlinebuf(stdout);

  if (c < -1) {
    /* an error occurred during option processing */
    fprintf(stderr, "%s: %s\n",
	    poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
	    poptStrerror(c));
    exit(-1);
  }
}

int
main(
  int argc,
  const char *argv[]
) {
	process_args(argc, argv);

  eventloop_init(10);

  Socket* serverSock = socket_server_new("server", port, on_connect, NULL);

  eventloop_run();
  return(0);
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
