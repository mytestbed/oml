/*
 * Copyright 2010-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file session.c
 * \brief Session management functions.
 */
#include "session.h"
#include "proxy_client.h"

void
session_add_client (Session *session, Client *client)
{
  if (client == NULL || session == NULL) return;
  client->next = session->clients;
  session->clients = client;
  session->client_count++;
}

void
session_remove_client (Session *session, Client *client)
{
  if (client == NULL || session == NULL) return;
  Client *current = session->clients;
  if (current == client)
    session->clients = current->next;
  else
    while (current) {
      if (current->next == client) {
        current->next = current->next->next;
        break;
      }
      current = current->next;
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
