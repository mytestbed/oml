/*
 * Copyright 2012 National ICT Australia (NICTA), Australia
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
#include <check.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#include "ocomm/o_log.h"

START_TEST (test_log_rate)
{
  int i;
  char *logfile = "check_oml2_log.log";
  struct stat statbuf;

  unlink(logfile);

  o_set_log_file (logfile);

  for (i=0;i<(1<<16);i++) {
    o_log_simplified(O_LOG_ERROR, "Unlimited\n");
    o_log_simplified(O_LOG_ERROR, "Unlimited\n");
    o_log_simplified(O_LOG_ERROR, "Unlimited\n");
  }

  for (i=0;i<(1<<4);i++) {
    usleep(500000);
    o_log_simplified(O_LOG_ERROR, "1/2 second\n");
  }

  for (i=0;i<(1<<2);i++) {
    usleep(2000000);
    o_log_simplified(O_LOG_ERROR, "2 seconds\n");
  }
  
  /* Roughly test that the log is not gigantic (>10k).
   *
   * The log file is line-buffered, so most should already have been written
   * out.
   */
  stat(logfile, &statbuf);
  fail_if(statbuf.st_size > 10000, "Log file too big despite rate limitation");
}
END_TEST

Suite*
log_suite (void)
{
  Suite* s = suite_create ("Log");

  TCase* tc_log = tcase_create ("Log");
  tcase_set_timeout(tc_log, 0);

  tcase_add_test (tc_log, test_log_rate);


  suite_add_tcase (s, tc_log);

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
