#include "session.h"
#include "client.h"

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
  if (current = client)
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
