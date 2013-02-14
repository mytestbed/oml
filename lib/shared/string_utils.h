/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

extern size_t
backslash_encode_size(size_t in_sz);

extern size_t
backslash_encode(const char *in, char *out);

extern size_t
backslash_decode(const char *in, char *out);

#endif /* STRING_UTILS_H */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
