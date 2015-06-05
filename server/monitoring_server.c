/*
 * Copyright 2013-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file monitoring-server.c
 * \brief Functions for taking measurements about this OML server.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "oml2/omlc.h"

#define OML_FROM_MAIN
#include "oml2-server_oml.h"

static int oml_enabled = 0;

/** Set up connection to a monitoring OML server.
 *
 * \param argc non-NULL pointer to the argument count
 * \param argv argument vector
 */
void
oml_setup(int *argc, const char **argv)
{
  int result;
  /* connect to the monitoring server */
  result = omlc_init("server", argc, argv, NULL);
  if(0 == result) {
    logdebug("Initialised OML client library\n");

    oml_register_mps();
    if(omlc_start() == 0) {
      logdebug("Started OML reporting of server's internal metrics\n");
      oml_enabled = 1;

    } else {
      logwarn("Could not start OML client library; this does not impact the server's collection capabilities\n");
      omlc_close();
    }

  } else if (result == 1) {
    logdebug ("OML was disabled by the user\n");

  } else {
    logwarn("Could not initialise OML client library; this does not impact the server's collection capabilities\n");
  }
}

/** Clean up connection to the monitoring OML server. */
void
oml_cleanup(void)
{
  if(oml_enabled) {
    omlc_close();
    oml_enabled = 0;
  }
}

/** Inject a client report into the monitoring OML server.
 *
 * \param address pointer to the client IP address string
 * \param port    client port number
 * \param oml_id  pointer to a string containing the client OML ID.
 * \param domain  pointer to a string containing the client domain.
 * \param appname pointer to a string containing the client appname.
 * \param event   (possibly NULL) pointer to an event string
 * \param message (possibly NULL) pointer to a message string
 */
void
client_event_inject(const char* address, uint32_t port, const char* oml_id, const char* domain, const char* appname, const char* event, const char* message)
{
  if(oml_enabled) {
    oml_inject_clients(g_oml_mps_oml2_server->clients, address, port, oml_id, domain, appname, event, message);
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
