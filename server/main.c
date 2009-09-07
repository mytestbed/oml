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
/*!\file main.c
  \brief This file is the starting point.
*/


#include <stdio.h>
#include <popt.h>

#include <oml2/oml_writer.h>
#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "version.h"
#include "server.h"

#define DEF_PORT 3003
#define DEF_PORT_STR "3003"
#define DEFAULT_LOG_FILE "oml_server.log"

static int listen_port = DEF_PORT;
char* g_database_data_dir = ".";

static int log_level = O_LOG_INFO;
static char* logfile_name = DEFAULT_LOG_FILE;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen", 'l', POPT_ARG_INT, &listen_port, 0,
        "Port to listen for TCP based clients", DEF_PORT_STR},

  { "data-dir", '\0', POPT_ARG_STRING, &g_database_data_dir, 0,
        "Directory to store dtabase files" },
  { "debug-level", 'd', POPT_ARG_INT, &log_level, 0,
        "Debug level - error:1 .. debug:4"  },
  { "logfile", '\0', POPT_ARG_STRING, &logfile_name, 0,
        "File to log to", DEFAULT_LOG_FILE },
  { "version", 'v', 0, 0, 'v', "Print version information and exit" },
  { NULL, 0, 0, NULL, 0 }
};
/**
 * \fn on_connect(Socket* newSock, void* handle)
 * \brief Called when a node connects via TCP
 * \param newSock
 */
void
on_connect(
  Socket* newSock,
  void* handle
) {
  (void)handle; // FIXME:  why is this not used?
//  Socket* outSock = (Socket*)handle;

  o_log(O_LOG_DEBUG, "New client connected\n");
  client_handler_new(newSock);
}

/**
 * \fn int main(int argc, const char *argv[])
 * \brief
 * \param argc
 * \param argv
 * \return
 */
int
main(
  int argc,
  const char *argv[]
) {


  char c;

  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  poptSetOtherOptionHelp(optCon, "configFile");

  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {
    case 'v':
      printf(V_STRING, VERSION);
	  printf("OML Protocol V%d\n", OML_PROTOCOL_VERSION);
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

  o_log(O_LOG_INFO, V_STRING, VERSION);
  o_log(O_LOG_INFO, "OML Protocol V%d\n", OML_PROTOCOL_VERSION);
  o_log(O_LOG_INFO, COPYRIGHT);

  eventloop_init();

  Socket* serverSock;
  serverSock = socket_server_new("server", listen_port, on_connect, NULL);

  if (serverSock)
	eventloop_run();
  else {
	o_log (O_LOG_ERROR, "SERVER QUIT:  failed to create socket for client connections.\n");
	return -2;
  }

  return(0);
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
