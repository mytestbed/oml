/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
#include <stdlib.h>
#include <check.h>

#include "ocomm/o_log.h"
#include "check_liboml2_suites.h"

int
main (void)
{
  int number_failed = 0;

  o_set_log_file ("check_liboml2_oml.log");
  SRunner* sr = srunner_create (omlvalue_suite());
  srunner_add_suite (sr, base64_suite ());
  srunner_add_suite (sr, bswap_suite ());
  srunner_add_suite (sr, mbuf_suite ());
  srunner_add_suite (sr, cbuf_suite ());
  srunner_add_suite (sr, api_suite ());
  srunner_add_suite (sr, writers_suite ());
  srunner_add_suite (sr, filters_suite ());
  /* The log_suite has to be last, lest it messes up logging for suites
   * following it */
  srunner_add_suite (sr, log_suite ());
  srunner_add_suite (sr, string_utils_suite ());

  srunner_run_all (sr, CK_ENV);
  number_failed += srunner_ntests_failed (sr);
  srunner_free (sr);
  return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
