/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
/** Prototypes and interfaces for the OComm EventLoop
 * \author Max Ott (max@winlab.rutgers.edu)
 *
 * XXX: Names 'SockEvtSource' and 'Channel' would make more sense it they were
 * swapped, or something like 'ChannelEvtSource' and 'ChannelInt', to match the
 * Timer*'s regularity. Also, note SockEvtSources and Channels are used for STDIN.
 *
 * \see eventloop_init, eventloop_run, eventloop_stop
 * \eventloop_on_stdin, eventloop_on_monitor_in_channel, eventloop_on_read_in_channel, eventloop_on_out_channel
 * \see o_el_timer_callback, o_el_read_socket_callback, o_el_monitor_socket_callback, o_el_state_socket_callback, o_el_timer_callback
 * \see poll(3)
 */

#ifndef O_EVENTLOOP_H
#define O_EVENTLOOP_H

#include <time.h>

#include "ocomm/o_socket.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Timer-based event source base class for the EventLoop. */
typedef struct _TimerEvtSource {
  /** Name of the source, used for debugging */
  char* name;

} TimerEvtSource;


/** Socket-based event source base class for the EventLoop. */
typedef struct _sockEvtSource {
  /** Name of the source,  used for debugging */
  char* name;

  /** Socket object encapsulating the file descriptor */
  Socket* socket;

} SockEvtSource;

/** Possible socket statuses after state change.
 *
 * Depends on poll(3) revents and additional condition during further
 * processing (e.g., read from FD).
 *
 * XXX: Make sure to update socket_status_string when adding values here.
 *
 * \see o_el_state_socket_callback, socket_status_string
 * \see poll(3)
 */
typedef enum _SockStatus {
  /** Unknown POLLERR */
  SOCKET_UNKNOWN = -1,
  /** POLLOUT */
  SOCKET_WRITEABLE,
  /** POLLHUP or POLLIN and no data read (for sockets only) */
  SOCKET_CONN_CLOSED,
  /** If relevant on POLLERR */
  SOCKET_CONN_REFUSED,
  /** POLLNVAL */
  SOCKET_DROPPED,
  /** No data was receveid for more than DEF_SOCKET_TIMEOUT s */
  SOCKET_IDLE,
} SocketStatus;

const char* socket_status_string(SocketStatus status);

/** Timout callback prototype for timers.
 *
 * \param source TimerEvtSource which expired
 * \param handle pointer to application-supplied data
 */
typedef void (*o_el_timer_callback)(TimerEvtSource* source, void* handle);


/** Data-read callback prototype for sockets.
 *
 * The EventLoop takes care of recv(2)ing data from a socket ready to be read,
 * then passes this data to this callback.
 *
 * Called on POLLIN, and POLLHUP before o_el_state_socket_callback if some data remains to
 * be read.
 *
 * \param source SockEvtSource from which the event originated
 * \param handle pointer to application-supplied data
 * \param buffer pointer to a buffer containing the read data
 * \param buf_size size of data in buffer
 *
 * \see o_el_state_socket_callback
 * \see poll(3)
 */
typedef void (*o_el_read_socket_callback)(SockEvtSource* source, void* handle, void* buffer, int buf_size);

/** Monitoring callback prototype for sockets.
 *
 * This callback is a fallback when no data-read callback. Listening sockets,
 * for example, do not need to read any data, but need to be warned of incoming
 * connections. This is the role of this callback.
 *
 * XXX: Couldn't this be wrapped to a default read callback?
 *
 * \param source SockEvtSource from which the event originated
 * \param handle pointer to application-supplied data
 */
typedef void (*o_el_monitor_socket_callback)(SockEvtSource* source, void* handle);

/** State-changes callback prototype for sockets.
 *
 * If defined, this callback should release and free both application data,
 * pointed by handle, and the socket, in source->socket, on termination
 * conditions.
 *
 * By default (no callback), source->socket is closed on SOCKET_CONN_CLOSED,
 * SOCKET_CONN_REFUSED, SOCKET_DROPPED and SOCKET_IDLE.
 *
 * \param source SockEvtSource from which the event originated
 * \param status SocketStatus new status for the socket
 * \param error errno related to that status
 * \param handle pointer to application-supplied data
 *
 * XXX: This prototype is not very regular, with status and error appearing
 * before handle, unlike, e.g., o_el_read_socket_callback.
 *
 * \see SocketStatus, o_el_read_socket_callback
 * \see errno(3)
 */
typedef void (*o_el_state_socket_callback)(SockEvtSource* source, SocketStatus status, int error, void* handle);

void eventloop_init(void);
void eventloop_set_socket_timeout(unsigned int to);
int eventloop_run(void);
void eventloop_stop(int reason);
void eventloop_terminate(int reason);
void eventloop_report (int loglevel);

TimerEvtSource* eventloop_every(char* name, int period, o_el_timer_callback callback, void* handle);
void eventloop_timer_stop(TimerEvtSource* timer);

/* These functions create new channels around either STDIN or an OComm socket,
 * with various callbacks depending on their use */
SockEvtSource* eventloop_on_stdin( o_el_read_socket_callback callback, void* handle);
SockEvtSource* eventloop_on_monitor_in_channel(Socket* socket, o_el_monitor_socket_callback monitor_cbk, o_el_state_socket_callback status_cbk, void* handle);
SockEvtSource* eventloop_on_read_in_channel(Socket* socket,o_el_read_socket_callback data_cbk, o_el_state_socket_callback status_cbk, void* handle);
SockEvtSource* eventloop_on_out_channel( Socket* socket, o_el_state_socket_callback status_cbk, void* handle);

/* XXX: Is "socket" the right term here? */
void eventloop_socket_activate(SockEvtSource* source, int flag);
void eventloop_socket_release(SockEvtSource* source);
void eventloop_socket_remove(SockEvtSource* source);

#ifdef __cplusplus
}
#endif

#endif

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
