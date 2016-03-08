/*
 * Copyright 2007-2016 National ICT Australia (NICTA), Australia
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

#ifndef MSTRING_H__
#define MSTRING_H__

#include <stdlib.h>

/** A managed string */
typedef struct
{
  /** Currently allocated size */
  size_t size;
  /** Current length of the C string */
  size_t length;
  /** Dynamically allocated buffer containing a nil-terminated C string */
  char*  buf;
} MString;

MString* mstring_create (void);

int mstring_set (MString* mstr, const char* str);
int mstring_cat (MString* mstr, const char* str);
int mstring_sprintf (MString *mstr, const char *fmt, ...);

size_t mstring_len (MString* mstr);
char* mstring_buf (MString* mstr);

void mstring_delete (MString* mstr);

#endif // MSTRING_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
