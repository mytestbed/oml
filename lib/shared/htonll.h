/*
 * Copyright 2010-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file htonll.h
 * \brief Macros for converting 64 bit integers between host and network representation.
 */
#ifndef HTONLL_H__
#define HTONLL_H__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <arpa/inet.h>

#if HAVE_BYTESWAP_H
#include <byteswap.h>
#else
#define bswap_16(value) ((((value) & 0xFF) << 8) | ((value) >> 8))
#define bswap_32(value)                                         \
  (((uint32_t)(bswap_16((uint16_t)((value) & 0xFFFF))) << 16) | \
   ((uint32_t)(bswap_16((uint16_t)((value) >> 16)))))
#define bswap_64(value)                                             \
  (((uint64_t)(bswap_32((uint32_t)((value) & 0xFFFFFFFF))) << 32) |  \
   ((uint64_t)(bswap_32((uint32_t)((value) >> 32)))))
#endif

#ifdef WORDS_BIGENDIAN
#define htonll(value) (value)
#define ntohll(value) (value)
#else
#define htonll(value) (bswap_64(value))
#define ntohll(value) (bswap_64(value))
#endif

#endif /* HTONLL_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
