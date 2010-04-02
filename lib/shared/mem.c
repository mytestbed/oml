#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include "mem.h"
#include "log.h"

static size_t xbytes = 0;
static size_t xnew = 0;
static size_t xfreed = 0;

static void xcount_new   (size_t bytes) { xbytes += bytes; xnew   += bytes; }
static void xcount_freed (size_t bytes) { xbytes -= bytes; xfreed += bytes; }

void xmemreport (void)
{
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

  loginfo ("%"PRIuMAX" %s currently allocated [%"
             PRIuMAX" alloc, %"
             PRIuMAX" free, %"
             PRIuMAX" current]\n",
             (uintmax_t)xbytes_h, units,
             (uintmax_t)xnew, (uintmax_t)xfreed, (uintmax_t)xbytes);
}

#define xreturn(ptr, size, str)                                         \
  do {                                                                  \
    logerror(str);                                                      \
    logerror("%d bytes allocated, trying to add %d bytes\n",            \
             xbytes, size);                                             \
    return ptr;                                                         \
  } while (0);

void *xmalloc (size_t size)
{
  size += sizeof (size_t);
  void *ret = malloc (size);
  memset (ret, 0, size);
  if (!ret)
    xreturn (ret, size, "Out of memory, malloc failed\n");
  *(size_t*)ret = size;
  xcount_new (size);
  return (size_t*)ret + 1;
}

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

void *xrealloc (void *ptr, size_t size)
{
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

void xfree (void *ptr)
{
  if (ptr) {
    size_t *sptr = (size_t*)ptr - 1, size = *sptr;
    free (sptr);
    xcount_freed (size);
  }
}

/*
 * xstralloc() allocates (len + 1) bytes of memory and
 * memset()s the allocated memory to zero.  It returns
 * a pointer to the allocated memory.  The program dies if
 * the allocation fail.
 */
char*
xstralloc (size_t len)
{
  char *ret = xmalloc (len + 1);
  memset (ret, 0, len + 1);
  return ret;
}

/* xmemdupz() and xstrndup() are taken from Git. */

/*
 * xmemdupz() allocates (len + 1) bytes of memory, copies "len" bytes
 * from "data" into the new memory, zero terminates the copy, adn
 * returns a pointer to the allocated memory.  If the allocation
 * fails, the program dies.
 */
void *xmemdupz (const void *data, size_t len)
{
  char *ret = xmalloc (len + 1);
  memcpy (ret, data, len);
  ret[len] = '\0';
  return ret;
}

/*
 * xstrndup() duplicates string "str", up to "len" characters, and
 * then adds a zero terminator, allocating enough memory to hold the
 * duplicate and its zero terminator.  It returns a pointer to the
 * allocated memory.
 */
char*
xstrndup (const char *str, size_t len)
{
  char *p = memchr (str, '\0', len);
  return xmemdupz (str, p ? (size_t)(p - str) : len);
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
 End:
*/
