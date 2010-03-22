/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
/*!\file misc.c
  \brief Implements various common utility functions
*/

#include <stdlib.h>
#include <pthread.h>
#include <ocomm/o_log.h>
#include <errno.h>
#include <string.h>
#include <oml2/omlc.h>
#include "client.h"

/**
 * \brief lock of the measurement point mutex
 * \param mp the measurement point
 * \return 0 if successful, -1 otherwise
 */
int
mp_lock(
  OmlMP* mp
) {
  if (mp->mutexP) {
    if (pthread_mutex_lock(mp->mutexP)) {
      o_log(O_LOG_WARN, "%s: Couldn't get mutex lock (%s)\n",
          mp->name, strerror(errno));
      return -1;
    }
  }
  return 0;
}
/**
 * \brief unlock of the measurement point mutex
 * \param mp the measurement point
 */
void
mp_unlock(
  OmlMP* mp
) {
  if (mp->mutexP) {
    if (pthread_mutex_unlock(mp->mutexP)) {
      o_log(O_LOG_WARN, "%s: Couldn't unlock mutex (%s)\n",
          mp->name, strerror(errno));
    }
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
