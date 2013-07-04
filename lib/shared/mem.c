/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
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
/** \file mem.c
 * The "x*()" functions: accountable memory allocation functions.
 *
 * To avoid confusion, these functions manipulate chunks of memory we call
 * 'xchunks'. They are essentially normally malloc(3)'d memory, of length
 * sizeof(size_t)+SIZE, and start with an offset of size_t from the malloc(3)'d
 * block. The first size_t element is used to store the actual size of the
 * xchunk (sizeof(size_t)+SIZE).
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#include "ocomm/o_log.h"
#include "mem.h"

static size_t xbytes = 0;
static size_t xnew = 0;
static size_t xfreed = 0;
static size_t xmax = 0;

/** Take into account newly allocated memory.
 * \param bytes size of the new xchunk
 * \see xmembytes, xmemnew, xmemreport
 */
static void xcount_new   (size_t bytes) {
#if OML_MEM_DEBUG
  o_log(O_LOG_DEBUG4, "Allocated %dB of memory\n", bytes);
#endif
  xbytes += bytes; xnew += bytes;
  if (xbytes > xmax) {
    xmax = xbytes;
  }
}

/** Take into account freed memory.
 * \param bytes size of the freed xchunk
 * \see xmembytes, xmemfreed, xmemreport
 */
static void xcount_freed (size_t bytes) {
#if OML_MEM_DEBUG
  o_log(O_LOG_DEBUG4, "Freed %dB of memory\n", bytes);
#endif
  xbytes -= bytes; xfreed += bytes;
}

/** Report the current memory allocation tracked by x*() functions */
size_t xmembytes() { return xbytes; }
/** Report the cumulated allocated memory tracked by x*() functions */
size_t xmemnew() { return xnew; }
/** Report the cumulated freed memory tracked by x*() functions */
size_t xmemfreed() { return xfreed; }

/** Create a summary of the dynamically allocated memory tracked by x*() functions
 *
 * \return a pointer to a statically allocated, nul-terminated, string containing the summary
 * \see xmemreport
 * */
char *xmemsummary ()
{
  static char summary[1024];

  size_t xbytes_h = xbytes;
  char *units = "bytes";
  if (xbytes_h > 10*(1<<10)) {
    units = "KiB";
    xbytes_h >>= 10;
  }
  if (xbytes_h > 10*(1<<10)) {
    units = "MiB";
    xbytes_h >>= 10;
  }

  snprintf(summary, 1024, "%"PRIuMAX" %s currently allocated [%"
             PRIuMAX" allocated overall, %"
             PRIuMAX" freed, %"
             PRIuMAX" current, %"
             PRIuMAX" maximum]",
             (uintmax_t)xbytes_h, units,
             (uintmax_t)xnew, (uintmax_t)xfreed, (uintmax_t)xbytes,
             (uintmax_t)xmax);
  summary[1023] = '\0';

  return summary;
}
/** Log a summary of the dynamically allocated memory tracked by x*() functions
 *
 * \param loglevel log level at which the message should be issued
 * \see xmemsummary
 * */
void xmemreport (int loglevel)
{
  o_log(loglevel, "%s\n", xmemsummary());
}

#define xreturn(ptr, size, str)                                         \
  do {                                                                  \
    logerror(str);                                                      \
    logerror("%d bytes allocated, trying to add %d bytes\n",            \
             xbytes, size);                                             \
    return ptr;                                                         \
  } while (0);

/** Allocate memory, tracking track of how much.
 *
 * The allocated memory is one size_t larger, just before the returned pointer,
 * to store the size of the xchunk.
 *
 * \param size desired size to allocate
 * \return an xchunk of memory at least as big as size, or NULL
 * \see malloc(3)
 */
void *xmalloc (size_t size)
{
  size += sizeof (size_t);
  void *ret = malloc (size);
  if (!ret)
    xreturn (ret, size, "Out of memory, malloc failed\n");
  memset (ret, 0, size);
  *(size_t*)ret = size;
  xcount_new (size);
  return (size_t*)ret + 1;
}

/** Allocate array, tracking track of allocated memory.
 *
 * \param number of elements in the array
 * \param size size of one element
 * \return an xchunk of memory at least as big as size, or NULL
 * \see calloc(3), xmalloc
 */
void *xcalloc (size_t count, size_t size)
{
  if (size >= sizeof (size_t))
    count++;
  else {
    int n = 0;
    size_t ssize = sizeof (size_t);
    while (ssize > size) {
      ssize >>= 1;
      n++;
    }
    count += (1 << n);
  }
  void *ret = calloc (count, size);
  if (!ret)
    xreturn (ret, count * size, "Out of memory, calloc failed\n");
  *(size_t*)ret = count * size;
  xcount_new (count * size);
  return (size_t*)ret + 1;
}

/** Resize an allocated xchunk, keeping track of the changes.
 *
 * \param xchunk to resize
 * \param size minimum size to resize the xchunk to
 * \return a new xchunk of at least the requested size containing the old data, or NULL
 * \see realloc(3)
 */
void *xrealloc (void *ptr, size_t size)
{
  if (!ptr) return xmalloc (size);
  size += sizeof (size_t);
  ptr = (size_t*)ptr - 1;
  size_t old = *(size_t*)ptr;
  void *ret = realloc (ptr, size);
  if (!ret)
    xreturn (ret, size - old, "Out of memory, realloc failed\n");
  *(size_t*)ret = size;
  xcount_new (size);
  xcount_freed (old);
  return (size_t*)ret + 1;
}

/** Report the size of the given xchunk
 *
 * \param ptr pointer to an xmalloc()'d bit of memory
 * \return the size of the allocated xchunk
 * \see xmalloc, malloc_usable_size(3)
 */
size_t xmalloc_usable_size(void *ptr) {
  if(!ptr) return 0;
  size_t *sptr = (size_t*)ptr - 1;
  return *sptr-sizeof(size_t);
}

/** Free an xchunk
 *
 * \param ptr pointer to an xmalloc()'d bit of memory
 * \see xmalloc, free(3)
 */
void xfree (void *ptr)
{
  if (ptr) {
    size_t *sptr = (size_t*)ptr - 1, size = *sptr;
    free (sptr);
    xcount_freed (size);
  }
}

/** Allocates (len + 1) bytes of memory and initialise it to zero (hence, very suitable for a string of length up to len).
 *
 * \param len maximum size of the string to be stored in the allocated xchunk
 * \return the allocated xchunk, or NULL
 * \see xmalloc, memset(3)
 */
char* xstralloc (size_t len)
{
  char *ret = xmalloc (len + 1);
  memset (ret, 0, len + 1);
  return ret;
}

/* xmemdupz() and xstrndup() are taken from Git. */

/** Allocates (len + 1) bytes of memory, copies "len" bytes from "data" into the new memory and zero terminates the copy
 *
 * \param data data to copy in the new xchunk
 * \param len length of the data
 * \return the newly allocated xchunk, or NULL
 */
void *xmemdupz (const void *data, size_t len)
{
  char *ret = xmalloc (len + 1);
  memcpy (ret, data, len);
  ret[len] = '\0';
  return ret;
}

/** Duplicate string, up to len characters, and nul-terminate it.
 *
 * \param str string to copy in the new xchunk
 * \param len length of the string
 * \return the newly allocated xchunk, or NULL
 * \see strndup(3)
 */
char* xstrndup (const char *str, size_t len)
{
  char *p = memchr (str, '\0', len);
  return xmemdupz (str, p ? (size_t)(p - str) : len);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
