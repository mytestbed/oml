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
 * \brief Prototypes for Zlib helpers for OML
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 * \see OmlZlibOutStream;
 */

#ifndef OML_ZLIB_H_
#define OML_ZLIB_H_

#include <stdio.h>
#include <stddef.h> /* ptrdiff_t is in there */
#include <zlib.h>

#include "mbuf.h"

/* Zlib parameters that we might want to adjust */
#define OML_ZLIB_CHUNKSIZE  16384                   /**< \see OmlZlibOutStream::chunk_size */
#define OML_ZLIB_WINDOWBITS 31                      /**< 31 makes Zlib output GZip headers \see deflateInit2. */
#define OML_ZLIB_ZLEVEL     Z_DEFAULT_COMPRESSION   /**< \see deflateInit2 */
#define OML_ZLIB_STRATEGY   Z_DEFAULT_STRATEGY      /**< \see deflateInit2 */
#define OML_ZLIB_FLUSH      Z_NO_FLUSH              /**< \see deflate */

/** Mode of operation for the stream \see oml_zlib_init */
typedef enum {
  OML_ZLIB_DEFLATE = 0, /**< Deflate */
  OML_ZLIB_INFLATE,     /**< Inflate */
} OmlZlibMode;

/** Initialise a Zlib stream */
int oml_zlib_init(z_streamp strm, OmlZlibMode mode, int level);
/** Deflate for MBuffers */
int oml_zlib_def_mbuf(z_streamp strm, int flush, MBuffer *srcmbuf, MBuffer *dstmbuf);
/** Inflate for MBuffers */
int oml_zlib_inf_mbuf(z_streamp strm, int flush, MBuffer *srcmbuf, MBuffer *dstmbuf);
/** Terminate a stream, and output the remaining data, if any */
int oml_zlib_end(z_streamp strm, OmlZlibMode mode, MBuffer *dstbuf);

/** Compress from file source to file dest until EOF on source. */
int oml_zlib_def(FILE *source, FILE *dest, int level);
/** Decompress from file source to file dest until stream ends or EOF. */
int oml_zlib_inf(FILE *source, FILE *dest);

/** Search for the next block or new GZip header, whichever comes first, and advance the string. */
ptrdiff_t oml_zlib_sync(z_streamp strm);
/** Search for the next block or new GZip header, whichever comes first. */
ptrdiff_t oml_zlib_find_sync(const char *buf, size_t len);

#endif /* OML_ZLIB_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
