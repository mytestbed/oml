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
#include <fcntl.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * Initialize a guid to a unique new value.
 *
 * \return in A non-NULL pointer to the guid_t to initialize.
 */
guid_t
omlc_guid_generate()
{
  guid_t out = OMLC_GUID_NULL;
  int fd = open("/dev/urandom", O_RDONLY);
  if(-1 == fd) {
    perror("open(\"/dev/urandom\", O_RDONLY)");
    exit(EXIT_FAILURE);
  }
  while(OMLC_GUID_NULL == out) {
    if(read(fd, &out, sizeof(out)) == -1) {
      perror("read(fd, in, sizeof(out))");
      exit(EXIT_FAILURE);
    }
  }
  close(fd);
  return out;
}

/**
 * Convert a guid into a human-readable string.
 *
 * Note that the external representation is not guaranteed portable
 * across releases. Both the internal form of a guid and the
 * external string representations are likely to change.
 *
 * \param in The guid_t to write.
 * \param out A non-NULL pointer to the buffer to write into.
 * \return The number of characters written.
 */
size_t
omlc_guid_to_string(guid_t in, char *out)
{
  assert(in);
  assert(out);
  return sprintf(out, "%" PRIu64, in);
}

/**
 * Convert a human-readable guid string into a guid.
 *
 * Note that the external representation is not guaranteed portable
 * across releases. Both the internal form of a guid and the
 * external string representations are likely to change.
 *
 * \param in The guid_t to write.
 * \param out A non-NULL pointer to the buffer to write into.
 * \return The number of characters read.
 */
ssize_t
omlc_string_to_guid(const char *in, guid_t *out)
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
