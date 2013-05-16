/*
 * Copyright 2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

/** \file monitoring-server.c
 * \brief functions for taking measurements about this OML server.
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


/** The OML server measurement point.
 */
static OmlMP *stats = NULL;

void
ms_setup(int *argc, const char **argv)
{
  /* connect to the monitoring server */
  if(omlc_init("server", argc, argv, NULL) == 0) {
    loginfo("Initialized OML client lib\n");
    if(omlc_start() == 0) {
      loginfo("Started OML client\n");
      static OmlMPDef oml_server_basic_def[] = {
        {"address", OML_STRING_VALUE},
        {"port", OML_UINT32_VALUE},
        {"oml_id", OML_STRING_VALUE},
        {"domain", OML_STRING_VALUE},
        {"appname", OML_STRING_VALUE},
        {"timestamp", OML_UINT64_VALUE},
        {"event", OML_STRING_VALUE},
        {"message", OML_STRING_VALUE},
        {NULL, OML_UNKNOWN_VALUE}
      };
      stats = omlc_add_mp("omlserver", oml_server_basic_def);
    } else {
      logwarn("Could not start OML client\n");    
    }
  } else {
    logwarn("Could not initialise link to OML monitor\n");
  }
}

void
ms_cleanup(void)
{
  if(stats) {
    omlc_close();
    stats = NULL;
  }
}

void
ms_inject(const char* address, uint32_t port, const char* oml_id, const char* domain, const char* appname, uint64_t timestamp, const char* event, const char* message)
{
  if(stats) {
    OmlValueU v[8];
    omlc_zero_array(v, 8);
    omlc_set_string(v[0], address);
    omlc_set_uint32(v[1], port);
    omlc_set_string(v[2], oml_id);
    omlc_set_string(v[3], domain);
    omlc_set_string(v[4], appname);
    omlc_set_uint64(v[5], timestamp);
    omlc_set_string(v[6], event);
    omlc_set_string(v[7], message);

    omlc_inject(stats, v);

    omlc_reset_string(v[0]);
    omlc_reset_string(v[2]);
    omlc_reset_string(v[3]);
    omlc_reset_string(v[4]);
    omlc_reset_string(v[6]);
    omlc_reset_string(v[7]);
  }
}
