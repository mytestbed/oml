/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file check_input_filters.c
 * \brief Tests behaviour and issues related to the server's input filters
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */

#include "config.h"
#define _GNU_SOURCE  /* For NAN */
#include <check.h>
#include <string.h>
#ifdef HAVE_LIBZ
# include <zlib.h>
# include "zlib_utils.h"
#endif

#include "ocomm/o_log.h"
#include "oml_utils.h"
#include "mbuf.h"
#include "input_filter.h"
#include "client_handler.h"
#include "check_server.h"

START_TEST(test_input_filters)
{
  ssize_t ret;
  ClientHandler ch;
  InputFilter *ifl;
  MBuffer *mbuf;

  memset(&ch, 0, sizeof(ch));
  memcpy(&ch.name, __FUNCTION__, sizeof(__FUNCTION__));

  mbuf = mbuf_create();
  ck_assert_msg(NULL != mbuf, "error allocating MBuffer");

  mbuf_write(mbuf, (const unsigned char*)__FUNCTION__, sizeof(__FUNCTION__));

  ifl = input_filter_create("null", &ch);
  ck_assert_msg(NULL != ifl, "The null InputFilter wasn't created");

  ck_assert_msg(-1 == (ret = input_filter_in(ifl, mbuf)),
      "The null input filter did not \"generate\" the right amount of data (%d rather than -1)", ret);
  ck_assert_msg(-1 == input_filter_in(NULL, mbuf),
      "The null input filter input did not return -1 on NULL InputFilter");
  ck_assert_msg(-1 == input_filter_in(ifl, NULL),
      "The null input filter input did not return -1 on NULL MBuffer");

  ck_assert_msg(-1 == (ret = input_filter_out(ifl, mbuf)),
      "The null input filter did not \"output\" the right amount of data (%d rather than -1)", ret);
  ck_assert_msg(-1 == input_filter_out(NULL, mbuf),
      "The null input filter output did not return -1 on NULL InputFilter");
  ck_assert_msg(-1 == input_filter_out(ifl, NULL),
      "The null input filter output did not return -1 on NULL MBuffer");

  input_filter_destroy(ifl);
  mbuf_destroy(mbuf);
}
END_TEST

#ifdef HAVE_LIBZ

START_TEST(test_gzip)
{
  size_t occupancy;
  ClientHandler ch;
  InputFilter *ifl;
  MBuffer *in, *out;
  FILE *blobgz, *blob;
  unsigned char buf[512];
  ssize_t len = 0, firstpass = 1, bloblen = 0, acc = 0;

  memset(&ch, 0, sizeof(ch));
  memcpy(&ch.name, __FUNCTION__, sizeof(__FUNCTION__));

  in = mbuf_create();
  ck_assert_msg(NULL != in, "error allocating input MBuffer");
  out = mbuf_create();
  ck_assert_msg(NULL != out, "error allocatoutg output MBuffer");

  ifl = input_filter_create("gzip", &ch);
  ck_assert_msg(NULL != ifl,
      "The gzip InputFilter wasn't created");

  blobgz = fopen("blob.gz", "r");

  do {
    len = fread(buf, 1, sizeof(buf), blobgz);
    if (firstpass) {
      firstpass = 0;
      logdebug3("%s: begining of data\n%s\n", __FUNCTION__, to_octets(buf, len));
    }
    mbuf_write(in, buf, len);
    logdebug3("wrote %dB of data into input mbuffer\n", len);
  } while(!feof(blobgz));
  fclose(blobgz);

  /* Input compressed data into the filter */
  occupancy = mbuf_rd_remaining(in);
  ck_assert_msg(0 <= input_filter_in(ifl, in),
      "The gzip input filter did not generate data");
  ck_assert_msg(mbuf_rd_remaining(in) < occupancy,
      "No data was read from the input buffer");

  /* Read uncompressed data from the filter */
  occupancy = mbuf_rd_remaining(out);
  ck_assert_msg(0 <=  input_filter_out(ifl, out),
      "The gzip input filter did not output data");
  ck_assert_msg(mbuf_rd_remaining(out) > occupancy,
      "No data was written into the output buffer");

  /* Check data in the file */
  blob = fopen("blob", "r");
  while(!feof(blob) &&
      (bloblen = mbuf_rd_remaining(out))) {
    if((len=(int)fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless(bloblen >= len,
          "Read %d bytes from blob, but only expected %d around offset %d",
          len, bloblen, acc);
      fail_if(memcmp(buf, mbuf_rdptr(out), len) != 0,
          "Contents of blob and inflated data differ around offset %d", acc);
      mbuf_read_skip(out, len);
      acc += len;
    }
  }
  /* Force an EOF detection if pending */
  len=fread(buf,1,1,blob);
  fail_unless(feof(blob) && mbuf_rd_remaining(out) == 0,
      "One of the files is not finished (blob: %d, inflated: %d) after offset %d",
      feof(blob), mbuf_rd_remaining(out), acc);
  fclose(blob);

  input_filter_destroy(ifl);
  mbuf_destroy(in);
  mbuf_destroy(out);
}
END_TEST
#endif

Suite*
input_filter_suite(void)
{
  Suite* s = suite_create ("Input filters");

  o_set_log_level(-1);

  TCase* tc_input_filters = tcase_create ("InputFilter");
  tcase_add_test (tc_input_filters, test_input_filters);
  suite_add_tcase (s, tc_input_filters);

#ifdef HAVE_LIBZ
  TCase* tc_test_gzip = tcase_create ("GZIP InputFilter");
  tcase_add_test (tc_test_gzip, test_gzip);
  suite_add_tcase (s, tc_test_gzip);
#endif

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
