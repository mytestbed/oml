/*
 * Copyright 2015-2016 National ICT Australia (NICTA)
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

#include "mem.h"
#include "mbuf.h"
#include "oml_utils.h"
#include "zlib_utils.h"
#include "ocomm/o_log.h"

/** Memory allocation function for Zlib
 * \see oml_malloc, alloc_func
 */
static void*
oml_zalloc (void* opaque, unsigned int items, unsigned int size)
{
  (void)opaque;
  return oml_malloc(items*size);
}

/** Memory liberation function for Zlib
 * \see oml_free, free_func
 */
static void
oml_zfree (void* opaque, void* address)
{
  (void)opaque;
  oml_free(address);
}

/** Initialise a Zlib stream
 * \param strm pointer to the Zlib stream to initialise
 * \param mode the mode of operation (deflate/inflate)
 * \param level compression level 0--9, or -1 for default (only for OML_ZLIB_DEFLATE)
 * \return 0 on success, <0 on error, as given by deflateInit, inflateInit
 * \see deflateInit, inflateInit
 */
int
oml_zlib_init(z_streamp strm, OmlZlibMode mode, int level)
{
  int ret = Z_STREAM_ERROR;

  if(strm) {
    strm->zalloc = oml_zalloc;
    strm->zfree = oml_zfree;
    strm->opaque = Z_NULL;
    strm->avail_in = 0;
    strm->next_in = Z_NULL;
    strm->avail_out = 0;
    strm->next_out = Z_NULL;

    switch (mode) {
    case OML_ZLIB_DEFLATE:
      ret = deflateInit2(strm, level, Z_DEFLATED, OML_ZLIB_WINDOWBITS, 8, Z_DEFAULT_STRATEGY);
      break;
    case OML_ZLIB_INFLATE:
      ret = inflateInit2(strm, OML_ZLIB_WINDOWBITS);
      break;
    }
  }
  return ret;
}

/** Deflate for MBuffers
 *
 * This function doesn't use MBuffer messages
 *
 * \param strm pointer to the Zlib stream
 * \param flush flush mode for deflate
 * \param srcmbuf source MBuffer
 * \param dstmbuf destination MBuffer
 * \return Z_OK or Z_STREAM_END or success, others on error
 * \see MBuffer, deflate
 */
int
oml_zlib_def_mbuf(z_streamp strm, int flush, MBuffer *srcmbuf, MBuffer *dstmbuf)
{
  int ret = Z_STREAM_ERROR;
  int avail_in, avail_out;

  if(!strm) { ret = Z_STREAM_ERROR; }
  else if(!srcmbuf) { ret = Z_DATA_ERROR; }
  else if(!dstmbuf) { ret = Z_BUF_ERROR; }
  else {

    strm->next_in = mbuf_rdptr(srcmbuf);
    strm->avail_in = avail_in = mbuf_fill(srcmbuf);

    /** \bug the size of the output MBuffer is set to at least deflateBound(...,mbuf_fill),
     * but this might ignore data already input but not output
     */
    mbuf_check_resize(dstmbuf, deflateBound(strm, avail_in));

    strm->next_out = mbuf_wrptr(dstmbuf);
    strm->avail_out = avail_out = mbuf_wr_remaining(dstmbuf);
    logdebug("%s: deflating %dB into a %dB buffer\n", __FUNCTION__, avail_in, avail_out);

    ret = deflate(strm, flush);

    if (Z_OK == ret || Z_STREAM_END == ret) {
      if (0 != strm->avail_in) {
        logwarn("%s: Not all input was consumed, %dB remaining\n", strm->avail_in);
      }
      mbuf_read_skip(srcmbuf, avail_in-strm->avail_in);
      if (0 == strm->avail_out) {
        logwarn("%s: All output space was consumed\n");
      }
      mbuf_write_extend(dstmbuf, avail_out - strm->avail_out);

    }
  }

  return ret;
}

/** Inflate for MBuffers
 *
 * This function doesn't use MBuffer messages
 *
 * \param strm pointer to the Zlib stream
 * \param flush flush mode for deflate
 * \param srcmbuf source MBuffer
 * \param dstmbuf destination MBuffer
 * \return Z_OK or Z_STREAM_END or success, others on error
 * \see MBuffer, deflate
 */
int
oml_zlib_inf_mbuf(z_streamp strm, int flush, MBuffer *srcmbuf, MBuffer *dstmbuf)
{
  int ret = Z_STREAM_ERROR;
  int avail_in, avail_out;

  strm->avail_out=0; /* Ensure we point into the output MBuffer on the first pass */

  if(!strm) { ret = Z_STREAM_ERROR; }
  else if(!srcmbuf) { ret = Z_DATA_ERROR; }
  else if(!dstmbuf) { ret = Z_BUF_ERROR; }
  else {

    do {
      strm->next_in = mbuf_rdptr(srcmbuf);
      strm->avail_in = avail_in = mbuf_rd_remaining(srcmbuf);

      if(strm->avail_out<=0) {
        /* Expand the buffer by at least OML_ZLIB_CHUNKSIZE, but also the data that remains to be read */
        if (mbuf_check_resize(dstmbuf,  mbuf_rd_remaining(srcmbuf) + OML_ZLIB_CHUNKSIZE)) {
          logerror("%s: Cannot allocate %d more memory to hold inflated contents\n",
              __FUNCTION__, OML_ZLIB_CHUNKSIZE);
          return Z_BUF_ERROR;
        }
      }
      strm->next_out = mbuf_wrptr(dstmbuf);
      strm->avail_out = avail_out = mbuf_wr_remaining(dstmbuf);
      logdebug3("%s: inflating %dB into a %d(%d total)B buffer\n",
          __FUNCTION__, avail_in, avail_out, mbuf_length(dstmbuf));
      ret = inflate(strm, flush);
      if (Z_OK == ret || Z_STREAM_END == ret || Z_BUF_ERROR == ret) {
        if (0 != strm->avail_in) {
          logdebug3("%s: Not all input was consumed, %dB remaining\n", __FUNCTION__, strm->avail_in);
        }
        mbuf_read_skip(srcmbuf, avail_in - strm->avail_in);

        if (0 == strm->avail_out) {
          logdebug3("%s: All output space was consumed\n", __FUNCTION__);
        }
        mbuf_write_extend(dstmbuf, avail_out - strm->avail_out);

      } else if(ret<0 && ret != Z_BUF_ERROR) {
        logerror("%s: Error inflating data (%d)\n", __FUNCTION__, ret);
        logdebug2("%s: Data is as follows\n%s\n", __FUNCTION__,  to_octets(mbuf_rdptr(srcmbuf), mbuf_rd_remaining(srcmbuf)));
      }
    } while ((Z_BUF_ERROR == ret) &&
        ((avail_in - strm->avail_in > 0) || (avail_out - strm->avail_out > 0)) /* Ensure something has been read and/or written */
        );
  }

  return ret;
}

/** Terminate a stream, and output the remaining data, if any
 * \param strm pointer to the Zlib stream to initialise
 * \param mode the mode of operation (deflate/inflate)
 * \param dstbuf MBuffer where the remaining output data should be stored
 * \return 0 on success, <0 on error, as given by deflateInit, inflateInit
 * \see deflateEnd, inflateEnd
 */
int
oml_zlib_end(z_streamp strm, OmlZlibMode mode, MBuffer *dstbuf)
{
  int ret = Z_STREAM_ERROR;

  /** \bug This does not currently output any data to dstbuf */

  switch(mode) {
  case OML_ZLIB_DEFLATE:
    ret = deflateEnd(strm);
    break;

  case OML_ZLIB_INFLATE:
    ret = inflateEnd(strm);
    break;
  }

  return ret;
}

/** Compress from file source to file dest until EOF on source.
 *
 * \param source FILE pointer to the undcompressed source stream
 * \param dest FILE pointer to the compressed destination stream
 * \param level compression level 0--9, or -1 for default
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

  ret = oml_zlib_init(&strm, OML_ZLIB_DEFLATE, level);
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
  ret = oml_zlib_init(&strm, OML_ZLIB_INFLATE, 0);
  if (ret != Z_OK) {
    return ret;
  }

  /* run inflate() on input until output buffer not full */
  do {

    strm.avail_in = fread(in, 1, OML_ZLIB_CHUNKSIZE, source);
    if (ferror(source)) {
      ret = Z_ERRNO;
      goto cleanup;
    }
    if (strm.avail_in == 0) {
      break;
    }
    strm.next_in = in;

    logdebug3("%s: read %d bytes\n", __FUNCTION__, strm.avail_in);

    /* run inflate() on input until output buffer not full */
    do {

      if(!resynced) {
        strm.avail_out = OML_ZLIB_CHUNKSIZE;
        strm.next_out = out;
      }

      ret = inflate(&strm, Z_NO_FLUSH);
      switch (ret) {
      case Z_NEED_DICT:
        ret = Z_DATA_ERROR;     /* and fall through */
      case Z_DATA_ERROR:
        if (resynced == 1) {
          logdebug3("%s: error after resync\n", __FUNCTION__);
          /* goto cleanup; */

        } else if(oml_zlib_sync(&strm) == Z_OK) { /** \bug Doesn't handle resyncing cases with a new GZip header */
          logdebug3("%s: found potential next block\n", __FUNCTION__);
          resynced = 1;
        } else {
          goto cleanup;
        }
        break;
      case Z_STREAM_ERROR:
        logerror("Zlib inflate state clobbered\n");
      case Z_MEM_ERROR:
        goto cleanup;
      }

      have = OML_ZLIB_CHUNKSIZE - strm.avail_out;
      if (fwrite(out, 1, have, dest) != have || ferror(dest)) {
        ret = Z_ERRNO;
        goto cleanup;
      } else if (have > 0) {
        strm.avail_out = OML_ZLIB_CHUNKSIZE;
        resynced = 0;
      }

    } while (strm.avail_out == 0 || resynced);

    /* done when inflate() says it's done */
  } while (ret != Z_STREAM_END);

cleanup:

  /* Write data out if any is still available */
  if((have = OML_ZLIB_CHUNKSIZE - strm.avail_out)>0) {
    fwrite(out, 1, have, dest); /* XXX: we can't really do anything of this fails at this stage... */
  }

  logdebug3("%s: Cleanup with status %d, %td bytes unread\n", __FUNCTION__, ret, strm.avail_in);
  if(strm.avail_in > 0) {
    logdebug3("%s: Rewinding input by %td\n", __FUNCTION__, strm.avail_in);
    /* If same data has not been consumed, reset the file to there */
    /* \bug fseeking back to the end of the Gzip stream will not work on stdin */
    fseek(source, ftell(source)-(long)strm.avail_in, SEEK_SET);
  }

  /* clean up and return */
  (void)inflateEnd(&strm);
  return ret == Z_STREAM_END ? Z_OK : ret;
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
 * potential sync point has been found, Z_STREAM_END if a new GZip header has
 * been found, Z_DATA_ERROR otherwise.
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
    if ((*strm->next_in) == 0x1f) {
      ret = Z_STREAM_END;

    } else if ((*strm->next_in) == 0x00) {
      ret = Z_OK;
    }
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
 * \param len length of data in buf
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
