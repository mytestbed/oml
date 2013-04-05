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
 * \brief Logging functions prototypes.
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
 *
 * \see o_set_log
 */
typedef void (*o_log_fn)(int log_level, const char* format, ...);

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
 */
void o_log(int level, const char* fmt, ...);

/** Direct the log stream to the named file.
 * \param name name of the file to write log into (if '-' or NULL, defaults to stderr)
 */
void o_set_log_file(char* name);

/** Set the log level below which messages should be displayed.
 * \param level log level
 * \see o_log, o_log_level
 */
void o_set_log_level(int level);

/** Test if a given log level is currently active
 * \param level log level
 * \return 0 if inactive, 1 otherwise
 * \see o_set_log_level
 */
int o_log_level_active(int log_level);

/** Set the format string-based log function if non NULL, or o_log_simplified
 * \param log_fn function to use for logging
 * \return the current logging function
 */
o_log_fn o_set_log(o_log_fn log_fn);

/** Reset the logging function to the internal default */
void o_set_simplified_logging (void);

/** Convenience function logging at level O_LOG_ERROR. */
void logerror (const char *fmt, ...);
/** Convenience function logging at level O_LOG_WARN. */
void logwarn (const char *fmt, ...);
/** Convenience function logging at level O_LOG_INFO. */
void loginfo (const char *fmt, ...);
/** Convenience function logging at level O_LOG_DEBUG. */
void logdebug (const char *fmt, ...);

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
