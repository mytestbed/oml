/*
 * Copyright 2010-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_liboml2_writers.c
 * \brief Test harness for OML client writers.
 */

#define _GNU_SOURCE  /* For NAN */
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <check.h>

#include "config.h"
#include "mbuf.h"
#include "client.h"
#include "oml_utils.h"
#include "file_stream.h"

#ifdef HAVE_LIBZ
# include "zlib_utils.h"
# include "zlib_stream.h"
#endif /* HAVE_LIBZ */

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
  fail_if(fs->os.write==NULL);
  /* Not set nor used at the moment
   *fail_if(fs->os.close==NULL); */

  /* The OmlFileOutStream is buffered by default */
  fail_unless(file_stream_get_buffered(os));

  fail_unless(os->write(NULL, (uint8_t*)buf, 0)==-1);

  /* Buffered operation */
  fail_unless(os->write(os, (uint8_t*)buf, sizeof(buf))==sizeof(buf));

  f = fopen(FN, "r");
  /* Nothing should be read at this time */
  len = fread(buf2, sizeof(char), sizeof(buf2), f);
  fail_unless(len==0, "Read %d bytes from " FN ", expected %d", len, 0);
  fclose(f);

  /* Unbuffered operation */
  file_stream_set_buffered(os, 0);
  fail_if(file_stream_get_buffered(os));
  fail_unless(os->write(os, (uint8_t*)buf, sizeof(buf))==sizeof(buf));

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

#ifdef HAVE_LIBZ

#define ZFN	"test_zw_create_buffered"

START_TEST (test_zw_create_buffered)
{
  int len, len2, acc=0, ret;
  uint8_t buf[512], buf2[512];
  FILE *blob, *f;
  OmlOutStream *os, *fs;
  OmlZlibOutStream *zs;

  /* Remove stray files */
  unlink(ZFN);
  unlink(ZFN ".blob");

  fs = file_stream_new(ZFN);
  os = zlib_stream_new(fs);
  zs = (OmlZlibOutStream*) os;

  fail_unless(zs->outs==fs);
  fail_if(zs->os.write==NULL);
  fail_if(zs->os.close==NULL);
  fail_unless(zs->outs!=os);

  blob = fopen("blob", "r");
  fail_if(blob == NULL, "Failure opening %s", "blob");

#ifndef DUMMY_COMPRESS
  while(!feof(blob)) {
    if((len=fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless(os->write(os, (uint8_t*)buf, len) == len);
    }
  }

#else
  /* Dummy compression to make sure the decompression step works */
  f=fopen(ZFN,"w");
  fail_unless(oml_zlib_def(blob, f, Z_DEFAULT_COMPRESSION) == Z_OK, "Error deflating");
  fclose(f);

#endif /* DUMMY_COMPRESS */

  os->close(os);
  fclose(blob);

  /* Decompress data */
  f = fopen(ZFN, "r");
  blob = fopen(ZFN ".blob", "w");

  /* TODO: read and skip first line */

  /* Skip uncompressed encapsulation header */
  ret = oml_zlib_inf(f,blob);
  fail_unless(Z_DATA_ERROR==ret, "Error inflating " ZFN ": %d", ret);
  /* Inflate the blob */
  ret = oml_zlib_inf(f,blob);
  fail_unless(Z_OK==ret, "Error inflating " ZFN ": %d", ret);

  fclose(f);
  fclose(blob);

  /* Check data in the file */
  blob = fopen("blob", "r");
  f = fopen(ZFN ".blob", "r");
  while(!feof(blob) && !feof(f)) {
    if((len=(int)fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless((len2=fread(buf2, 1, len, f))==len,
          "Read %d bytes from " ZFN ".blob, expected %d", len2, len);
      fail_if(memcmp(buf, buf2, len) != 0,
          "Contents of blob and " ZFN ".blob differ around offset %d", acc);
      acc += len;
    }
  }
  /* Force an EOF detection if pending */
  fread(buf,1,1,blob);
  fread(buf2,1,1,f);
  fail_unless (feof(blob) && feof(f),
      "One of the files is not finished (blob: %d, " ZFN ".blob: %d) after offset %d",
      feof(blob), feof(f), acc);
  fclose(f);
  fclose(blob);
}
END_TEST
#endif /* HAVE_LIBZ */

Suite*
writers_suite (void)
{
  Suite* s = suite_create ("Writers");

  TCase* tc_fw = tcase_create ("FileWriter");
  tcase_add_test (tc_fw, test_fw_create_buffered);
  suite_add_tcase (s, tc_fw);

#ifndef HAVE_LIBZ
# warning "Zlib not found, OmlZlibOutStream not tested"
#else
  TCase* tc_zw = tcase_create ("ZlibWriter");
  tcase_add_test (tc_zw, test_zw_create_buffered);
  suite_add_tcase (s, tc_zw);
#endif /* HAVE_LIBZ */

  return s;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
