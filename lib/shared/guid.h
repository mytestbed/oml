/*
 * Copyright 2013-2014 National ICT Australia Limited (NICTA).
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License
 * in LICENSE.TXT or at http://opensource.org/licenses/MIT. By
 * downloading or using this software you accept the terms and the
 * liability disclaimer in the License.
 */

#ifndef OML_GUID_H
#define OML_GUID_H

#include <stdint.h>
#include <sys/types.h>

#include "oml2/omlc.h"

extern oml_guid_t
omlc_guid_generate();

#define MAX_GUID_STRING_SZ 21

extern size_t
omlc_guid_to_string(oml_guid_t in, char *out);

extern ssize_t
omlc_string_to_guid(const char *in, oml_guid_t *out);
  
#endif /* OML_GUID_H */
