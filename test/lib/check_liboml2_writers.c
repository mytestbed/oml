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

#include "config.h"
#include "mbuf.h"
#include "client.h"
#include "oml_utils.h"
#include "file_stream.h"

#ifdef HAVE_LIBZ
# include <zlib.h>
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

#ifdef HAVE_LIBZ

#define ZFN	"test_zw_create_buffered"

/* def() and inf() adapted from the (public domain) zlib usage example at [0]
 * to use (de/in)flateInit2 with -15 as the windowBits to not include any
 * header/trailer.
 * [0] http://zlib.net/zlib_how.html
 */
#define CHUNK 512

/* Compress from file source to file dest until EOF on source.
   def() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_STREAM_ERROR if an invalid compression
   level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
   version of the library linked do not match, or Z_ERRNO if there is
   an error reading or writing the files. */
int def(FILE *source, FILE *dest, int level)
{
    int ret, flush;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate deflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    ret = deflateInit2(&strm, level, Z_DEFLATED, -15, 8, Z_DEFAULT_STRATEGY);
    if (ret != Z_OK) {
      return ret;
    }

    /* compress until end of file */
    do {

        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)deflateEnd(&strm);
            return Z_ERRNO;
        }
        flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
        strm.next_in = in;

        /* run deflate() on input until output buffer not full, finish
           compression if all of source has been read in */
        do {

            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = deflate(&strm, flush);    /* no bad return value */
            fail_unless(ret != Z_STREAM_ERROR, "Zlib deflate state clobbered");  /* state not clobbered */

           have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)deflateEnd(&strm);
                return Z_ERRNO;
            }

        } while (strm.avail_out == 0);
        fail_unless(strm.avail_in == 0, "Not all input used by the end of def()");     /* all input will be used */

        /* done when last data in file processed */
    } while (flush != Z_FINISH);
    fail_unless(ret == Z_STREAM_END, "Zlib deflate stream not finished");        /* stream will be complete */

    /* clean up and return */
    (void)deflateEnd(&strm);
    return Z_OK;
}

/* Decompress from file source to file dest until stream ends or EOF.
   inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
   allocated for processing, Z_DATA_ERROR if the deflate data is
   invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
   the version of the library linked do not match, or Z_ERRNO if there
   is an error reading or writing the files. */
int inf(FILE *source, FILE *dest)
{
    int ret;
    unsigned have;
    z_stream strm;
    unsigned char in[CHUNK];
    unsigned char out[CHUNK];

    /* allocate inflate state */
    strm.zalloc = Z_NULL;
    strm.zfree = Z_NULL;
    strm.opaque = Z_NULL;
    strm.avail_in = 0;
    strm.next_in = Z_NULL;
    ret = inflateInit2(&strm, -15);
    if (ret != Z_OK)
        return ret;

    /* decompress until deflate stream ends or end of file */
    do {

        strm.avail_in = fread(in, 1, CHUNK, source);
        if (ferror(source)) {
            (void)inflateEnd(&strm);
            return Z_ERRNO;
        }
        if (strm.avail_in == 0)
            break;
        strm.next_in = in;

        /* run inflate() on input until output buffer not full */
        do {

            strm.avail_out = CHUNK;
            strm.next_out = out;

            ret = inflate(&strm, Z_NO_FLUSH);
            fail_unless(ret != Z_STREAM_ERROR, "Zlib inflate state clobbered");  /* state not clobbered */
            switch (ret) {
            case Z_NEED_DICT:
                ret = Z_DATA_ERROR;     /* and fall through */
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                (void)inflateEnd(&strm);
                return ret;
            }



            have = CHUNK - strm.avail_out;
            if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
                (void)inflateEnd(&strm);
                return Z_ERRNO;
            }


        } while (strm.avail_out == 0);

        /* done when inflate() says it's done */
    } while (ret != Z_STREAM_END);


    /* clean up and return */
    (void)inflateEnd(&strm);
    return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

START_TEST (test_zw_create_buffered)
{
  int len, len2, acc=0;
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

  fail_unless(zs->os==fs);
  fail_if(zs->write==NULL);
  fail_if(zs->close==NULL);
  fail_unless(zs->os!=os);

  blob = fopen("blob", "r");
  fail_if(blob == NULL, "Failure opening %s", "blob");

  /*
  while(!feof(blob)) {
    if((len=fread(buf, 1, sizeof(buf), blob))>0) {
      fail_unless(os->write(os, (uint8_t*)buf, len, NULL, 0));
    }
  }
  os->close(os);
  */

  /* Dummy compression to make sure the decompression step works */
  f=fopen(ZFN,"w");
  fail_unless(def(blob, f, Z_DEFAULT_COMPRESSION) == Z_OK, "Error deflating");
  fclose(f);

  fclose(blob);

  /* Decompress data */
  f = fopen(ZFN, "r");
  blob = fopen(ZFN ".blob", "w");

  fail_unless(inf(f,blob)==Z_OK, "Error inflating " ZFN);

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
