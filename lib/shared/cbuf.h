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
int cbuf_add_page (CBuffer *cbuf, int size);
int cbuf_write (CBuffer *cbuf, char *buf, size_t size);
struct cbuffer_cursor *cbuf_cursor (CBuffer *cbuf);

void
cbuf_write_cursor (CBuffer *cbuf, struct cbuffer_cursor *cursor);

int
cbuf_read_cursor (CBuffer *cbuf, struct cbuffer_cursor *cursor, size_t n);

char*
cbuf_cursor_pointer (struct cbuffer_cursor *cursor);

size_t cbuf_cursor_page_remaining (struct cbuffer_cursor *cursor);

int cbuf_consume_cursor (struct cbuffer_cursor *cursor, size_t n);


#endif /* CBUF_H__ */
