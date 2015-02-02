/*
 * Copyright 2010-2015 National ICT Australia (NICTA), Australia
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
#ifndef CBUF_H__
#define CBUF_H__

struct cbuffer_page {
  int empty;    /* True if reading has passed beyond this node */
  size_t size;  /* Allocated storage size */
  size_t fill;  /* Number of bytes currently in the buffer */
  size_t read;  /* Current reading pointer */
  char *buf;    /* Underlying storage */
  struct cbuffer_page *next;  /* Next buffer in chain */
};

typedef struct _cbuffer {
  int page_size;
  struct cbuffer_page *read;
  struct cbuffer_page *tail;
} CBuffer;

struct cbuffer_cursor {
  struct cbuffer_page *page;
  size_t index; // Index into page
};

CBuffer *cbuf_create(int default_size);
void cbuf_destroy (CBuffer *cbuf);
int cbuf_add_page (CBuffer *cbuf, int size);
int cbuf_write (CBuffer *cbuf, char *buf, size_t size);
struct cbuffer_cursor *cbuf_cursor (CBuffer *cbuf);
void cbuf_write_cursor (CBuffer *cbuf, struct cbuffer_cursor *cursor);
int cbuf_read_cursor (CBuffer *cbuf, struct cbuffer_cursor *cursor, size_t n);
char* cbuf_cursor_pointer (struct cbuffer_cursor *cursor);
size_t cbuf_cursor_page_remaining (struct cbuffer_cursor *cursor);
int cbuf_advance_cursor (struct cbuffer_cursor *cursor, size_t n);
int cbuf_consume_cursor (struct cbuffer_cursor *cursor, size_t n);

#endif /* CBUF_H__ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
