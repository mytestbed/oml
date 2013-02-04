/*
 * Copyright 2013 National ICT Australia Limited (NICTA).
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License
 * in LICENSE.TXT or at http://opensource.org/licenses/MIT. By
 * downloading or using this software you accept the terms and the
 * liability disclaimer in the License.
 */

#ifndef BASE64_H
#define BASE64_H

#include <stddef.h>
#include <sys/types.h>

extern size_t
base64_size_string(size_t blob_sz);

extern size_t
base64_encode_blob(size_t blob_sz, const void *blob, char *s);

extern ssize_t
base64_validate_string(const char *s);

extern size_t
base64_size_blob(size_t s_sz);

extern ssize_t
base64_decode_string(size_t s_sz, const char *s, size_t blob_sz, void *blob);

#endif /* BASE64_H */

