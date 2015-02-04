/*
 * Copyright 2012-2015 National ICT Australia (NICTA), Australia
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
/** \privatesection \file check_server.c
 * Test the features of the server
 */

#include <stdlib.h>
#include <check.h>

#include "ocomm/o_log.h"
#include "ocomm/o_eventloop.h"
#include "mem.h"
#include "mbuf.h"
#include "client_handler.h"
#include "check_server.h"
#include "check_server_suites.h"

/** Create a fake test ClientHandler almost as server/client_handler.h:client_handler_new() would.
 *
 * \param name identifier for this ClientHandler
 * \return a working ClientHandler (or fail the test)
 *
 * \see client_handler_new
 */
ClientHandler*
check_server_prepare_client_handler(const char* name, SockEvtSource *source) {
  ClientHandler* ch;

  ch = (ClientHandler*) oml_malloc(sizeof(ClientHandler));
  fail_if(ch == NULL, "Problem allocating ClientHandler");

  ch->state = CS_HEADER;
  ch->content = CM_UNSPEC_DATA;
  ch->mbuf = mbuf_create ();
  ch->socket = NULL;
  ch->event = source;
  strncpy (ch->name, name, MAX_STRING_SIZE);

  return ch;
}

/** Free the fake test ClientHandler.
 *
 * \param ch ClientHandler
 *
 * \see client_handler_new
 */
void
check_server_destroy_client_handler(ClientHandler* ch)
{
  mbuf_destroy(ch->mbuf);
  oml_free(ch);
}

int
main (void)
{
  int number_failed = 0;

  o_set_log_file ("check_server.oml.log");
  SRunner *sr = srunner_create (text_protocol_suite ());
  srunner_add_suite (sr, binary_protocol_suite ());
  //  srunner_add_suite (sr, database_suite ()); /* For example ... */

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
