/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file misc.c
 * \brief Implements various common utility functions (XXX: Only mutex lock/unlock).
 */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>

#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "client.h"

/** Lock a measurement point mutex
 * \param mp OmlMP to lock
 * \return 0 if successful, -1 otherwise
 * \see mp_unlock, oml_lock
 */
int
mp_lock(OmlMP* mp)
{
  return oml_lock(mp->mutexP, mp->name);
}

/** Unlock a measurement point mutex
 * \param mp OmlMP to unlock
 * \see mp_lock, oml_unlock
 */
void
mp_unlock(OmlMP* mp)
{
  oml_unlock(mp->mutexP, mp->name);
}

/** Lock a mutex
 * \param mutexP pointer to mutex lock
 * \param mutexName name of mutex; used for error reporting only
 * \return 0 if successful, -1 otherwise
 * \see oml_unlock
 */
int
oml_lock(pthread_mutex_t* mutexP, const char* mutexName)
{
  if (mutexP) {
    if (pthread_mutex_lock(mutexP)) {
      logwarn("%s: Couldn't get mutex lock (%s)\n",
          mutexName, strerror(errno));
      return -1;
    }
  }
  return 0;
}

/** Unock a mutex
 * \param mutexP pointer to mutex to unlock
 * \param mutexName name of mutex; used for error reporting only
 * \see oml_lock
 */
void
oml_unlock(pthread_mutex_t* mutexP, const char* mutexName)
{
  if (mutexP) {
    if (pthread_mutex_unlock(mutexP)) {
      logwarn("%s: Couldn't unlock mutex (%s)\n",
          mutexName, strerror(errno));
    }
  }
}

/** Try to obtain a lock on the BufferedWriter until it succeeds
 * \param mutexP pointer to mutex to unlock
 * \param mutexName name of mutex; used for error reporting only
 * \see oml_lock, oml_unlock
 */
void
oml_lock_persistent(pthread_mutex_t* mutexP, const char* mutexName)
{
  while (oml_lock(mutexP, mutexName)) {
    logwarn("Cannot get lock in %s; will try again soon.\n", mutexName);
    sleep(1);
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
