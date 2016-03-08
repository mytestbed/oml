/*
 * Copyright 2013-2016 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
#ifndef CHECK_SERVER_H
#define CHECK_SERVER_H

#include "ocomm/o_eventloop.h"
#include "client_handler.h"

ClientHandler* check_server_prepare_client_handler(const char* name, SockEvtSource *source);
void check_server_destroy_client_handler(ClientHandler* ch);

#endif /* CHECK_SERVER_H */
