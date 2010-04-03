/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
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
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <check.h>
#include <mbuf.h>

#include "util.h"

START_TEST (test_bw_create)
{
  /*
  MBuffer* mbuf = mbuf_create ();

  fail_if (mbuf == NULL);
  fail_if (mbuf->base == NULL);
  fail_if (mbuf->rdptr != mbuf->base);
  fail_if (mbuf->wrptr != mbuf->base);
  fail_if (mbuf->fill != 0);
  fail_unless (mbuf->length > 0);
  fail_if (mbuf->wr_remaining != mbuf->length);
  fail_if ((int)mbuf->rd_remaining != (mbuf->wrptr - mbuf->rdptr));
  unsigned int i;
  for (i = 0; i < mbuf->length; i++)
	fail_if (mbuf->base[i] != 0);
  */
}
END_TEST



Suite*
bw_suite (void)
{
  Suite* s = suite_create ("BufferedWriter");

  /* Mbuf test cases */
  TCase* tc_bw = tcase_create ("BfWr");

  /* Add tests to "BfWr" */
  tcase_add_test (tc_bw, test_bw_create);


  suite_add_tcase (s, tc_bw);
  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
