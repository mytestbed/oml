/*
 * Copyright 2013 National ICT Australia Limited (NICTA).
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License
 * in LICENSE.TXT or at http://opensource.org/licenses/MIT. By
 * downloading or using this software you accept the terms and the
 * liability disclaimer in the License.
 */

#include "guid.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Lazily opened handle to /dev/random.
 */
static FILE *fp = NULL; 

/**
 * Close random source.
 */
static void
random_close()
{
  if(fp) {
    close(fp);
    fp = NULL;
  }
}

/** Path to random device.
 */
static const char *random_path = "/dev/urandom";

/**
 * Open random source for input.
 * 
 * \return A valid file descriptor.
 */
static FILE*
random_open()
{
  if(!fp) {
    fp = fopen(random_path, "r");
    if(!fp) {
      logerror("Failed to fopen(\"%s\", \"r\"): %s\n", random_path, sys_errlist[errno]);
      exit(EXIT_FAILURE);
    }
    atexit(random_close);
  }
  return fp;
}

/**
 * Initialize a guid to a unique new value.
 *
 * \return in A non-NULL pointer to the oml_guid_t to initialize.
 */
oml_guid_t
omlc_guid_generate()
{
  FILE *fp = random_open();
  oml_guid_t out = OMLC_GUID_NULL;
  while(OMLC_GUID_NULL == out) {
    if(fread(&out, sizeof(out), 1, fp) != 1) {
      logerror("Failed to read from %s: %s\n", random_path, sys_errlist[errno]);
      exit(EXIT_FAILURE);
    }
  }
  return out;
}

/**
 * Convert a guid into a human-readable string.
 *
 * \param in The oml_guid_t to write.
 * \param out A non-NULL pointer to the buffer to write into.
 * \return The number of characters written.
 */
size_t
omlc_guid_to_string(oml_guid_t in, char *out)
{
  assert(in);
  assert(out);
  return sprintf(out, "%" PRIu64, in);
}

/**
 * Convert a human-readable guid string into a guid.
 *
 * \param in The oml_guid_t to write.
 * \param out A non-NULL pointer to the buffer to write into.
 * \return The number of characters read.
 */
ssize_t
omlc_string_to_guid(const char *in, oml_guid_t *out)
{
  assert(in);
  assert(out);
  char *end;
  *out = strtoull(in, &end, 10);
  return end - in;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
