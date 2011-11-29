/*
 * Copyright 2007-2011 National ICT Australia (NICTA), Australia
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
/*!\file eventloop.c
  \brief Implements an event loop dispatching callbacks
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
//#include <sys/errno.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "ocomm/o_eventloop.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_log.h"

//! Initial expected number of socket event sources
#define DEF_FDS_LENGTH 10
#define MAX_READ_BUFFER_SIZE 512

typedef struct _channel {
  char* name;

  Socket* socket;

  //! If true, is active, otherwise ignore
  int is_active;

  //! If true, this channel can be eventloop_socket_remove()'d
  //! next time through the poll loop
  int is_removable;

  //! Callback to call when channel is ready
  o_el_read_socket_callback read_cbk;
  o_el_monitor_socket_callback monitor_cbk;
  o_el_state_socket_callback status_cbk;
  void* handle;

  int fds_fd;
  int fds_events;

  struct _channel* next;


  char nameBuf[64];
} Channel;

typedef struct _timerInt {
  char* name;

  //! If true, is active, otherwise ignore
  int is_active;

  //! If true, periodically fire
  int is_periodic;

  //! Length of period [sec]
  int period;

  //! Unix time this timer should fire
  time_t due_time;

  //! Callback to call when timer fires
  o_el_timer_callback callback;
  void* handle;

  struct _timerInt* next;

  char nameBuf[64];

} TimerInt;

//! Stores the event loops internal state
typedef struct _eventLoop {

  //! Array of registered channels
  Channel* channels;

  //! Array of registered timer
  TimerInt* timers;

  //! Number of used timers
  //int timer_size;

  //! Array of descriptors to monitor
  struct pollfd* fds;

  //! Associated channel for fds entry
  Channel** fds_channels;

  //! Number of active descriptors
  int size;

  //! True if fds structure needs to get recomputed
  int fds_dirty;

  //! Length of fds array
  int length;


} EventLoop;

static EventLoop self;
static time_t now = -1;
static time_t start = -1;


void
eventloop_init()

{
  memset(&self, 0, sizeof(EventLoop));

  //  self.fds = (struct pollfd **)malloc(length * sizeof(struct pollfd*));
  self.channels = NULL; //(Channel **)malloc(length * sizeof(Channel*));
  self.timers = NULL; // (TimerInt **)malloc(length * sizeof(TimerInt*));
  //  self.timer_size = 0;

  self.size = 0;
  self.length = 0;

}

//! Build the fds array from active socket sources
static void
update_fds(void)

{
  Channel* ch = self.channels;
  int i = -1;

  while (ch != NULL) {
    if (ch->is_active) {
      i++;
      if (self.length <= i) {
    // Need to increase size of fds array
    int l = (self.length > 0 ? 2 * self.length : DEF_FDS_LENGTH);
    if (self.fds != NULL) free(self.fds);
    self.fds = (struct pollfd *)malloc(l * sizeof(struct pollfd));
    if (self.fds_channels != NULL) free(self.fds_channels);
    self.fds_channels = (Channel **)malloc(l * sizeof(Channel*));
    self.length = l;
    return update_fds();  // start over
      }
      self.fds[i].fd = ch->fds_fd;
      self.fds[i].events = ch->fds_events;
      self.fds_channels[i] = ch;
    }
    ch = ch->next;
  }
  self.size = i + 1;
  self.fds_dirty = 0;
}

static void
do_read_callback (Channel *ch, void *buffer, int buf_size)
{
  if (ch->read_cbk && !ch->is_removable)
    ch->read_cbk ((SockEvtSource*)ch, ch->handle, buffer, buf_size);
}

static void
do_status_callback (Channel *ch, SocketStatus status, int error)
{
  if (ch->status_cbk && !ch->is_removable)
    ch->status_cbk ((SockEvtSource*)ch, status, error, ch->handle);
}

static void
do_monitor_callback (Channel *ch)
{
  if (ch->monitor_cbk)
    ch->monitor_cbk((SockEvtSource*)ch, ch->handle);
}

void
eventloop_run()
{
  start = now = time(NULL);
  while (1) {
    // Check for active timers
    int timeout = -1;
    TimerInt* t = self.timers;
    while (t != NULL) {
      if (t->is_active) {
        int delta = 1000 * (t->due_time - now);
        if (delta < 0) delta = 0; // overdue
        if (delta < timeout || timeout < 0) timeout = delta;
      }
      t = t->next;
    }
    if (timeout != -1)
      o_log(O_LOG_DEBUG3, "Eventloop: Timeout = %d\n", timeout);

    if (self.fds_dirty) update_fds();
    int count = poll(self.fds, self.size, timeout);
    now = time(NULL);

    if (count > 0) {
      // Check sockets
      int i = 0;
      for (; i < self.size; i++) {
        Channel* ch = self.fds_channels[i];
        if (self.fds[i].revents & POLLERR) {
          char buf[32];
          SocketStatus status;
          int len;

          if ((len = recv(self.fds[i].fd, buf, 32, 0)) <= 0) {
            switch (errno) {
            case ECONNREFUSED:
              status = SOCKET_CONN_REFUSED;
              break;
            default:
              status = SOCKET_UNKNOWN;
              if (!ch->status_cbk) {
                o_log(O_LOG_ERROR, "EventLoop: While reading from socket '%s': (%d) %s\n",
                      ch->name, errno, strerror(errno));
                socket_close(ch->socket);
              }
            }
            ch->is_active = 0;
            self.fds_dirty = 1;
            do_status_callback (ch, status, errno);
          } else {
            o_log(O_LOG_ERROR, "EventLoop: Expected error on socket '%s' but read '%s'\n", ch->name, buf);
          }
        } else if (self.fds[i].revents & POLLHUP) {
          ch->is_active = 0;
          self.fds_dirty = 1;

          /* Client closed the connection, but there might still be bytes
             for us to read from our end of the connection. */
          int len;
          int fd = self.fds[i].fd;
          char buf[MAX_READ_BUFFER_SIZE];
          do {
            if (fd == 0) {
              len = read(fd, buf, MAX_READ_BUFFER_SIZE);
            } else {
              len = recv(fd, buf, 512, 0);
            }
            if (len > 0) {
              o_log(O_LOG_DEBUG3, "Eventloop:  received %i octets\n", len);
              do_read_callback (ch, buf, len);
            }
          } while (len > 0);
          do_status_callback (ch, SOCKET_CONN_CLOSED, 0);
        } else if (self.fds[i].revents & POLLIN) {
          char buf[MAX_READ_BUFFER_SIZE];
          if (ch->read_cbk) {
            int len;
            int fd = self.fds[i].fd;
            if (fd == 0) {
              // stdin
              len = read(fd, buf, MAX_READ_BUFFER_SIZE);
            } else {
              // socket
              len = recv(fd, buf, 512, 0);
            }
            if (len > 0) {
              o_log(O_LOG_DEBUG3, "Eventloop:  received %i octets\n", len);
              do_read_callback (ch, buf, len);
            } else if (len == 0 && ch->socket != NULL) {  // skip stdin
              // closed down
              ch->is_active = 0;
              self.fds_dirty = 1;
              // expect the callback to handle socket close
              do_status_callback (ch, SOCKET_CONN_CLOSED, 0);
              if (!ch->status_cbk)
                socket_close(ch->socket);
            } else if (len < 0) {
              if (errno == ENOTSOCK) {
                o_log(O_LOG_ERROR,
                      "Eventloop: Monitored socket '%s' is now invalid; "
                      "removing from monitored set\n",
                      ch->name);
                eventloop_socket_remove ((SockEvtSource*)ch);
              } else {
                o_log(O_LOG_ERROR, "Eventloop: Unrecognized read error not handled (errno=%d)\n",
                      errno);
              }
            }
          } else {
            do_monitor_callback (ch);
          }
        }
        if (self.fds[i].revents & POLLOUT)
          do_status_callback(ch, SOCKET_WRITEABLE, 0);
        if (self.fds[i].revents & POLLNVAL) {
          o_log(O_LOG_DEBUG3, "POLLNVAL\n");

          ch->is_active = 0;
          self.fds_dirty = 1;
          do_status_callback(ch, SOCKET_DROPPED, 0);
          if (!ch->status_cbk)
            o_log(O_LOG_WARN, "EventLoop: Deactivated socket '%s'\n", ch->name);
        }
      }
      for (i = 0; i < self.size; i++) {
        Channel* ch = self.fds_channels[i];
        if (ch->is_removable)
          eventloop_socket_remove ((SockEvtSource*)ch);
      }
    }
    if (timeout >= 0) {
      // check timers
      TimerInt* t = self.timers;
      while (t != NULL) {
        if (t->is_active) {
          if (t->due_time <= now) {
            // fires
            o_log(O_LOG_DEBUG2, "Eventloop: Timer '%s' fired\n", t->name);
            if (t->callback) t->callback((TimerEvtSource*)t, t->handle);

            if (t->is_periodic) {
              while ((t->due_time += t->period) < now) {
                // should really only happen during debugging
                o_log(O_LOG_WARN, "Eventloop: Skipped timer period for '%s'\n",
                      t->name);
              }
            } else {
              t->is_active = 0;
            }
          }
        }
        t = t->next;
      }
    }
  }
}

static Channel*
channel_new(
  char* name,
  int fd,
  int fd_events,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  Channel* ch = (Channel *)malloc(sizeof(Channel));
  memset(ch, 0, sizeof(Channel));

  ch->is_active = 1;
  ch->is_removable = 0;

  ch->fds_fd = fd;
  ch->fds_events = fd_events;

  ch->name = ch->nameBuf;
  strcpy(ch->name, name);

  ch->status_cbk = status_cbk;
  ch->handle = handle;

  ch->next = self.channels;
  self.channels = ch;

  self.fds_dirty = 1;
  return ch;
}

static Channel*
eventloop_on_in_fd(
  char* name,
  int fd,
  o_el_read_socket_callback read_cbk,
  o_el_monitor_socket_callback monitor_cbk,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  Channel* ch = channel_new(name, fd, POLLIN, status_cbk, handle);

  ch->read_cbk = read_cbk;
  ch->monitor_cbk = monitor_cbk;
  return ch;
}

SockEvtSource*
eventloop_on_read_in_channel(
  Socket* socket,
  o_el_read_socket_callback data_cbk,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  if (socket == NULL) {
    o_log(O_LOG_ERROR, "EventLoop: Missing socket\n");
    return NULL;
  }
  Channel* ch;
  ch = eventloop_on_in_fd(socket->name, socket->get_sockfd(socket),
              data_cbk, NULL, status_cbk, handle);
  ch->socket = socket;
  return (SockEvtSource*)ch;
}

SockEvtSource*
eventloop_on_monitor_in_channel(
  Socket* socket,
  o_el_monitor_socket_callback data_cbk,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  if (socket == NULL) {
    o_log(O_LOG_ERROR, "EventLoop: Missing socket\n");
    return NULL;
  }
  Channel* ch;
  ch = eventloop_on_in_fd(socket->name, socket->get_sockfd(socket),
              NULL, data_cbk, status_cbk, handle);
  ch->socket = socket;
  return (SockEvtSource*)ch;
}

SockEvtSource*
eventloop_on_stdin(
  o_el_read_socket_callback callback,
  void* handle
) {
  char* s = "stdin";
  Channel* ch;

  ch = eventloop_on_in_fd(s, 0, callback, NULL, NULL, handle);

  return (SockEvtSource*)ch;
}

static Channel*
eventloop_on_out_fd(
  char* name,
  int fd,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  Channel* ch = channel_new(name, fd, POLLOUT, status_cbk, handle);

  return ch;
}

SockEvtSource*
eventloop_on_out_channel(
  Socket* socket,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  if (socket == NULL) {
    o_log(O_LOG_ERROR, "EventLoop: Missing socket\n");
    return NULL;
  }
  Channel* ch;
  ch = eventloop_on_out_fd(socket->name, socket->get_sockfd(socket),
              status_cbk, handle);
  ch->socket = socket;

  return (SockEvtSource*)ch;
}

/*! Set activit flag of 'source' according to boolean 'flag'.
 */
void
eventloop_socket_activate(
  SockEvtSource* source,
  int flag
) {
  Channel* ch = (Channel*)source;
  if (ch->is_active != flag) {
    ch->is_active = flag;
    self.fds_dirty = 1;
  }
}

/*! Remove 'source' from being monitored.
 */
void
eventloop_socket_remove(
  SockEvtSource* source
) {
  Channel* ch = (Channel*)source;

  if (self.channels == ch) {
    // it's first
    self.channels = ch->next;
  } else {
    // TODO: this could be improved
    Channel* prev = self.channels;
    Channel* p = prev->next;
    while (p != NULL) {
      if (p == ch) {
    prev->next = ch->next;
    p = NULL;
      } else {
    prev = p;
    p = p->next;
      }
    }
  }
  free(ch);
  self.fds_dirty = 1;
}

/**
 *  @brief Tell the eventloop to release a socket event source.
 *
 *  This marks the socket as "removable", but does not remove it
 *  immediately.  The next time the event loop finishes processing
 *  events, it will scan the list of channels/sockets for ones that
 *  are removable, and will call eventloop_socket_remove() on them.
 *
 *  At that point, the socket data structure will be destroyed.
 *
 *  After this function is called on a socket, the eventloop will no
 *  longer generate callbacks for that socket, therefore the client
 *  code must ensure that it has already disposed of the socket's
 *  @c handle data structure, otherwise a memory leak might occur.
 */
void
eventloop_socket_release(SockEvtSource* source)
{
  Channel *ch = (Channel*)source;
  ch->is_active = 0;
  ch->is_removable = 1;
  ch->handle = NULL;
}


TimerEvtSource*
eventloop_every(
  char* name,
  int period,
  o_el_timer_callback callback,
  void* handle
) {
  TimerInt* t = (TimerInt *)malloc(sizeof(TimerInt));
  memset(t, 0, sizeof(TimerInt));

  t->name = t->nameBuf;
  strcpy(t->name, name);

  t->is_active = 1;
  t->is_periodic = 1;
  t->period = period;
  t->due_time = time(NULL) + period;
  t->callback = callback;
  t->handle = handle;

  t->next = self.timers;
  self.timers = t;

  return (TimerEvtSource*)t;
}

/*! Return the current time */
time_t
eventloop_now(void)

{
  return now - start;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
