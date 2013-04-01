#ifndef MEM_H__
#define MEM_H__

#include <stdlib.h>

#include "ocomm/o_log.h"

void *xmalloc (size_t size);
void *xcalloc (size_t count, size_t size);
void *xrealloc (void *ptr, size_t size);
size_t xmalloc_usable_size(void *ptr);
char *xstralloc (size_t len);
void *xmemdupz (const void *data, size_t len);
char *xstrndup (const char *str, size_t len);
void xfree (void *ptr);

size_t xmembytes();
size_t xmemnew();
size_t xmemfreed();
char *xmemsummary ();
void xmemreport (int loglevel);

/* Duplicate nil-terminated string
 *
 * \param str string to copy in the new xchunk
 * \return the newly allocated xchunk, or NULL
 * \see xstrndup
 * \see strndup(3), strlen(3)
 */
#define xstrdup(str)  xstrndup((str), strlen((str)))

#endif /* MEM_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
