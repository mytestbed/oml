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

#define _GNU_SOURCE  /* For NAN */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <check.h>

#include "mbuf.h"
#include "client.h"
#include "oml_utils.h"
#include "file_stream.h"
#include "zlib_stream.h"

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

  /* Unbuffered operation */
  file_stream_set_buffered(os, 0);
  fail_if(file_stream_get_buffered(os));
  fail_unless(os->write(os, (uint8_t*)buf, sizeof(buf), NULL, 0)==sizeof(buf));

  f = fopen(FN, "r");
  /* Now there should be data */
  len=fread(buf2, sizeof(char), sizeof(buf2), f);
  fail_unless(len==2*sizeof(buf), "Read %d bytes from " FN ", expected %d", len, 2*sizeof(buf)-2);
  fclose(f);

  /* Final check */
  file_stream_set_buffered(os, 1);
  fail_unless(file_stream_get_buffered(os));
}
END_TEST

#define ZFN	"test_zw_create_buffered"

START_TEST (test_zw_create_buffered)
{
  int len, len2, acc=0;
  char buf[512], buf2[512];
  FILE *blob, *f;
  OmlOutStream *os, *fs;
  OmlZlibOutStream *zs;

  /* Remove stray file */
  unlink(ZFN);

  fs = file_stream_new(ZFN);
  os = zlib_stream_new(fs);
  zs = (OmlZlibOutStream*) os;

  fail_unless(zs->os==fs);
  fail_if(zs->write==NULL);
  fail_if(zs->close==NULL);
  fail_unless(zs->os!=os);

  blob = fopen("blob", "r");
  fail_if(blob == NULL, "Failure opening %s", "blob");

  while(!feof(blob)) {
    if((len=fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless(os->write(os, (uint8_t*)buf, len, NULL, 0));
    }
  }

  fclose(blob);
  os->close(os);

  /* Check data in the file */
  blob = fopen("blob", "r");
  f = fopen(ZFN, "r");
  while(!feof(blob) && !feof(f)) {
    if((len=(int)fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless((len2=fread(buf2, 1, len, f))==len,
          "Read %d bytes from " ZFN ", expected %d", len2, len);
      fail_if(memcmp(buf, buf2, len) != 0,
          "Contents of blob and " ZFN " differ around offset %d", acc);
      acc += len;
    }
  }
  /* Force an EOF detection if pending */
  fread(buf,1,1,blob);
  fread(buf2,1,1,f);
  fail_unless (feof(blob) && feof(f),
      "One of the files is not finished (blob: %d, " ZFN ": %d) after offset %d",
      feof(blob), feof(f), acc);
  fclose(f);
  fclose(blob);
}
END_TEST

Suite*
writers_suite (void)
{
  Suite* s = suite_create ("Writers");

  /* Test cases */
  TCase* tc_fw = tcase_create ("FileWriter");
  TCase* tc_zw = tcase_create ("ZlibWriter");

  /* Add tests */
  tcase_add_test (tc_fw, test_fw_create_buffered);
  tcase_add_test (tc_zw, test_zw_create_buffered);

  suite_add_tcase (s, tc_fw);
  suite_add_tcase (s, tc_zw);
  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
