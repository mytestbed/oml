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

/* Zlib parameters that we might want to adjust */
#define OML_ZLIB_CHUNKSIZE  16384                   /**< \see OmlZlibOutStream::chunk_size */
#define OML_ZLIB_WINDOWBITS 31                      /**< 31 makes Zlib output GZip headers \see deflateInit2. */
#define OML_ZLIB_ZLEVEL     Z_DEFAULT_COMPRESSION   /**< \see deflateInit2 */
#define OML_ZLIB_STRATEGY   Z_DEFAULT_STRATEGY      /**< \see deflateInit2 */
#define OML_ZLIB_FLUSH      Z_NO_FLUSH              /**< \see deflate */
#define OML_ZLIB_CHUNK      512

/** Compress from file source to file dest until EOF on source. */
int oml_zlib_def(FILE *source, FILE *dest, int level);
/** Decompress from file source to file dest until stream ends or EOF. */
int oml_zlib_inf(FILE *source, FILE *dest);

#endif /* OML_ZLIB_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
