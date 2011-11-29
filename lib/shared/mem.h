#ifndef MEM_H__
#define MEM_H__

#include <stdlib.h>

void *xmalloc (size_t size);
void *xcalloc (size_t count, size_t size);
void *xrealloc (void *ptr, size_t size);
char *xstralloc (size_t len);
void *xmemdupz (const void *data, size_t len);
char *xstrndup (const char *str, size_t len);
void xfree (void *ptr);

void xmemreport (void);

#endif /* MEM_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
