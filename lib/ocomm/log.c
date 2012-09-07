/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
#include <stdarg.h>
#include <time.h>

#include "ocomm/o_log.h"

extern int errno;

static void o_log_simplified(int level, const char* format, ...);
static void o_vlog_simplified(int level, const char* format, va_list va);

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

void
o_vlog_simplified (int level, const char *format, va_list va)
{
  const char * const labels [] = {
    "ERROR",
    "WARN",
    "INFO",
    "DEBUG"
  };
  const int label_max = sizeof(labels) / sizeof(labels[0]) - 1;
  int label_index;
  int debug_level;

  if (level > o_log_level) return;

  label_index = level - O_LOG_ERROR; /* O_LOG_ERROR is negative */
  label_index = (label_index < 0) ? 0 : label_index;
  label_index = (label_index > label_max) ? label_max : label_index;

  if (logfile == NULL) {
    logfile = stderr;
    setlinebuf(logfile);
  }

  if (logfile != stderr) {
    time_t t;
    time(&t);
    struct tm* ltime = localtime(&t);

    char now[20];
    strftime(now, 20, "%b %d %H:%M:%S", ltime);
    fprintf (logfile, "%s  ", now);
  }

  fprintf (logfile, "%-5s", labels[label_index]);

  if (level > O_LOG_DEBUG) {
    debug_level = level - O_LOG_INFO;
    fprintf (logfile, "%-2d ", debug_level);
  } else {
    fprintf (logfile, "  ");
  }

  vfprintf(logfile, format, va);
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

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
