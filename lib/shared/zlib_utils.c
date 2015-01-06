/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file zlib_utils.h
 * \brief Zlib helpers for OML
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 * \author Mark Adler (zlib usage example)
 *
 * oml_zlib_def() and oml_zlib_inf() were adapted from the (public domain) zlib
 * usage example at [0] to use (de/in)flateInit2 with OML_ZLIB_WINDOWBITS  as
 * the windowBits to parametrise header/trailer addition.
 *
 * [0] http://zlib.net/zlib_how.html
 *
 * \see OmlZlibOutStream;
 */
#include <stdio.h>
#include <zlib.h>

#include "ocomm/o_log.h"
#include "zlib_utils.h"

/** Compress from file source to file dest until EOF on source.
 *
 * \param source FILE pointer to the undcompressed source stream
 * \param dest FILE pointer to the compressed destination stream
 *
 * \return Z_OK on success, Z_MEM_ERROR if memory could not be
 * allocated for processing, Z_STREAM_ERROR if an invalid compression
 * level is supplied, Z_VERSION_ERROR if the version of zlib.h and the
 * version of the library linked do not match, or Z_ERRNO if there is
 * an error reading or writing the files.
 *
 * \see deflate
 */
int
oml_zlib_def(FILE *source, FILE *dest, int level)
{
  int ret, flush;
  unsigned have;
  z_stream strm;
  unsigned char in[OML_ZLIB_CHUNKSIZE];
  unsigned char out[OML_ZLIB_CHUNKSIZE];

  /* allocate deflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  ret = deflateInit2(&strm, level, Z_DEFLATED, OML_ZLIB_WINDOWBITS, 8, Z_DEFAULT_STRATEGY);
  if (ret != Z_OK) {
    return ret;
  }

  /* compress until end of file */
  do {

    strm.avail_in = fread(in, 1, OML_ZLIB_CHUNKSIZE, source);
    if (ferror(source)) {
      (void)deflateEnd(&strm);
      return Z_ERRNO;
    }
    flush = feof(source) ? Z_FINISH : Z_NO_FLUSH;
    strm.next_in = in;

    /* run deflate() on input until output buffer not full, finish
       compression if all of source has been read in */
    do {

      strm.avail_out = OML_ZLIB_CHUNKSIZE;
      strm.next_out = out;

      ret = deflate(&strm, flush);    /* no bad return value */
      if(ret == Z_STREAM_ERROR) {
        logerror("Zlib deflate state clobbered\n");  /* state not clobbered */
        return ret;
      }

      have = OML_ZLIB_CHUNKSIZE - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)deflateEnd(&strm);
        return Z_ERRNO;
      }

    } while (strm.avail_out == 0);

    if(strm.avail_in != 0) {     /* all input will be used */
      logerror("Not all input used by the end of %s\n", __FUNCTION__);
      return Z_STREAM_ERROR;
    }

    /* done when last data in file processed */
  } while (flush != Z_FINISH);
  if(ret != Z_STREAM_END) {        /* stream will be complete */
    logerror("Zlib deflate stream not finished\n");
    return Z_STREAM_ERROR;
  }

  /* clean up and return */
  (void)deflateEnd(&strm);
  return Z_OK;
}

/** Decompress from file source to file dest until stream ends or EOF.
 *
 * \param source FILE pointer to the compressed source stream
 * \param dest FILE pointer to the uncompressed destination stream
 *
 * \return Z_OK on success, Z_MEM_ERROR if memory could not be
 * allocated for processing, Z_DATA_ERROR if the deflate data is
 * invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
 * the version of the library linked do not match, or Z_ERRNO if there
 * is an error reading or writing the files.
 *
 * \see inflate
 */
int
oml_zlib_inf(FILE *source, FILE *dest)
{
  int ret;
  unsigned have;
  z_stream strm;
  unsigned char in[OML_ZLIB_CHUNKSIZE];
  unsigned char out[OML_ZLIB_CHUNKSIZE];

  /* allocate inflate state */
  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit2(&strm, OML_ZLIB_WINDOWBITS);
  if (ret != Z_OK) {
    return ret;
  }

  /* decompress until deflate stream ends or end of file */
  do {

    strm.avail_in = fread(in, 1, OML_ZLIB_CHUNKSIZE, source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0)
      break;
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {

      strm.avail_out = OML_ZLIB_CHUNKSIZE;
      strm.next_out = out;

      ret = inflate(&strm, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR) {  /* state not clobbered */
        logerror("Zlib inflate state clobbered\n");
        return Z_STREAM_ERROR;
      }
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return ret;
      }



      have = OML_ZLIB_CHUNKSIZE - strm.avail_out;
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

/*
  Local Variables:
  mode: C
  tab-width: 2
  indent-tabs-mode: nil
  vim: sw=2:sts=2:expandtab
*/
