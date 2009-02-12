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

#include <popt.h>
/*
#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/errno.h>
#include <assert.h>
*/

#include <ocomm/o_log.h>
#include <ocomm/o_socket.h>
#include <ocomm/o_eventloop.h>

#include "version.h"
#include "marshall.h"

#define DEF_PORT 3003
#define DEF_PORT_STR "3003"
#define DEFAULT_LOG_FILE "oml_proxy_server.log"
#define DEFAULT_RESULT_FILE "oml_result_proxy.res"
#define DEF_PAGE_SIZE 512

static int listen_port = DEF_PORT;

static int log_level = O_LOG_INFO;
static char* logfile_name = DEFAULT_LOG_FILE;
static char* resultfile_name = DEFAULT_RESULT_FILE;
static int page_size = DEF_PAGE_SIZE;


struct poptOption options[] = {
  POPT_AUTOHELP
  { "listen", 'l', POPT_ARG_INT, &listen_port, 0,
        "Port to listen for TCP based clients", DEF_PORT_STR},
  { "debug-level", 'd', POPT_ARG_INT, &log_level, 0,
        "Debug level - error:1 .. debug:4"  },
  { "logfile", '\0', POPT_ARG_STRING, &logfile_name, 0,
        "File to log to", DEFAULT_LOG_FILE },
  { "version", 'v', 0, 0, 'v', "Print version information and exit" },
  { "resultfile", 'r', POPT_ARG_STRING, &resultfile_name, 0,
        "File to put the result", DEFAULT_RESULT_FILE},
  { "size", 's', POPT_ARG_INT, &page_size, 0,
        "Size of the bufferised data",  POPT_ARG_INT},
  { NULL, 0, 0, NULL, 0 }
};

/**
 * \fn
 * \brief
 * \param
 * \return
 */
//! Called when a node connects via TCP
void
on_connect(
  Socket* newSock,
  void* handle
) {
//  Socket* outSock = (Socket*)handle;

  o_log(O_LOG_DEBUG, "New client connected\n");
  proxy_client_handler_new(newSock, page_size, resultfile_name);
}

/**
 * \fn
 * \brief
 * \param
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
  o_log(O_LOG_INFO, COPYRIGHT);

  eventloop_init(100);

  Socket* serverSock;
  serverSock = socket_server_new("proxy_server", listen_port, on_connect, NULL);

  eventloop_run();
  return(0);
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
