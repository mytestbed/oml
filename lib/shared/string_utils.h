/*
 * Copyright 2013 NICTA
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license. You should have received a copy of this license in
 * the file LICENSE.TXT or you can retrieve a copy at
 * http://opensource.org/licenses/MIT. If you require a license of
 * this Software, you must read and accept the terms and the liability
 * disclaimer in the above file or link.
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
