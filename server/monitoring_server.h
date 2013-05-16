/*
 * Copyright 2013 National ICT Australia (NICTA), Australia
 *
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#ifndef MONITORING_SERVER_H_
#define MONITORING_SERVER_H_

/**
 * Set up link to a monitoring OML server.
 * 
 * \param argc A non-NULL pointer to the argument count.
 * \param argv The argument vector.
 */
void ms_setup(int *argc, const char **argv);

/**
 * Clean up link to a monitoring OML server.
 */
void ms_cleanup(void);

/**
 * Inject a measurement into the monitoring OML server.
 *
 * \param address A pointer to the client IP address string.
 * \param port    The client port number.
 * \param oml_id  Pointer to a string containing the client OML ID.
 * \param domain  Pointer to a string containing the client domain.
 * \param appname Pointer to a string containing the client appname.
 * \param timestamp Timestamp.
 * \param event   (possibly NULL) pointer to an event string.
 * \param message (possibly NULL) pointer to a message string.
 */
void ms_inject(
  const char* address,
  uint32_t port,
  const char* oml_id,
  const char* domain,
  const char* appname,
  uint64_t timestamp,
  const char* event,
  const char* message);

#endif /*MONITORING_SERVER_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
