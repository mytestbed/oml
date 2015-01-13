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
#define _GNU_SOURCE /* For memmem(3) */
#include <stdio.h>
#include <stddef.h> /* ptrdiff_t is in there */
#include <unistd.h>
#include <inttypes.h>
#include <string.h>
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
  int resynced = 0;
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

  /* run inflate() on input until output buffer not full */
  do {

    strm.avail_in = fread(in, 1, OML_ZLIB_CHUNKSIZE, source);
    if (ferror(source)) {
      (void)inflateEnd(&strm);
      return Z_ERRNO;
    }
    if (strm.avail_in == 0) {
      break;
    }
    strm.next_in = in;

    /* run inflate() on input until output buffer not full */
    do {

      if(!resynced) {
        strm.avail_out = OML_ZLIB_CHUNKSIZE;
        strm.next_out = out;
      }

      ret = inflate(&strm, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR) {  /* state not clobbered */
        logerror("Zlib inflate state clobbered\n");
        return Z_STREAM_ERROR;
      }
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
        if (resynced == 1) {
          resynced = 2;

        } else if(oml_zlib_sync(&strm) == Z_OK) {
          resynced = 1;
        }
        break;
      case Z_MEM_ERROR:
        (void)inflateEnd(&strm);
        return ret;
      }

      have = OML_ZLIB_CHUNKSIZE - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        (void)inflateEnd(&strm);
        return Z_ERRNO;
      } else if (have > 0) {
        resynced = 0;
      }

    } while (strm.avail_out == 0 || resynced);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

  if(strm.avail_in > 0) {
    /* If same data has not been consumed, reset the file to there */
    /* \bug fseeking back to the end of the Gzip stream will not work on stdin */
    fseek(source, ftell(source)-(long)strm.avail_in, SEEK_SET);
  }

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

/** Search for the next block or new GZip header, whichever comes first, and advance the string.
 *
 * This function looks as far as strm->avail_in, and overwrites strm->total_in
 * with the number of bits to skip to the first, if found. It should be usable
 * as a drop-in replacement for Zlib's inflateSync.
 *
 * XXX: There might be false positives. As for inflateSync, trying to inflate
 * from the new block might return a Z_STREAM_ERROR due to a CRC32 mismatch,
 * but the data would have been decompressed properly nonetheless [0].
 *
 * [0] http://newsgroups.derkeiler.com/Archive/Comp/comp.compression/2006-02/msg00327.html
 *
 * TODO: If a new header is found, the state should be reset.
 *
 * \param strm pointer to zlib's z_stream
 *
 * \return Z_OK if a possible full flush point has been found, Z_BUF_ERROR if
 * no more input was provided, Z_DATA_ERROR if no flush point has been found,
 * or Z_STREAM_ERROR if the stream structure was inconsistent.Z_OK if a
 * potential sync point has been found, Z_DATA_ERROR otherwise.
 *
 * \see oml_zlib_find_sync, inflateSync
 */
ptrdiff_t
oml_zlib_sync(z_streamp strm)
{
  int ret = Z_DATA_ERROR;
  unsigned long in, out;
  ptrdiff_t offset = -1;
  size_t len;

  /* From the code of inflateSync() */
  if (strm == Z_NULL || strm->state == Z_NULL) { return Z_STREAM_ERROR; }
  if (strm->avail_in == 0) { return Z_BUF_ERROR; }

  /** \bug we should look in the state bits (in strm->state->hold), but this
   * requires a lot of assumptions to be imported about the internal state of
   * Zlib */

  len = strm->avail_in-strm->total_in;

  offset = oml_zlib_find_sync((const char*)(strm->next_in), len);

  if(offset >= 0 && (unsigned)offset<=len) {
    strm->total_in += offset;
    strm->next_in += offset;
    strm->avail_in -= offset;
    ret = Z_OK;
    in = strm->total_in;  out = strm->total_out;
    inflateReset(strm);
    strm->total_in = in;  strm->total_out = out;
    ret = Z_OK;
  }

  return ret;
}

/** Search for the next block or new GZip header, whichever comes first.
 *
 * Looks for a GZip block or new header in buf, up to len bytes.
 *
 * \warning This function stops after len bytes, it doesn't look for nul bytes.
 *
 * The two markers looked for are:
 * - GZip header: 0x8b1f
 * - Block header: 0x0000ffff
 *
 * \param buf buffer of length (at least) len
 *
 * \return an offset to the first marker, if found, or -1 otherwise.
 */
ptrdiff_t
oml_zlib_find_sync(const char *buf, size_t len)
{
  ptrdiff_t offset = -1;
  static char gziphdr[] = { 0x1f, 0x8b };
  static char blockhdr[] = { 0x00, 0x00, 0xff, 0xff};
  const char *gziphdrptr = NULL, *blockhdrptr = NULL;


  /** \bug Header marker search is not the most efficient.
   * \todo We need someting like find_charn_2 looking for the first of two markers, lather, and repeat. */
  if (len >= 2) { gziphdrptr = memmem(buf, len, gziphdr, sizeof(gziphdr)); }
  if (len >= 4) { blockhdrptr = memmem(buf, len, blockhdr, sizeof(blockhdr)); }

  if (blockhdrptr && !gziphdrptr) {
    offset = (ptrdiff_t)(blockhdrptr - buf);

  } else if (gziphdrptr && !blockhdrptr) {
    offset = (ptrdiff_t)(gziphdrptr - buf);

  } else if (blockhdrptr && blockhdrptr < gziphdrptr) {
    offset = (ptrdiff_t)(blockhdrptr - buf);

  } else if (gziphdrptr && gziphdrptr < blockhdrptr ) {
    offset = (ptrdiff_t)(gziphdrptr - buf);

  }

  return offset;
}

/*
  Local Variables:
  mode: C
  tab-width: 2
  indent-tabs-mode: nil
  vim: sw=2:sts=2:expandtab
*/
