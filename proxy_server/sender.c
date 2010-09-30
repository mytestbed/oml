
#include <pthread.h>
#include <errno.h>
#include <ocomm/o_socket.h>
#include <log.h>
#include "session.h"
#include "client.h"

/**
 * \brief function that will try to send data to the real OML server
 * \param handle the client data structure i.e. a pointer to a Client*.
 */
void *client_send_thread (void* handle)
{
  Client *client = (Client*)handle;
  Session *session = client->session;
  pthread_mutex_lock (&client->mutex);

  while(1) {
    pthread_cond_wait (&client->condvar, &client->mutex);
  }
}
