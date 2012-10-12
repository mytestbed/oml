/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
#include <stdlib.h>
#include <check.h>
#include "check_liboml2_suites.h"
#include "ocomm/o_log.h"

int
main (void)
{
  int number_failed = 0;

  o_set_log_file ("check_oml2.log");
  SRunner* sr = srunner_create (filters_suite ());
  srunner_add_suite (sr, util_suite ());
  srunner_add_suite (sr, log_suite ());
  srunner_add_suite (sr, api_suite ());
  srunner_add_suite (sr, parser_suite ());
  srunner_add_suite (sr, bswap_suite ());
  srunner_add_suite (sr, marshal_suite ());
  srunner_add_suite (sr, mbuf_suite ());
  srunner_add_suite (sr, cbuf_suite ());
  srunner_add_suite (sr, writers_suite ());

  srunner_run_all (sr, CK_NORMAL);
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
