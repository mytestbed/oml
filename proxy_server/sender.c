
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

    if (session->state == ProxyState_SENDING) {
      while (client->send_buffer &&
             client->send_buffer != client->recv_buffer) {
        ClientBuffer* buffer = client->send_buffer;

        int bytes_to_send = buffer->current_size - buffer->bytes_already_sent;

        if (bytes_to_send > 0) {
          /* Don't block other threads while sending data */
          pthread_mutex_unlock (&client->mutex);
          int result = socket_sendto (client->send_socket, (char*)buffer->buff_to_send,
                                      bytes_to_send);
          pthread_mutex_lock (&client->mutex);
          if (result == 0) {
            buffer->buff_to_send += bytes_to_send;
            buffer->bytes_already_sent = buffer->current_size;
            client->bytes_sent += bytes_to_send;
          } else {
            logerror ("Error sending %d bytes to downstream server:  %s\n",
                      bytes_to_send, strerror (errno));
          }
        }
        client->send_buffer = client->send_buffer->next;
      }

      if (client->recv_socket_closed == 1) {
        /* Client disconnected */
        logdebug("Closing downstream server connection '%s'\n", client->send_socket->name);
        socket_close (client->send_socket);
        client->send_socket_closed = 1;
        session_remove_client (session, client);
        client_free (client);
        pthread_exit (NULL);
      }
    } else {
      logdebug ("Client sender thread woke up when not in state SENDING (actual state %d)\n",
                session->state);
    }
  }
}
