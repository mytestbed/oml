/*! \file o_log.h
  \brief Header file for generic log functions
  \author Max Ott (max@winlab.rutgers.edu)
 */


#ifndef O_LOG_H
#define O_LOG_H

#define O_LOG_ERROR -2
#define O_LOG_WARN  -1
#define O_LOG_INFO  0
#define O_LOG_DEBUG 1
#define O_LOG_DEBUG2 2
#define O_LOG_DEBUG3 3
#define O_LOG_DEBUG4 4

typedef void (*o_log_fn)(int log_level, const char* format, ...); 
extern o_log_fn o_log;

/*! \fn o_log_fn o_set_log(o_log_fn log_fn)
  \brief Set the log function, or if NULL return the default function.
  \param log_fn Function to use for logging
*/
o_log_fn 
o_set_log(o_log_fn log_fn);

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

#endif
