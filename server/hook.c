/*
 * Copyright 2012 National ICT Australia (NICTA), Australia
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
 * IMPLIE, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <signal.h>

#include <log.h>

#define HOOK_C_
#include "hook.h"

char* hook = NULL; /** Character string containing the name of the event hook program (or script) to instantiate. */
int hookpipe[] = {-1, -1}; /** Pipes to/from event hook. 
                            * The semantics are the same as pipe(3):
                            * read in hookpipe[0], write in hookpipe[1]
                            */
static int hookpid = -1; /** PID of the event hook script */

/** Clean up the pipes openned for the event hook */
static void
hook_clean_pipes (void) {
  logdebug("hook: Cleaning up pipes for `%s'\n", hook);
  close(hookpipe[0]);
  close(hookpipe[1]);
  hookpipe[0] = hookpipe[1] = -1;
}

/** Terminate the event hook process */
void
hook_cleanup (void)
{
  int i, ret;
  if (!hook) return;

  logdebug("hook: Cleaning up `%s'\n", hook);
  if(hook_write(HOOK_CMD_EXIT, sizeof(HOOK_CMD_EXIT)) > (int)sizeof(HOOK_CMD_EXIT))
    return;
  logdebug("hook: Problem commanding `%s' to exit: %s\n", hook, strerror(errno));

  if(!kill(hookpid, SIGTERM))
    return;
  logdebug("hook: Cannot kill (TERM) process %d: %s\n", hookpid, strerror(errno));

  for (i=0; i<3; i++) {
    ret = kill(hookpid, SIGKILL);
    if(ret == ESRCH)
      return;
    logdebug("hook: Cannot kill (KILL) process %d: %s\n", hookpid, strerror(errno));
  }

  logwarn("hook: Could not kill process %d; giving up...", hookpid);
}

/** Initialise the event hook if specified.
 *
 * This function creates two pipes, forks, redirects the child's stdin and
 * stdout to/from these pipes, then executes the hook program
 *
 * The hook program is expected to first print an identifying banner (\see
 * HOOK_BANNER), then wait for commands on stdin.
 *
 * Though a reverse pipe is also created for the hook's stdout to be available
 * to the main server process, it is not currently used for anything else than
 * getting the banner. Also, a read(2) on it from the main process is blocking.
 */
void
hook_setup (void)
{
  int pto[2], pfrom[2];
  fd_set readfds;
  struct timeval timeout = { .tv_sec=5, .tv_usec=0, };
  char buf[sizeof(HOOK_BANNER)];

  if (!hook)
    return;

  if (pipe(pto) || pipe(pfrom)) {
    logwarn("hook: Cannot create pipes to `%s': %s\n", hook, strerror(errno));
    goto clean_pipes;
  }

  hookpid = fork();
  if (hookpid < 0) {
    logwarn("hook: Cannot fork for `%s': %s\n", hook, strerror(errno));
    hookpid = -1;
    goto clean_pipes;

  } else if (0 == hookpid) {            /* Child process */
    close(pto[1]);
    close(pfrom[0]);
    dup2(pto[0], STDIN_FILENO);
    dup2(pfrom[1], STDOUT_FILENO);
    execlp(hook, hook, NULL);
    logwarn("hook: Cannot execute `%s': %s\n", hook, strerror(errno));
    exit(1);

  } else {                              /* Parent process */
    close(pto[0]);
    close(pfrom[1]);
    hookpipe[0] = pfrom[0];
    hookpipe[1] = pto[1];

    /* Wait for banner or timeout */
    FD_ZERO(&readfds);
    FD_SET(hookpipe[0], &readfds);
    logdebug("hook: Waiting for `%s' to respond...\n", hook);

    if(select(hookpipe[0]+1, &readfds, NULL, NULL, &timeout) < 1) {
      logwarn("hook: `%s' (PID %d) not responding\n", hook, hookpid);
      hook_cleanup();
      goto clean_pipes;
    }

    /* Only hookpipe[0] was in readfds, so we know why we're here */
    if(hook_read(&buf, sizeof(buf)) <= 0) {
      logwarn("hook: Cannot get banner from `%s' (PID %d): %s\n", hook, hookpid, strerror(errno));
    } else if (strncmp(buf, HOOK_BANNER, sizeof(HOOK_BANNER)-1)) {  /* XXX: Ignore final '\n' instead of '\0' */
      buf[sizeof(buf)-1]=0;
      logwarn("hook: Incorrect banner from `%s' (PID %d): `%s'\n", hook, hookpid, buf);
      goto clean_pipes;
    }
    loginfo("hook: `%s' in place\n", hook);
  }

  return;

clean_pipes:
  logdebug("hook: Giving up on `%s'\n", hook);
  close(pto[0]);
  close(pfrom[0]);
  close(pto[1]);
  close(pfrom[1]);
  hook_clean_pipes();
}

/** Determine whether an event hook has been enabled
 * \return 1 if hook enabled, 0 otherwise
 */
inline int
hook_enabled(void) {
 return hookpipe[0] >= 0 && hookpipe[1] >= 0;
}

/** Write commands to the event hook.
 * 
 * This function writes commands into the pipe connected to the event hook's
 * stdin. It takes the same parameters as write(2), but skips the file
 * descriptor.
 *
 * \param buf buffer from which +count+ bytes of data will be read out andwritten into event hook's +stdin+
 * \param count the number of bytes from +buf+ to write into the hook's +stdin+
 * \return the same as write(2), and sets +errno+ accordingly
 */
ssize_t
hook_write (const void *buf, size_t count)
{
  int n;
  if (-1 == hookpipe[1])
    return -1;
  logdebug("hook: Sending command fd %d: '%s'\n", hookpipe[1], buf);
  n = write(hookpipe[1], buf, count);
  return n;
}

/** Read commands to the event hook.
 * 
 * This function reads whatever the event hook outputs on its +stdout+
 * It takes the same parameters as read(2), but skips the file
 * descriptor.
 *
 * This read is blocking, and should be used when knowingly.
 *
 * \param buf buffer into which +count+ bytes frome the event hook's +stdout+
 * \param count the number of bytes to read into +buf+
 * \return the same as read(2), and sets +errno+ accordingly
 */
ssize_t
hook_read (void *buf, size_t count)
{
  int n;
  if (-1 == hookpipe[0])
    return -1;
  n = read(hookpipe[0], buf, count);
  logdebug("hook: Read from fd %d: '%s'\n", hookpipe[0], buf);
  return n;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
