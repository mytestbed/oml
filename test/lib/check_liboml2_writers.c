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
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <check.h>

#include "mbuf.h"
#include "client.h"
#include "oml_util.h"

/*
START_TEST (test_bw_create)
{
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
}
END_TEST
*/

/* XXX: Duplicated from lib/client/file_stream.c */
typedef struct _omlFileOutStream {
  oml_outs_write_f write;
  oml_outs_close_f close;
  FILE* f;
} OmlFileOutStream;

#define FN	"test_fw_create_buffered"

START_TEST (test_fw_create_buffered)
{
  char buf[] = "aaa\n";
  char buf2[10];
  int len;
  FILE *f;
  OmlOutStream *os;
  OmlFileOutStream *fs;

  /* Remove stray file */
  unlink(FN);

  os = file_stream_new(FN);
  fs = (OmlFileOutStream*) os;

  fail_if(fs->f==NULL);
  fail_if(fs->write==NULL);
  /* Not set nor used at the moment
   *fail_if(fs->close==NULL); */

  /* The OmlFileOutStream is buffered by default */
  fail_unless(file_stream_get_buffered(os));

  fail_unless(os->write(NULL, (uint8_t*)buf, 0, NULL, 0)==-1);

  /* Buffered operation */
  fail_unless(os->write(os, (uint8_t*)buf, sizeof(buf), NULL, 0)==sizeof(buf));

  f = fopen(FN, "r");
  /* Nothing should be read at this time */
  len = fread(buf2, sizeof(char), sizeof(buf2), f);
  fail_unless(len==0, "Read %d bytes from " FN ", expected %d", len, 0);
  fclose(f);

  /* Unuffered operation */
  file_stream_set_buffered(os, 0);
  fail_if(file_stream_get_buffered(os));
  fail_unless(os->write(os, (uint8_t*)buf, sizeof(buf), NULL, 0)==sizeof(buf));

  f = fopen(FN, "r");
  /* Nothing should be read at this time */
  len=fread(buf2, sizeof(char), sizeof(buf2), f);
  fail_unless(len==2*sizeof(buf), "Read %d bytes from " FN ", expected %d", len, 2*sizeof(buf)-2);
  fclose(f);

  /* Final check */
  file_stream_set_buffered(os, 1);
  fail_unless(file_stream_get_buffered(os));
}
END_TEST

Suite*
writers_suite (void)
{
  Suite* s = suite_create ("Writers");

  /* Test cases */
  /*TCase* tc_bw = tcase_create ("BfWr");*/
  TCase* tc_fw = tcase_create ("FileWr");

  /* Add tests */
  /*tcase_add_test (tc_bw, test_bw_create);*/

  tcase_add_test (tc_fw, test_fw_create_buffered);

  /*suite_add_tcase (s, tc_bw);*/
  suite_add_tcase (s, tc_fw);
  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
