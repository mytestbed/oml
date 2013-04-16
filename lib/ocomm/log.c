/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file log.c
 * \brief Logging functions
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdarg.h>
#include <time.h>

#include "ocomm/o_log.h"
#include "oml_util.h"

/** Maximal logging period for repeated messages, in seconds */
#define MAX_MESSAGE_RATE 1
/** Maximum buffer length for log messages */
#define LOG_BUF_LEN  1024
/** Defines for how many repeated messages a log entry should be written first */
#define INIT_LOG_EXPONENT 8

static const char* const log_labels[] = {
  "ERROR",
  "WARN",
  "INFO",
  "DEBUG"
};

static void _o_log_real(int log_level, const char *fmt, ...);
static void o_log_simplified(int level, const char* format, ...);

/** Logfile to which messages are directed \see o_log_simplified */
static FILE* logfile = NULL;
/** Log level below which messages are shown \see O_LOG_ERROR, O_LOG_WARN, O_LOG_INFO, O_LOG_DEBUG, O_LOG_DEBUG2, O_LOG_DEBUG3, O_LOG_DEBUG4*/
int o_log_level = O_LOG_INFO;

/* XXX: This is needed for backward compatibility with <=2.9 */
o_log_fn o_log = _o_log_real;
/** The current logging function. \see _o_log, o_log */
o_log_fn o_log_function = o_log_simplified;

/** Direct the log stream to the named file.
 * \param name name of the file to write log into (if '-' or NULL, defaults to stderr)
 * \see o_log_simplified
 */
void
o_set_log_file (char* name)
{
  if (name == NULL) {
    fprintf (stderr, "The log file name is NULL!  Logging to stderr instead\n");
    logfile = stderr;

  } else {
    if (*name == '-') {
      logfile = stderr;

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

/** Set the log level below which messages should be displayed.
 * \param level log level
 * \see o_log, o_log_level
 */
void
o_set_log_level (int level)
{
  o_log_level = level;
}

/** Test if a given log level is currently active
 * \param level log level
 * \return 0 if inactive, 1 otherwise
 * \see o_set_log_level
 */
int
o_log_level_active(int log_level)
{
  if (log_level <= o_log_level) { return 1; }
  return 0;
}

/** Set the format string-based log function if non NULL, or o_log_simplified
 * \param log_fn function to use for logging
 * \return the current logging function
 * \see o_log_simplified
 */
o_log_fn
o_set_log (o_log_fn new_log_fn)
{
  if ((o_log_function = new_log_fn) == NULL) {
    o_log_function = o_log_simplified;
  }
  return o_log;
}

/** Reset the logging function to the internal o_log_simplified.
 * \see o_log_simplified, o_set_log
 */
void
o_set_simplified_logging (void)
{
  o_set_log(NULL);
}

/** Log message using the configured function, adding timestamps if needed.
 *
 * A timestamp is prepended if not using o_log_simplified or not logging to
 * stderr.
 *
 * No rate limitation nor level control is done here.
 *
 * \param now time_t to label the message with if needed
 * \param level loglevel of the message
 * \param msg message to log
 *
 * \see o_log, o_log_function, o_log_fn
 * \see time(3), vfprintf(3)
 */
static void
_o_log(time_t now, int level, char *msg)
{
  const int label_max = LENGTH(log_labels)-1;
  int label_index;
  struct tm* ltime;
  char now_str[20], dbglvl[] = { 0, 0 };

  label_index = level - O_LOG_ERROR; /* O_LOG_ERROR is negative */
  label_index = (label_index < 0) ? 0 : label_index;
  label_index = (label_index > label_max) ? label_max : label_index;

  if (level > O_LOG_DEBUG) {
    *dbglvl = '0' + (level - O_LOG_INFO);
  }

  if (o_log_simplified == o_log_function && stderr == logfile) {
  /* This logic should clearly go into o_log_simplified,
   * but as it is variadic, we have little option to manipulate its parameters */
    o_log_function(level, "%s%s\t%s", log_labels[label_index], dbglvl, msg);

  } else {
    ltime = localtime(&now);
    strftime(now_str, 20, "%b %d %H:%M:%S", ltime);
    o_log_function(level, "%s %s%s\t%s", now_str, log_labels[label_index], dbglvl ,msg);
  }
}

/** Log a message using the current logging backend.
 *
 * Log output is limited to at most one similar message per occurence or time
 * period, whichever comes first.
 *
 * The time period is set by MAX_MESSAGE_RATE (1s), and the occurence count
 * increases exponentially up to 2^63, starting at 1, but printing a final
 * tally when the message changes.
 *
 * The log message is limited to 1024 bytes (\ref LOG_BUF_LEN), not counting metaninformation.
 *
 * \param level log level for the message
 * \param fmt format string
 * \param ... arguments for format
 *
 * \see o_log
 */
static void
o_vlog(int log_level, const char* fmt, va_list va)
{
  static char b1[LOG_BUF_LEN], b2[LOG_BUF_LEN], *new_log=NULL, *last_log=NULL, *tmp;
  static int last_level = O_LOG_INFO;
  static time_t last_time = (time_t)0;
  static uint64_t nseen = 0;
  static uint64_t exponent = INIT_LOG_EXPONENT;
  time_t now;

  if (!o_log_level_active(log_level)) { return; }

  time(&now);

  if (!new_log || !last_log || last_time == (time_t)-1) {
    /* Initialisation of static arrays */
    *b1=0;
    *b2=0;
    new_log = b1;
    last_log = b2;
  }

  vsnprintf(new_log, LOG_BUF_LEN, fmt, va);

  if (difftime(now, last_time) < MAX_MESSAGE_RATE &&
      !strncmp(new_log, last_log, LOG_BUF_LEN) &&
      /* Same message and level at too high a rate */
      log_level == log_level) {

    if(++nseen > exponent) {
      snprintf(new_log, LOG_BUF_LEN,
          "Message repeated %" PRIu64 " time%s: %s", nseen, nseen>1?"s":"", last_log);
      _o_log(now, log_level, new_log);

      last_time = now;
      nseen = 0;

      if (exponent < (INT64_MAX/2)) { exponent *= 2; }
    }

  } else {
      /* Give a final count of all the previously unreported
       * similar messages before printing the new one */
    if (nseen > 1) {
      snprintf(last_log, LOG_BUF_LEN,
          "Previous message repeated %" PRIu64 " times\n", nseen);
      _o_log(last_time, last_level, last_log);

    } else if (nseen > 0) {
      /* If it was only repeated once, simply show it */
      _o_log(last_time, last_level, last_log);

    }

    /* New message */
    _o_log(now, log_level, new_log);

    last_time = now;
    last_level = log_level;
    nseen = 0;
    exponent = INIT_LOG_EXPONENT;

    tmp = new_log;
    new_log = last_log;
    last_log = tmp;

  }
}

/** Simplified logging function (default)
 *
 * Outputs the formated string to logfile, prepending a string representing the
 * error level.
 *
 * \copydoc o_log_fn
 */
static void
o_log_simplified(int level, const char* format, ...)
{
  va_list va;

  if (!logfile) { logfile = stderr; }

  va_start(va, format);
  vfprintf(logfile, format, va);
  va_end(va);
}

/** Log a message following the current logging parameters.
 *
 * Output will be rate-limited, and timestamped (except on stderr).
 * One should however use the convenience functions logerror(), logwarn(),
 * loginfo() and logdebug() for user-facing code.
 *
 * \param level log level for the message
 * \param fmt format string
 * \param ... format string arguments
 *
 * \see logerror, logwarn, loginfo, logdebug
 * \see o_set_log_file, o_set_log_level, o_set_log, o_set_simplified_logging
 * \see o_vlog, o_log_simplified
 */
static void
_o_log_real(int log_level, const char *fmt, ...) {
  va_list va;
  va_start (va, fmt);
  o_vlog (log_level, fmt, va);
  va_end (va);
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
