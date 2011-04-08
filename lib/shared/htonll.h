/*
 * Copyright 2010-2011 National ICT Australia (NICTA), Australia
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
#ifndef HTONLL_H__
#define HTONLL_H__

#include <config.h>
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
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
