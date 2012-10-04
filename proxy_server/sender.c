/*
 * Copyright 2010-2012 National ICT Australia (NICTA), Australia
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
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>

//#include <ocomm/o_socket.h>
#include <log.h>
#include <mem.h>
#include "session.h"
#include "client.h"

extern int sigpipe_flag;

int
client_sender_connect (Client *client)
{
  int result = 0;
  struct addrinfo hints;
  struct addrinfo *servinfo;
  char port[6];

  if (client->downstream_port > 65535)
    return -1;

  snprintf (port, 5, "%d", client->downstream_port);

  bzero (&hints, sizeof (hints));
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;

  result = getaddrinfo (client->downstream_addr, port, &hints, &servinfo);
  if (result != 0) {
    logerror ("Could not resolve downstream host %s:%s -- %s\n", client->downstream_addr, port,
              strerror (errno));
    return -1;
  }

  /* Take the first address returned */
  int s = socket (servinfo->ai_family, servinfo->ai_socktype, servinfo->ai_protocol);
  if (s == -1) {
    logerror ("Could not create socket for downstream host %s:%s -- %s\n", client->downstream_addr, port,
              strerror (errno));
    freeaddrinfo (servinfo);
    return -1;
  }

  client->send_socket = s;
  result = connect (client->send_socket, servinfo->ai_addr, servinfo->ai_addrlen);
  if (result == -1) {
    logerror ("Could not connect to downstream server: %s:%s -- %s\n", client->downstream_addr, port,
              strerror (errno));
    freeaddrinfo (servinfo);
    return -1;
  }

  freeaddrinfo (servinfo);
  return 0;
}

int
client_send_header (Client *client, struct header *header)
{
  const char *tagstr = tag_to_string (header->tag);
  char *value = header->value;
  char *buf = xmalloc (strlen (tagstr) + strlen (value) + 4);
  if (!buf)
    return -1;

  strcpy (buf, tagstr);
  strcat (buf, ": ");
  strcat (buf, value);
  strcat (buf, "\n");

  int count = strlen (buf);

  do {
    logdebug ("Sending %d bytes of header\n", count);
    int result = send (client->send_socket, buf, count, 0);
    if (result == -1) {
      xfree (buf);
      return -1;
    }
    count -= result;
  } while (count > 0);
  xfree (buf);

  return 0;
}

int
client_send_headers (Client *client)
{
  enum HeaderTag header_tags [] = {
    H_PROTOCOL,
    H_DOMAIN,
    H_START_TIME,
    H_SENDER_ID,
    H_APP_NAME,
  };
  unsigned int i = 0;

  for (i = 0; i < sizeof (header_tags) / sizeof (header_tags[0]); i++) {
    if (client_send_header (client, client->header_table[header_tags[i]]) == -1)
      return -1;
  }

  struct header *header = client->headers;
  while (header) {
    if (header->tag == H_SCHEMA) {
      if (client_send_header (client, header) == -1)
        return -1;
    }
    header = header->next;
  }

  if (client_send_header (client, client->header_table[H_CONTENT]) == -1)
    return -1;

  if (send (client->send_socket, "\n", 1, 0) == -1)
    return -1;

  return 0;
}

/**
 * \brief function that will try to send data to the real OML server
 * \param handle the client data structure i.e. a pointer to a Client*.
 */
void *client_send_thread (void* handle)
{
  Client *client = (Client*)handle;
  Session *session = client->session;

  loginfo ("'%s': client sender thread started\n", client->recv_socket->name);
  pthread_mutex_lock (&client->mutex);

  logdebug ("Mutex locked...\n");

  while(1) {
    logdebug ("Sender waiting...\n");
    pthread_cond_wait (&client->condvar, &client->mutex);

    if (session->state == ProxyState_SENDING) {
      if (!client->sender_connected) {
        // try to connect
        if (client->state == C_HEADER || client->state == C_CONFIGURE)
          continue; // Haven't finished receiving the headers


        pthread_mutex_unlock (&client->mutex);
        while (!client->sender_connected && session->state == ProxyState_SENDING) {
          if (client_sender_connect (client) == 0) {
            logdebug ("Connected to downstream OK\n");
            client->sender_connected = 1;
          } else {
            logdebug ("Failed to connect to downstream:  %s\n", strerror (errno));
            sleep (1); // Retry every second.
            continue;
          }
        }
        pthread_mutex_lock (&client->mutex);
        if (client->sender_connected && client_send_headers (client) == -1) {
          logdebug ("Something failed while sending headers:  %s\n", strerror (errno));
          client->sender_connected = 0;
          continue;
        }
      }

      while (client->messages->length > 0 && client->sender_connected) {
        struct msg_queue_node *head = msg_queue_head (client->messages);
        struct cbuffer_cursor cursor = head->cursor;
        size_t length = head->msg->length;
        size_t count = 0;
        int result = 0;

        while (length > 0) {
          char *buf = cbuf_cursor_pointer (&cursor);
          size_t page_remaining = cbuf_cursor_page_remaining (&cursor);
          size_t to_write = length < page_remaining ? length : page_remaining;

          logdebug ("Seqno: %d Sending %d bytes (fill=%d, remain=%d)\n", head->msg->seqno, to_write, cursor.page->fill, page_remaining);
          pthread_mutex_unlock (&client->mutex);
          result = send (client->send_socket, buf, to_write, 0);
          pthread_mutex_lock (&client->mutex);

          if (result == -1) {
            logdebug ("send(2) returned -1 (%s)\n", strerror (errno));
            if (sigpipe_flag == 1) {
              logdebug ("sigpipe_flag is 1\n");
              sigpipe_flag = 0;
            }
            client->sender_connected = 0;
            break;
          } else if (result < (int)to_write) {
            logdebug ("Wrote less than expected \n");
          }
          cbuf_advance_cursor (&cursor, result);
          length -= result;
          count += result;
        }
        if (result != -1) {
          cbuf_consume_cursor (&head->cursor, head->msg->length);
          msg_queue_remove (client->messages);
        }
      }

      if (client->state == C_DISCONNECTED && client->messages->length == 0) {
        loginfo ("Client disconnected and all pending measurements have been sent; shutting down this client\n");
        shutdown (client->send_socket, SHUT_RDWR);
        sleep (1);
        close (client->send_socket);
        session_remove_client (session, client);
        client_free (client);
        pthread_exit (NULL);
      }
    } else if (session->state == ProxyState_PAUSED && client->sender_connected) {
      shutdown (client->send_socket, SHUT_RDWR);
      sleep (1);
      close (client->send_socket);
      client->sender_connected = 0;
    }
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
