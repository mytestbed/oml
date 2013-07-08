/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file eventloop.c
 * \brief Callback-based event loop supporting file descriptors, socket and timers.
 *
 * \see eventloop_init, eventloop_run, eventloop_stop
 * \eventloop_on_stdin, eventloop_on_monitor_in_channel, eventloop_on_read_in_channel, eventloop_on_out_channel
 * \see o_el_timer_callback, o_el_read_socket_callback, o_el_monitor_socket_callback, o_el_state_socket_callback, o_el_timer_callback
 * \see poll(3)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <poll.h>
#include <assert.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>

#include "mem.h"
#include "ocomm/o_log.h"
#include "ocomm/o_socket.h"
#include "ocomm/o_eventloop.h"
#include "mem.h"

/** Initial expected number of socket event sources */
#define DEF_FDS_LENGTH 10
#define MAX_READ_BUFFER_SIZE 512

/** Default time, in second, after which an idle socket is cleaned up */
#define DEF_SOCKET_TIMEOUT 60

/* A quick hace to avoid having to repeat too much */
#define case_string(val)  case val: { return #val; break; }
const char* socket_status_string(SocketStatus status)
{
  switch(status) {
    case_string(SOCKET_UNKNOWN);
    case_string(SOCKET_WRITEABLE);
    case_string(SOCKET_CONN_CLOSED);
    case_string(SOCKET_CONN_REFUSED);
    case_string(SOCKET_DROPPED);
    case_string(SOCKET_IDLE);
  default:
    return "Unknown SocketStatus";
  }
}

/** Instance of a TimerEvtSource with callbacks.
 *
 * \see eventloop_every
 */
typedef struct _timerInt {
  /** \see TimerEvtSource */
  char *name;

  /** If non zero, channel is currently monitored by the EventLoop \see
   * eventloop_socket_activate */
  int is_active;

  /** Non zero if the timer is periodic */
  int is_periodic;

  /** Period of the timer: in seconds */
  int period;

  /** Next UNIX time when the timer will expire */
  time_t due_time;

  /** Function pointer to the timeout callback \see o_el_timer_callback */
  o_el_timer_callback callback;

  /** Pointer to application-provided data */
  void *handle;

  /** Pointer to next TimerInt in the linked-list */
  struct _timerInt* next;

  /** Buffer where name is actually stored */
  char nameBuf[64];

} TimerInt;

/** Instance of a SockEvtSource with data storage and callbacks.
 *
 * \see channel_new, channel_free
 */
typedef struct _channel {
  /** \see SockEvtSource */
  char *name;
  /** \see SockEvtSource */
  Socket* socket;

  /** If non zero, channel is currently monitored by the EventLoop
   * \see eventloop_socket_activate */
  int is_active;

  /** If non zero, the channel should be released when no more data is
   * available
   * \see eventloop_socket_release*/
  int is_shutting_down;

  /** If non zero, the EventLoop will release the socket at the next
   * iteration */
  int is_removable;

  /** Function pointer to the read callback for this channel */
  o_el_read_socket_callback read_cbk;

  /** Function pointer to the monitoring callback for this channel */
  o_el_monitor_socket_callback monitor_cbk;

  /** Function pointer to the status-change callback for this channel
   * \see o_el_state_socket_callback */
  o_el_state_socket_callback status_cbk;

  /** Pointer to application-provided data */
  void *handle;

  /** File descriptor associated to that Channel */
  int fds_fd;
  /** Mask of events monitored for that FD \see poll(3) */
  int fds_events;

  /** Pointer to next Channel in the linked-list */
  struct _channel* next;

  /** Buffer where name is actually stored */
  char nameBuf[64];

  /** Last UNIX time this channel was active */
  time_t last_activity;
} Channel;

/** EventLoop object storing the internal internal state */
typedef struct _eventLoop {
  /** Linked list of registered channels */
  Channel* channels;
  /** Linked list registered timers */
  TimerInt* timers;

  /** Array of descriptors to monitor
   * \see update_fds */
  struct pollfd* fds;
  /** Array of channels associated to the descriptors in fds */
  Channel** fds_channels;
  /** If non zero, fds structure needs to get recomputed
   * \see eventloop_socket_activate */
  int fds_dirty;
  /** Number of active descriptors in the fds array */
  int size;
  /** Allocated size of fds and fds_channels arrays */
  int length;

  /** Timeout after which sockets are considered idle, and reaped
   * \see DEF_SOCKET_TIMEOUT */
  int socket_timeout;

  /** Stopping condition for the event loop */
  int stopping;
  /** If set to 1, the eventloop will not wait for active FDs to be closed */
  int force_stop;

  /** UNIX Time when the EventLoop was started
   * \see time(3) */
  time_t start;
  /** Current UNIX time (updated whenever poll() returns)
   * \see time(3)*/
  time_t now;
  /** Last UNIX time idle sockets were reaped
   * \see time(3) */
  time_t last_reaped;

} EventLoop;


/* Local helpers, defined at the end of this file */
static Channel* channel_new(char* name, int fd, int fd_events, o_el_state_socket_callback status_cbk, void* handle);
static void channel_free(Channel *ch);

/* XXX: These should probably be made non-static, but they use the Channel type */
static Channel* eventloop_on_in_fd(char* name, int fd, o_el_read_socket_callback read_cbk, o_el_monitor_socket_callback monitor_cbk, o_el_state_socket_callback status_cbk, void* handle);
static Channel* eventloop_on_out_fd( char* name, int fd, o_el_state_socket_callback status_cbk, void* handle);

static int update_fds(void);
static void terminate_fds(void);

static void do_read_callback (Channel *ch, void *buffer, int buf_size);
static void do_monitor_callback (Channel *ch);
static void do_status_callback (Channel *ch, SocketStatus status, int error);


/** Global EventLoop object */
static EventLoop self;


/** Initialise the global EventLoop
 * \see eventloop_run, eventloop_stop, eventloop_terminate
 */
void eventloop_init()
{
  memset(&self, 0, sizeof(EventLoop));

  self.fds = NULL;
  self.channels = NULL;
  self.timers = NULL;

  self.size = 0;
  self.length = 0;

  eventloop_set_socket_timeout(DEF_SOCKET_TIMEOUT);

  /* Just to be sure we initialise everything */
  self.start = self.now = self.last_reaped = -1;
}

/** Set the timeout, in seconds, after which idle sockets are reaped.
 *
 * \param to timeout [s], 0 to disable
 */
void eventloop_set_socket_timeout(unsigned int to)
{
  o_log(O_LOG_DEBUG2, "EventLoop: Setting socket idleness timeout to %ds\n", to);
  self.socket_timeout = to;
}

/** Run the global EventLoop until eventloop_stop() or eventloop_terminate() is called.
 *
 * The loop is based around the poll(3) system call. It monitor event sources
 * such as Channel or Timers, registered in the respective fields of the global
 * EventLoop object self. It first consider all timers to find whether some
 * have expired and to set the timeout for the poll(3) call. It then calls
 * poll(3) on the file descriptors (STDIN or sockets) related to active
 * Channels, and runs the relevant callbacks for those with pending events.  It
 * finally executes the callback functions of the expired timers.  The loop
 * will not return until eventloop_stop() or eventloop_terminate() is called.
 * In the former case, it will try to wait until all active sockets are close,
 * while not in the latter.
 *
 * \return the (non-zero) value passed to eventloop_stop() or eventloop_terminate()
 *
 * \see eventloop_init, eventloop_stop, eventloop_terminate
 * \eventloop_on_stdin, eventloop_on_monitor_in_channel, eventloop_on_read_in_channel, eventloop_on_out_channel
 * \see o_el_timer_callback, o_el_read_socket_callback, o_el_monitor_socket_callback, o_el_state_socket_callback, o_el_timer_callback
 * \see poll(3)
 */
int eventloop_run()
{
  int i;
  self.stopping = 0;
  self.force_stop = 0;
  self.start = self.now = self.last_reaped = time(NULL);
  while (!self.stopping || (self.size>0 && !self.force_stop)) {
    // Check for active timers
    int timeout = -1;
    TimerInt* t = self.timers;
    while (t != NULL) {
      if (t->is_active) {
        int delta = 1000 * (t->due_time - self.now);
        if (delta < 0) delta = 0; // overdue
        if (delta < timeout || timeout < 0) timeout = delta;
      }
      t = t->next;
    }
    if (timeout != -1)
      o_log(O_LOG_DEBUG3, "EventLoop: Timeout = %d\n", timeout);

    if (self.fds_dirty)
      if (update_fds()<1 && timeout < 0) /* No FD nor timeout */
        continue;
    o_log(O_LOG_DEBUG4, "EventLoop: About to poll() on %d FDs with a timeout of %ds\n", self.size, timeout);
    /* for(i=0; i < self.size; i++) {
      o_log(O_LOG_DEBUG4, "EventLoop: FD %d->%s\n", self.fds[i].fd, self.fds_channels[i]->name);
    } */

    int count = poll(self.fds, self.size, timeout);
    self.now = time(NULL);

    if (count < 1) {
      o_log(O_LOG_DEBUG4, "EventLoop: Timeout\n");
    } else {
    // Check sockets
      i = 0;
      o_log(O_LOG_DEBUG4, "EventLoop: Got events\n");
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
              }
            }
            eventloop_socket_activate((SockEvtSource*)ch, 0);
            do_status_callback (ch, status, errno);
          } else {
            o_log(O_LOG_ERROR, "EventLoop: Expected error on socket '%s' but read '%s'\n", ch->name, buf);
          }
        } else if (self.fds[i].revents & POLLHUP) {
          eventloop_socket_activate((SockEvtSource*)ch, 0);

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
              o_log(O_LOG_DEBUG3, "EventLoop: Received last %i bytes\n", len);
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
            ch->last_activity = self.now;
            if (len > 0) {
              o_log(O_LOG_DEBUG3, "EventLoop: Received %i bytes\n", len);
              do_read_callback (ch, buf, len);
            } else if (len == 0 && ch->socket != NULL) {  // skip stdin
              // closed down
              eventloop_socket_activate((SockEvtSource*)ch, 0);
              do_status_callback (ch, SOCKET_CONN_CLOSED, 0);
            } else if (len < 0) {
              if (errno == ENOTSOCK) {
                o_log(O_LOG_ERROR,
                      "EventLoop: Monitored socket '%s' is now invalid; "
                      "removing from monitored set\n",
                      ch->name);
                eventloop_socket_remove ((SockEvtSource*)ch);
              } else {
                o_log(O_LOG_ERROR, "EventLoop: Unrecognized read error not handled (errno=%d)\n",
                      errno);
              }
            }
          } else {
            do_monitor_callback (ch);
          }
        } else if (ch->is_shutting_down) {
          /* The socket was shutting down, and nothing new has appeared;
           * We flushed the buffers, mark it as removable */
          eventloop_socket_release((SockEvtSource*)ch);
        }

        if (self.fds[i].revents & POLLOUT) {
          do_status_callback(ch, SOCKET_WRITEABLE, 0);
          if (0 != ch->last_activity) {
            /* If we track the activity of this socket */
            ch->last_activity = self.now;
          }
        }

        if (self.fds[i].revents & POLLNVAL) {
          o_log(O_LOG_WARN, "EventLoop: socket '%s' invalid, deactivating...\n", ch->name);
          eventloop_socket_activate((SockEvtSource*)ch, 0);
          do_status_callback(ch, SOCKET_DROPPED, 0);
        }

        /* We reap idle channels as we go through the list.  XXX: There might
         * be a corner case where all FDs are already used, and some of them
         * idle, however a new a new connection (on one listening socket early
         * in the list) would be dropped before cleanup triggered by its
         * arrival freed the resources it needs (from the idle sockets further
         * towards the end of the list. See #959.*/
        if (ch->last_activity != 0 &&
            self.socket_timeout > 0 &&
            self.now - ch->last_activity > self.socket_timeout) {
            o_log(O_LOG_DEBUG2, "EventLoop: Socket '%s' idle for %ds, reaping...\n", ch->name, self.now - ch->last_activity);
          do_status_callback(ch, SOCKET_IDLE, 0);
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
          if (t->due_time <= self.now) {
            // fires
            o_log(O_LOG_DEBUG2, "EventLoop: Timer '%s' fired\n", t->name);
            if (t->callback) t->callback((TimerEvtSource*)t, t->handle);

            if (t->is_periodic) {
              while ((t->due_time += t->period) < self.now) {
                // should really only happen during debugging
                o_log(O_LOG_WARN, "EventLoop: Skipped timer period for '%s'\n",
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
  return self.stopping;
}

/** Stop the eventloop,
 *
 * The eventloop will try to gracefully finish by waiting for all active FDs to be closed.
 *
 * \param reason a non-zero reason for stopping the loop; default to 1, with a warning
 */
void eventloop_stop(int reason)
{
  if(reason) {
    self.stopping = reason;
  } else {
    o_log(O_LOG_WARN, "EventLoop: Tried to stop with no reason, defaulting to 1");
    self.stopping = 1;
  }
  terminate_fds();
}

/** Terminate the eventloop,
 *
 * \param reason a non-zero reason for stopping the loop; default to 1, with a warning
 */
void eventloop_terminate(int reason)
{
    self.force_stop = 1;
    eventloop_stop(reason);
}

/** Log a summary of resource usage.
 *
 * \param loglevel log level at which the message should be issued
 * \see xmemsummary
 */
void eventloop_report (int loglevel)
{
  o_log(loglevel, "EventLoop: Open file descriptors: %d/%d\n", self.size, self.length);
  o_log(loglevel, "EventLoop: Memory usage: %s\n", xmemsummary());
}


/** Register a new periodic timer to the event loop
 *
 * \param name name of this object, used for debugging
 * \param period period [s] of the timer
 * \param timer_cbk function called when the state of the timer expires
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to the newly-created TimerInt, cast as a TimerEvtSource
 *
 * \see o_el_timer_callback, eventloop_timer_stop
 */
TimerEvtSource* eventloop_every(
  char* name,
  int period,
  o_el_timer_callback callback,
  void* handle
) {
  TimerInt* t = (TimerInt*)xmalloc(sizeof(TimerInt));
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

/** Deallocate memory for a TimerInt
 * \param timer TimerInt to deallocate
 * \see eventloop_every, eventloop_timer_stop
 */
static void eventloop_timer_free(TimerInt* timer) {
  if (!timer) {
    o_log(O_LOG_DEBUG, "EventLoop: %s: Trying to free NULL pointer\n", __FUNCTION__);
  } else {
    xfree(timer);
  }
}

/** Stop timer and free its allocated memory
 *
 * \param timer TimerEvtSource to terminate and free
 *
 * \see eventloop_every
 * \see xfree
 */
void eventloop_timer_stop(TimerEvtSource* timer) {
  TimerInt *t = (TimerInt *)timer;

  /* Update the linked list */
  if (self.timers == t) {
    self.timers = t->next;

  } else {
    TimerInt* prev = self.timers;
    TimerInt* p = prev->next;

    while (p != NULL) {

      if (p == t) {
        prev->next = t->next;
        p = NULL; /* We're done */
      } else {
        prev = p;
        p = p->next;
      }
    }
  }

  eventloop_timer_free(t);
}


/** Register STDIN as a new input channel to read data from.
 *
 * \param data_cbk read callback called with freshly-read data, can be NULL
 * \param status_cbk status-change callback, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to a new Channel cast as a SockEvtSource
 *
 * \see o_el_read_socket_callback, o_el_state_socket_callback
 */
SockEvtSource* eventloop_on_stdin(
  o_el_read_socket_callback read_cbk,
  void* handle
) {
  char* s = "stdin";
  Channel* ch;

  ch = eventloop_on_in_fd(s, 0, read_cbk, NULL, NULL, handle);

  return (SockEvtSource*)ch;
}

/** Register a Socket as a new input channel to read data from.
 *
 * \param socket OComm Socket
 * \param data_cbk read callback called with freshly-read data, can be NULL
 * \param status_cbk status-change callback, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to a new Channel cast as a SockEvtSource
 *
 * \see Socket, o_el_read_socket_callback, o_el_state_socket_callback
 */
SockEvtSource* eventloop_on_read_in_channel(
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
  ch->last_activity = self.now;
  return (SockEvtSource*)ch;
}

/** Register a Socket as a new channel to monitor (e.g., listening socket).
 *
 * \param socket OComm Socket
 * \param monitor_cbk monitoring callback called when a new event arrives, can be NULL
 * \param status_cbk status-change callback, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to a new Channel cast as a SockEvtSource
 *
 * \see Socket, o_el_monitor_socket_callback, o_el_state_socket_callback
 */
SockEvtSource* eventloop_on_monitor_in_channel(
  Socket* socket,
  o_el_monitor_socket_callback monitor_cbk,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  if (socket == NULL) {
    o_log(O_LOG_ERROR, "EventLoop: Missing socket\n");
    return NULL;
  }
  Channel* ch;
  ch = eventloop_on_in_fd(socket->name, socket->get_sockfd(socket),
              NULL, monitor_cbk, status_cbk, handle);
  ch->socket = socket;
  return (SockEvtSource*)ch;
}

/** Register a Socket as a new output channel.
 *
 * In essence, this only registers status-change callback to allow reception of
 * a SOCKET_WRITEABLE SocketStatus.
 *
 * \param socket OComm Socket
 * \param status_cbk status-change callback, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to a new Channel cast as a SockEvtSource
 *
 * \see Socket, o_el_state_socket_callback, SocketStatus
 */
SockEvtSource* eventloop_on_out_channel(
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

/** Mark socket event source (channel) as active or not.
 *
 * This triggers FD update if need be.
 *
 * \param source SockEvtSource to (de)activate
 * \param flag 0 to deactivate, anything else to activate (use 1)
 *
 * \see update_fds
 */
void eventloop_socket_activate(SockEvtSource* source, int flag)
{
  Channel* ch = (Channel*)source;
  if (ch->is_active != flag) {
    ch->is_active = flag;
    self.fds_dirty = 1;
  }
}

/** Tell the EventLoop to release a channel.
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
void eventloop_socket_release(SockEvtSource* source)
{
  Channel *ch = (Channel*)source;
  eventloop_socket_activate(source, 0);
  ch->is_removable = 1;
  ch->handle = NULL;
}

/** Remove channels from monitoring of the EventLoop.
 *
 * The EventLoop calls this function on its own when sockets have been released
 * using eventloop_socket_release(). You probably want to use that one instead.
 *
 * \param source SockEvtSource to remove and free
 * \see eventloop_socket_release, channel_free
 */
void eventloop_socket_remove(SockEvtSource* source)
{
  Channel* ch = (Channel*)source;

  eventloop_socket_activate(source, 0);

  /* Update the linked list */
  if (self.channels == ch) {
    self.channels = ch->next;

  } else {
    Channel* prev = self.channels;
    Channel* p = prev->next;

    while (p != NULL) {
      if (p == ch) {
        prev->next = ch->next;
        p = NULL; /* We're done */
      } else {
        prev = p;
        p = p->next;
      }
    }
  }

  channel_free(ch);
}


/** Create a new channel and register it to the EventLoop.
 *
 * The Channel is allocated and initialised. It is also registered to the
 * global EventLoop self, at the beginning of the channels linked list, and activated.
 *
 * \param name name of this object, used for debugging
 * \param fd file descriptor linked to the channel
 * \param fd_events event flags for poll()
 * \param status_cbk callback function called when the state of the file descriptor changes
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to the newly-created Channel
 *
 * \see EventLoop, eventloop_socket_activate
 * \see xmalloc
 * \see poll(3)
 */
static Channel* channel_new(
  char* name,
  int fd,
  int fd_events,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  Channel* ch = (Channel *)xmalloc(sizeof(Channel));
  memset(ch, 0, sizeof(Channel));

  ch->fds_fd = fd;
  ch->fds_events = fd_events;

  ch->name = ch->nameBuf;
  strcpy(ch->name, name);

  ch->status_cbk = status_cbk;
  ch->handle = handle;

  ch->next = self.channels;
  self.channels = ch;

  eventloop_socket_activate((SockEvtSource*)ch, 1); /* Updates ch->is_active */

  return ch;
}

/** Terminate Channel and free its allocated memory
 *
 * The file descriptor in ch->socket is *NOT* cleaned up here, as all Channels are created through eventloop_on_* functions which first argument is a Socket, socket or file descriptor. The caller of these functions is in charge of cleaning
 *
 * \param ch Channel to terminate and free
 *
 * \see channel_new
 * \see xfree
 */
static void channel_free(
    Channel *ch
) {
  if (!ch) {
    o_log(O_LOG_DEBUG, "EventLoop: %s: Trying to free NULL pointer\n", __FUNCTION__);
  } else {
    xfree(ch);
  }
}

/** Create a new Channel with the specified callbacks on incoming data.
 *
 * \param name name of this object, used for debugging
 * \param fd file descriptor to consider
 * \param read_cbk callback function called when there is data to read, can be NULL
 * \param monitor_cbk callback function called when a new normal event arrives but no read_cbk was registered (e.g., listening socket), can be NULL
 * \param status_cbk callback function called when the state of the socket changes, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to the new Channel
 *
 * \see o_el_read_socket_callback, o_el_monitor_socket_callback, o_el_state_socket_callback
 * \see channel_new
 */
static Channel* eventloop_on_in_fd(
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
  /* status_cbk is already set by channel_new */
  return ch;
}

/** Create a new Channel with the specified callbacks to manage outgoing data.
 *
 * \param name name of this object, used for debugging
 * \param fd file descriptor to consider
 * \param status_cbk callback function called when the state of the socket changes, can be NULL
 * \param handle pointer to opaque data passed to callback functions
 * \return a pointer to the new Channel
 *
 * \see o_el_state_socket_callback
 * \see channel_new
 */
static Channel* eventloop_on_out_fd(
  char* name,
  int fd,
  o_el_state_socket_callback status_cbk,
  void* handle
) {
  Channel* ch = channel_new(name, fd, POLLOUT, status_cbk, handle);

  return ch;
}

/** Update the number of currently active Channels
 * \return the number of active channels
 */
static int update_fds(void)
{
  Channel* ch = self.channels;
  int i = 0;

  while (ch != NULL) {
    if (ch->is_active) {
      if (self.length <= i) {
        // Need to increase size of fds array
        int l = (self.length > 0 ? 2 * self.length : DEF_FDS_LENGTH);
        self.fds = (struct pollfd *)realloc(self.fds, l * sizeof(struct pollfd));
        self.fds_channels = (Channel **)realloc(self.fds_channels, l * sizeof(Channel*));
        self.length = l;
      }
      self.fds[i].fd = ch->fds_fd;
      self.fds[i].events = ch->fds_events;
      self.fds_channels[i] = ch;
      i++;
    }
    ch = ch->next;
  }
  o_log(O_LOG_DEBUG, "EventLoop: %d active channel%s\n", i, i>1?"s":"");

  self.size = i;
  self.fds_dirty = 0;

  return i;
}

/** Terminate sources.
 *
 * Close listening Sockets and shutdown() others
 *
 * XXX: This function is not very efficient (going through the linked list of
 * channels and repeatedly calling functions which do the same), but it's only
 * used for cleanup, so it should be fine.
 *
 * \see eventloop_stop
 */
static void terminate_fds(void)
{
  Channel *ch = self.channels, *next;

  while (ch != NULL) {
    next = ch->next;
    o_log(O_LOG_DEBUG4, "EventLoop: Terminating channel %s\n", ch->name);
    if (!ch->is_active ||
        socket_is_disconnected(ch->socket) ||
        socket_is_listening(ch->socket)) {
      o_log(O_LOG_DEBUG3, "EventLoop: Releasing listening channel %s\n", ch->name);
      eventloop_socket_release((SockEvtSource*)ch);
    } else if (self.force_stop) {
      o_log(O_LOG_DEBUG3, "EventLoop: Closing down %s\n", ch->name);
      eventloop_socket_release((SockEvtSource*)ch);
      socket_close(ch->socket);
    } else {
      o_log(O_LOG_DEBUG3, "EventLoop: Shutting down %s\n", ch->name);
      socket_shutdown(ch->socket);
      ch->is_shutting_down = 1;
    }
    ch = next;
  }

  update_fds();
}

/** Execute the data-read callback of a channel, if defined.
 *
 * \param ch Channel just read from
 * \param buffer pointer to a buffer containing the read data
 * \param buf_size size of data in buffer
 *
 * \see o_el_read_socket_callback
 */
static void do_read_callback (Channel *ch, void *buffer, int buf_size)
{
  if (ch->read_cbk && !ch->is_removable) {
    ch->read_cbk ((SockEvtSource*)ch, ch->handle, buffer, buf_size);
  } else {
    o_log(O_LOG_DEBUG, "EventLoop: Channel '%s' has fresh data but no defined callback\n",  ch->name);
  }
}

/** Execute the monitoring callback of a channel, if defined.
 *
 * \param ch Channel which just changed state
 *
 * \see o_el_monitor_socket_callback
 */
static void do_monitor_callback (Channel *ch)
{
  if (ch->monitor_cbk) {
    ch->monitor_cbk((SockEvtSource*)ch, ch->handle);
  } else {
    o_log(O_LOG_DEBUG, "EventLoop: Channel '%s' has fresh data but no defined callback\n",  ch->name);
  }
}

/** Execute the status-change callback of a channel, if defined.
 *
 * Otherwise, a default behaviour is implemented, cleaning up ch->socket on
 * SOCKET_CONN_CLOSED, SOCKET_CONN_REFUSED SOCKET_DROPPED and SOCKET_IDLE.
 *
 * \param ch Channel which just changed state
 * \param status SocketStatus new status for the socket
 * \param error errno related to that status
 *
 * \see o_el_state_socket_callback, SocketStatus
 */
static void do_status_callback (Channel *ch, SocketStatus status, int error)
{
  if (ch->status_cbk && !ch->is_removable) {
    ch->status_cbk ((SockEvtSource*)ch, status, error, ch->handle);

  } else if (!ch->status_cbk) {
    o_log(O_LOG_DEBUG, "EventLoop: Channel '%s' has changed state but no defined callback\n",  ch->name);

    switch(status) {
    case SOCKET_WRITEABLE:
      break;
    case SOCKET_CONN_CLOSED:
    case SOCKET_CONN_REFUSED:
    case SOCKET_DROPPED:
    case SOCKET_IDLE:
      o_log(O_LOG_DEBUG, "EventLoop: Closing socket '%s' due to status %d\n", ch->name, status);
      eventloop_socket_release((SockEvtSource*)ch);
      break;
    case SOCKET_UNKNOWN:
    default:
      o_log(O_LOG_WARN, "EventLoop: Unexpected status on socket '%s': %d\n", ch->name, status);
      break;
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
