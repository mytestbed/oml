/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
#ifndef CHECK_LIBOML2_SUITES_H__
#define CHECK_LIBOML2_SUITES_H__

#include <check.h>

extern Suite* api_suite (void);
extern Suite* base64_suite (void);
extern Suite* bswap_suite (void);
extern Suite* cbuf_suite (void);
extern Suite* filters_suite (void);
extern Suite* log_suite (void);
extern Suite* mbuf_suite (void);
extern Suite* omlvalue_suite (void);
extern Suite* string_utils_suite (void);
extern Suite* writers_suite (void);

#endif /* CHECK_LIBOML2_SUITES_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
