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
/*!\file log.c
  \brief This file logging functions
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <time.h>

#include "ocomm/o_log.h"

/** Maximal logging period for repeated messages, in seconds */
#define MAX_MESSAGE_RATE 1
/** Maximum buffer length for log messages */
#define LOG_BUF_LEN  1024
/** Defines for how many repeated messages a log entry should be written first */
#define INIT_LOG_EXPONENT 8

extern int errno;

void o_log_simplified(int level, const char* format, ...);
void o_vlog_simplified(int level, const char* format, va_list va);

static FILE* logfile;
int o_log_level = O_LOG_INFO;

o_log_fn o_log = o_log_simplified;
o_vlog_fn o_vlog = o_vlog_simplified;

void
o_set_log_file (char* name)
{
  if (name == NULL) {
    fprintf (stderr, "The log file name is NULL!  Logging to stderr instead\n");
    logfile = NULL;
  } else {
    if (*name == '-') {
      logfile = NULL;
    } else {
      logfile = fopen(name, "a");
      if (logfile == NULL) {
        fprintf(stderr, "Can't open logfile '%s'\n", name);
        return;
      }
      setvbuf (logfile, NULL, _IOLBF, 1024);
    }
  }
}

void
o_set_log_level (int level)
{
  o_log_level = level;
}

/** Set the format-based log function if non NULL, or the default one.
 *
 * The default one simply parses the format int a varrarg and calls o_vlog().
 *
 * \param log_fn function to use for logging
 * \return the current logging function
 *
 */
o_log_fn
o_set_log (o_log_fn new_log_fn)
{
  if ((o_log = new_log_fn) == NULL) {
    o_log = o_log_simplified;
  }
  return o_log;
}

/** Set the vararg-based vlog function if non NULL, or the default one.
 * \param vlog_fn function to use for logging
 * \return the current logging function
 */
o_vlog_fn
o_set_vlog (o_vlog_fn new_vlog_fn)
{
  if ((o_vlog = new_vlog_fn) == NULL) {
    o_vlog = o_vlog_simplified;
  }
  return o_vlog;
}

void
o_set_simplified_logging (void)
{
  o_log = o_log_simplified;
  o_vlog = o_vlog_simplified;
}

/** Print meta information about log message depending on the output stream.
 *
 * \param stream file descriptor where to write the messag
 * \param now time to label the message with
 * \param level loglevel of the message
 * \return \return the number of characters written to the log by fprintf()
 * \see time, fprintf
 */
static int _o_vlog_metainformation(FILE *stream, time_t now, int level) {
  const char * const labels [] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
  };
  const int label_max = sizeof(labels) / sizeof(labels[0]) - 1;
  int label_index;
  int debug_level;
  struct tm* ltime;
  char now_str[20];
  int n = 0;

  label_index = level - O_LOG_ERROR; /* O_LOG_ERROR is negative */
  label_index = (label_index < 0) ? 0 : label_index;
  label_index = (label_index > label_max) ? label_max : label_index;

  if (stream != stderr) {
    ltime = localtime(&now);

    strftime(now_str, 20, "%b %d %H:%M:%S", ltime);
    n += fprintf (logfile, "%s  ", now_str);
  }

  n += fprintf (logfile, "%-5s", labels[label_index]);

  if (level > O_LOG_DEBUG) {
    debug_level = level - O_LOG_INFO;
    n += fprintf (logfile, "%-2d ", debug_level);
  } else {
    n += fprintf (logfile, "  ");
  }
  
  return n;
}

/** Rate limit log output to at most one similar message per occurence or time
 * period, whichever comes first.
 *
 * The time period is set by MAX_MESSAGE_RATE (1s), and the occurence count
 * increases exponentially up to 2^63, starting at 1, but printing a final
 * tally when the message changes.
 *
 * The log message is limited to 1024 bytes, not counting metaninformation.
 *
 * \param stream file descriptor where to write the message
 * \param level loglevel of the message
 * \param now current time as given by time()
 * \param format format string
 * \param va variadic list of arguments for format
 * \return the number of characters written to the log by fprintf()
 * \see time, fprintf
 */
static int _o_vlog_ratelimited(FILE *stream, int level, time_t now, const char *format, va_list va) {
  static char b1[LOG_BUF_LEN], b2[LOG_BUF_LEN], *new_log=NULL, *last_log=NULL;
  static time_t last_time = (time_t)-1;
  static int last_level = O_LOG_INFO;
  static uint64_t nseen = 0;
  static uint64_t exponent = INIT_LOG_EXPONENT;
  int n = 0;
  char *tmp;
  
  if (!new_log || !last_log || last_time == (time_t)-1) {
    /* Initialisation of static arrays */
    *b1=0;
    *b2=0;
    new_log = b1;
    last_log = b2;
    last_time = now - 2*MAX_MESSAGE_RATE;
  }

  vsnprintf(new_log, LOG_BUF_LEN, format, va);

  if (difftime(now, last_time) < MAX_MESSAGE_RATE || nseen > 0) {
    if (!strncmp(new_log, last_log, LOG_BUF_LEN)) {
      nseen++;
      if(nseen <= exponent) return 0;

      n += _o_vlog_metainformation(stream, now, level);
      n += fprintf(logfile, "Last message repeated %" PRIu64 " time%s\n", nseen, nseen>1?"s":"");

      last_time = now;
      nseen = 0;
      if (exponent < (1ul<<63)) exponent <<= 1;

      return n;
    } else if (nseen >0) {
      /* Give a final count of all the previously unreported similar messages before printing the new one */
      n += _o_vlog_metainformation(stream, last_time, last_level);
      n += fprintf(logfile, "Last message repeated %" PRIu64 " time%s\n", nseen, nseen>1?"s":"");
    }
  }

  n += _o_vlog_metainformation(stream, now, level);
  n += fprintf(logfile, "%s", new_log);

  last_time = now;
  last_level = level;
  nseen = 0;
  exponent = INIT_LOG_EXPONENT;

  tmp = new_log;
  new_log = last_log;
  last_log = tmp;

  return n;
}

void
o_vlog_simplified (int level, const char *format, va_list va)
{
  time_t t;
  time(&t);

  if (level > o_log_level) return;

  if (logfile == NULL) {
    logfile = stderr;
    setlinebuf(logfile);
  }

  _o_vlog_ratelimited(logfile, level, t, format, va);
}

void
o_log_simplified(int level, const char* format, ...)
{
  if (level > o_log_level) return;

  va_list va;
  va_start(va, format);

  o_vlog_simplified (level, format, va);

  va_end(va);
}

/** Convenience function logging at level O_LOG_ERROR.
 *
 * \param fmt format string
 * \param ... format string arguments
 */
void logerror (const char *fmt, ...) {
  va_list va;
  va_start (va, fmt);
  o_vlog (O_LOG_ERROR, fmt, va);
  va_end (va);
}

/** Convenience function logging at level O_LOG_WARN.
 *
 * \param fmt format string
 * \param ... format string arguments
 */
void logwarn (const char *fmt, ...) {
  va_list va;
  va_start (va, fmt);
  o_vlog (O_LOG_WARN, fmt, va);
  va_end (va);
}

/** Convenience function logging at level O_LOG_INFO.
 *
 * \param fmt format string
 * \param ... format string arguments
 */
void loginfo (const char *fmt, ...) {
  va_list va;
  va_start (va, fmt);
  o_vlog (O_LOG_INFO, fmt, va);
  va_end (va);
}

/** Convenience function logging at level O_LOG_DEBUG.
 *
 * \param fmt format string
 * \param ... format string arguments
 */
void logdebug (const char *fmt, ...) {
  va_list va;
  va_start (va, fmt);
  o_vlog (O_LOG_DEBUG, fmt, va);
  va_end (va);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
