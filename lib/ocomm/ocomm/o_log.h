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
/*! \file o_log.h
  \brief Header file for generic log functions
  \author Max Ott (max@winlab.rutgers.edu)
 */


#ifndef O_LOG_H
#define O_LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

#define O_LOG_ERROR -2
#define O_LOG_WARN  -1
#define O_LOG_INFO  0
#define O_LOG_DEBUG 1
#define O_LOG_DEBUG2 2
#define O_LOG_DEBUG3 3
#define O_LOG_DEBUG4 4

/** The o_log_fn function pointer is for format string-based log functions.
 *
 * A typical function will check that the log_level is larger than the
 * o_log_level, then report the message by creating a string from the format.
 * The default is to create the va_list and call o_vlog().
 *
 * \code{.c}
 * if (level > o_log_level) return;
 * va_list va;
 * va_start(va, format);
 * vfprintf(stderr, format, va);
 * va_end(va);
 * \endcode
 *
 * This function will then be used on calls to o_log()
 *
 * \param log_level log level for the message
 * \param format format string
 * \param ... arguments for the format string
 */
typedef void (*o_log_fn)(int log_level, const char* format, ...);
/** The o_vlog_fn function pointer is for vararg-based log functions.
 *
 * A typical function will check that the log_level is larger than the
 * o_log_level, then report the message by creating a string from the format
 * and varargs.
 *
 * \code{.c}
 * if (level > o_log_level) return;
 * vfprintf(stderr, format, va);
 * \endcode
 *
 * This function will then be used on calls to o_vlog()
 *
 * \param log_level log level for the message
 * \param format format string
 * \param va vararg list
 */
typedef void (*o_vlog_fn)(int log_level, const char* format, va_list va);

/** Current logging function.
 * The default function internally parses the format string and arguments as a
 * vararg, and calls the associated o_vlog_fn which does the actual work.
 */
extern o_log_fn o_log;
/** Current vararg logging function.
 * The default function does the actually message outputting job.
 */
extern o_vlog_fn o_vlog;

/** Set the format-based log function if non NULL, or the default one */
o_log_fn o_set_log(o_log_fn log_fn);
/** Set the vararg-based vlog function if non NULL, or the default one */
o_vlog_fn o_set_vlog(o_vlog_fn vlog_fn);

/*! \fn void o_set_log_file(char* name)
  \brief Set the file to send log messages to, '-' for stdout
  \param name Name of logfile
*/
void
o_set_log_file(char* name);

/*! \fn void o_set_log_level(int level)
  \brief Set the level at which to print log message.
  \param level Level at which to start printing log messages
*/
void
o_set_log_level(int level);

void o_set_simplified_logging (void);

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
