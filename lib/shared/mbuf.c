/*
 * Copyright 2007-2012 National ICT Australia (NICTA), Australia
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"
#include "mbuf.h"

#define DEF_BUF_SIZE 512
#define DEF_MIN_BUF_RESIZE (size_t)(0.1 * DEF_BUF_SIZE)
/*
 *  mbuf->base : base address of the underlying storage
 *
 *  mbuf->length : number of bytes allocated for underlying storage
 *
 *  mbuf->fill : number of bytes filled with data
 *
 *  mbuf->wr_remaining : remaining space left to write into (between fill and end of allocated space)
 *
 *  mbuf->rd_remaining : number of bytes of valid data left to read (difference between mbuf->wrptr and mbuf->rdptr)
 *
 *  mbuf->rdptr : When reading the buffer, pointer to the next byte to be read
 *
 *  mbuf->wrptr : When writing the buffer, pointer to the next byte to be written
 *
 *  mbuf->msgptr : When reading the buffer, pointer to the base of the start of the current message
 *
 */

static void
mbuf_check_invariant (MBuffer* mbuf)
{
  assert (mbuf != NULL);
  assert (mbuf->base != NULL);
  assert (mbuf->wr_remaining <= mbuf->length);
  assert (mbuf->fill <= mbuf->length);
  assert (mbuf->fill + mbuf->wr_remaining == mbuf->length);
  assert (mbuf->wrptr >= mbuf->rdptr);
  assert (mbuf->wrptr - mbuf->base == (int)mbuf->fill);
  assert ((int)mbuf->rd_remaining == (mbuf->wrptr - mbuf->rdptr));
  assert ((mbuf->msgptr <= mbuf->rdptr) || (mbuf->msgptr <= mbuf->wrptr));
}

MBuffer*
mbuf_create (void)
{
  return mbuf_create2(DEF_BUF_SIZE, DEF_MIN_BUF_RESIZE);
}

MBuffer*
mbuf_create2 (size_t buffer_length, size_t min_resize)
{
  MBuffer* mbuf = xmalloc (sizeof (MBuffer));
  if (mbuf == NULL) return NULL;

  mbuf->length = buffer_length > 0 ? buffer_length : DEF_BUF_SIZE;
  mbuf->min_resize = min_resize > 0 ? min_resize : DEF_MIN_BUF_RESIZE;
  mbuf->base = xmalloc (mbuf->length);

  if (mbuf->base == NULL)
    {
      xfree (mbuf);
      return NULL;
    }

  mbuf->rdptr = mbuf->base;
  mbuf->wrptr = mbuf->base;
  mbuf->msgptr = mbuf->base;
  mbuf->fill = 0;
  mbuf->wr_remaining = mbuf->length;
  mbuf->rd_remaining = 0;
  mbuf->allow_resizing = 1;

  mbuf_check_invariant(mbuf);

  return mbuf;
}

void
mbuf_destroy (MBuffer* mbuf)
{
  xfree (mbuf->base);
  xfree (mbuf);
}

uint8_t*
mbuf_buffer (MBuffer* mbuf)
{
  return mbuf->base;
}

size_t
mbuf_length (MBuffer* mbuf)
{
  return mbuf->length;
}

size_t
mbuf_remaining (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->rdptr;
}

size_t
mbuf_fill (MBuffer* mbuf)
{
  return mbuf->fill;
}

size_t
mbuf_fill_excluding_msg (MBuffer* mbuf)
{
  return mbuf->msgptr - mbuf->rdptr;
}

size_t
mbuf_read_offset (MBuffer* mbuf)
{
  return mbuf->rdptr - mbuf->base;
}

size_t
mbuf_write_offset (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->base;
}

size_t
mbuf_message_offset (MBuffer* mbuf)
{
  return mbuf->msgptr - mbuf->base;
}

/*
 * Difference between the rdptr and the msgptr
 */
size_t
mbuf_message_index (MBuffer* mbuf)
{
  return mbuf->rdptr - mbuf->msgptr;
}

size_t
mbuf_message_length (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->msgptr;
}

void
mbuf_message_start_advance (MBuffer *mbuf, size_t n)
{
  if (mbuf && n < mbuf->fill - mbuf_message_offset (mbuf)) {
    mbuf->msgptr += n;
  }
}

uint8_t*
mbuf_message (MBuffer* mbuf)
{
  return mbuf->msgptr;
}

uint8_t*
mbuf_rdptr (MBuffer* mbuf)
{
  return mbuf->rdptr;
}

uint8_t*
mbuf_wrptr (MBuffer* mbuf)
{
  return mbuf->wrptr;
}

int
mbuf_resize (MBuffer* mbuf, size_t new_length)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL || mbuf->base == NULL)
    return -1;

  /* Don't resize if we don't have to */
  if (new_length <= mbuf->length)
    return 0;

  if (! mbuf->allow_resizing)
    return -1;

/*   // Make sure we grow at leat by a minimum delta */
/*   if ((new_length - mbuf->length) < mbuf->min_resize) { */
/*     new_length = mbuf->length + mbuf->min_resize; */
/*   } */

  int wr_offset = mbuf->wrptr - mbuf->base;
  int rd_offset = mbuf->rdptr - mbuf->base;
  int msg_offset = mbuf->msgptr - mbuf->base;

  assert (wr_offset == (int)mbuf->fill);

  uint8_t* new = xrealloc (mbuf->base, new_length);
  if (new == NULL)
    return -1;

  mbuf->base = new;
  mbuf->wrptr = mbuf->base + wr_offset;
  mbuf->rdptr = mbuf->base + rd_offset;
  mbuf->msgptr = mbuf->base + msg_offset;

  mbuf->resized += (new_length - mbuf->length);
  mbuf->length = new_length;
  mbuf->wr_remaining = mbuf->length - mbuf->fill;

  mbuf_check_invariant (mbuf);

  return 0;
}

int
mbuf_check_resize (MBuffer* mbuf, size_t bytes)
{
  if (mbuf->wr_remaining < bytes) {
    size_t inc = bytes - mbuf->wr_remaining;
    size_t min_resize = mbuf->min_resize > 0 ? mbuf->min_resize : mbuf->length;
    inc = min_resize * (inc / min_resize + 1);
    return mbuf_resize (mbuf, mbuf->length + inc);
  } else {
    return 0;
  }
}

/**
 *  Write bytes into the mbuf.
 *
 *  Write len bytes from the raw buffer pointed to by buf into the
 *  mbuf.  If the mbuf doen't have enough remaining space to fit len
 *  bytes, no bytes are written to mbuf and the function will fail.
 *
 *  If the function succeeds, the bytes are written starting at the
 *  mbuf's write pointer, and the write pointer is advanced by len
 *  bytes.
 *
 *  @param mbuf the mbuf to write bytes into
 *  @param buf the bytes to write
 *  @param len the length of the buffer of bytes (not including \0 for strings)
 *  @return 0 on success, -1 on failure.
 *
 */
int
mbuf_write (MBuffer* mbuf, const uint8_t* buf, size_t len)
{
  if (mbuf == NULL || buf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  if (mbuf_check_resize (mbuf, len) == -1)
    return -1;

  memcpy (mbuf->wrptr, buf, len);

  mbuf->wrptr += len;
  mbuf->fill += len;
  mbuf->wr_remaining -= len;
  mbuf->rd_remaining += len;

  mbuf_check_invariant (mbuf);

  return 0;
}

/**
 *  Append the printed string described by +format+ to the mbuff
 *
 *  Write len bytes from the raw buffer pointed to by buf into the
 *  mbuf.  If the mbuf doen't have enough remaining space to fit len
 *  bytes, no bytes are written to mbuf and the function will fail.
 *
 *  If the function succeeds, the bytes are written starting at the
 *  mbuf's write pointer, and the write pointer is advanced by len
 *  bytes.
 *
 *  @return 0 on success, -1 on failure.
 *
 */
int
mbuf_print(MBuffer* mbuf, const char* format, ...)
{
  if (mbuf == NULL || format == NULL) return -1;
  mbuf_check_invariant (mbuf);

  int len;
  int success = 0;

  do {
    va_list arglist;
    va_start(arglist, format);
    len = vsnprintf((char*)mbuf->wrptr, mbuf->wr_remaining, format, arglist);
    va_end(arglist);
    if (! (success = (len <= (int)mbuf->wr_remaining))) {
      if (mbuf_check_resize(mbuf, len) == -1)
    return -1;
    }
  } while (! success);

  mbuf->wrptr += len;
  mbuf->fill += len;
  mbuf->wr_remaining -= len;
  mbuf->rd_remaining += len;

  mbuf_check_invariant (mbuf);
  return 0;
}

/**
 *  Read bytes from mbuf.
 *
 *  Read len bytes from the mbuf into the raw buffer pointed to by
 *  buf.  If the mbuf doesn't have at least len bytes available to
 *  read, then the function fails.
 *
 *  If the function succeeds, the bytes are written to buf, with
 *  reading starting at the mbuf's read pointer, and the read pointer
 *  is then advanced by len bytes.
 *
 *  @return 0 on success, -1 on failure.
 *
 */
int
mbuf_read (MBuffer* mbuf, uint8_t* buf, size_t len)
{
  if (mbuf == NULL || buf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  if (mbuf->rd_remaining < len) return -1;

  memcpy (buf, mbuf->rdptr, len);

  mbuf->rdptr += len;
  mbuf->rd_remaining -= len;

  mbuf_check_invariant (mbuf);

  return 0;
}

int
mbuf_read_skip (MBuffer* mbuf, size_t len)
{
  if (mbuf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  if (mbuf->rd_remaining < len) return -1;

  mbuf->rdptr += len;
  mbuf->rd_remaining -= len;

  mbuf_check_invariant (mbuf);

  return 0;
}

int
mbuf_read_byte (MBuffer* mbuf)
{
  if (mbuf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  if (mbuf->rd_remaining < 1) return -1;

  int byte = (int)*(mbuf->rdptr);

  mbuf->rdptr++;
  mbuf->rd_remaining--;

  mbuf_check_invariant (mbuf);

  return byte;
}

size_t
mbuf_find (MBuffer* mbuf, uint8_t c)
{
  if (mbuf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  uint8_t* p = mbuf->rdptr;

  while (p < mbuf->wrptr && *p != c) p++;

  int result = -1;
  if (p < mbuf->wrptr && *p == c)
    result = p - mbuf->rdptr;

  mbuf_check_invariant (mbuf);
  return result;
}

size_t
mbuf_find_not (MBuffer* mbuf, uint8_t c)
{
  if (mbuf == NULL) return -1;

  mbuf_check_invariant (mbuf);

  uint8_t* p = mbuf->rdptr;

  while (p < mbuf->wrptr && *p == c) p++;

  int result = -1;
  if (p < mbuf->wrptr && *p != c)
    result = p - mbuf->rdptr;

  mbuf_check_invariant (mbuf);
  return result;
}

int
mbuf_begin_read (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  mbuf->msgptr = mbuf->rdptr;

  mbuf_check_invariant (mbuf);

  return 0;
}

int
mbuf_begin_write (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  mbuf->msgptr = mbuf->wrptr;

  mbuf_check_invariant (mbuf);

  return 0;
}

int
mbuf_clear (MBuffer* mbuf)
{
  return mbuf_clear2(mbuf, 1);
}

int
mbuf_clear2 (MBuffer* mbuf, int zero_buffer)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  if (zero_buffer) {
    memset (mbuf->base, 0, mbuf->length);
  }

  mbuf->rdptr = mbuf->wrptr = mbuf->msgptr = mbuf->base;
  mbuf->fill = 0;
  mbuf->wr_remaining = mbuf->length;
  mbuf->rd_remaining = 0;

  mbuf_check_invariant (mbuf);
  return 0;
}

int
mbuf_reset_write (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  // If in the middle of a read, can't do a write reset
  if (mbuf->rdptr > mbuf->msgptr)
    return -1;

  size_t msglen = mbuf->wrptr - mbuf->msgptr;
  mbuf->fill -= msglen;
  mbuf->wr_remaining += msglen;
  mbuf->rd_remaining -= msglen;
  mbuf->wrptr = mbuf->msgptr;

  mbuf_check_invariant (mbuf);
  return 0;
}

int
mbuf_reset_read (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  // If in the middle of a write, can't do a read reset
  if (mbuf->msgptr > mbuf->rdptr)
    return -1;

  mbuf->rdptr = mbuf->msgptr;
  mbuf->rd_remaining = mbuf->wrptr - mbuf->rdptr;

  mbuf_check_invariant (mbuf);
  return 0;
}

int
mbuf_consume_message (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  // Can't consume message if we're in the middle of writing it
  if (mbuf->msgptr > mbuf->rdptr)
    return -1;

  mbuf->msgptr = mbuf->rdptr;

  mbuf_check_invariant (mbuf);
  return 0;
}

int
mbuf_repack (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  memmove (mbuf->base, mbuf->rdptr, mbuf->rd_remaining);

  mbuf->fill = mbuf->rd_remaining;
  mbuf->wr_remaining = mbuf->length - mbuf->fill;
  mbuf->wrptr = mbuf->base + mbuf->fill;
  mbuf->msgptr = mbuf->rdptr = mbuf->base;

  mbuf_check_invariant (mbuf);
  return 0;
}

// Preserve the rdptr relative to the msgptr
// Max: I don't understand what this message is doing
int
mbuf_repack_message (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  size_t msg_remaining = mbuf->wrptr - mbuf->msgptr;

  memmove (mbuf->base, mbuf->msgptr, msg_remaining);

  mbuf->fill = mbuf->wrptr - mbuf->msgptr;
  mbuf->wr_remaining = mbuf->length - mbuf->fill;
  mbuf->wrptr = mbuf->base + mbuf->fill;
  mbuf->msgptr = mbuf->base;
  mbuf->rdptr = mbuf->wrptr - mbuf->rd_remaining;

  mbuf_check_invariant (mbuf);
  return 0;
}

int
mbuf_repack_message2 (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  size_t msg_size = mbuf->wrptr - mbuf->msgptr;
  if (msg_size > 0)
    memmove (mbuf->base, mbuf->msgptr, msg_size);

  mbuf->fill = msg_size;
  mbuf->wr_remaining = mbuf->length - msg_size;
  mbuf->rd_remaining = msg_size;
  mbuf->wrptr = mbuf->base + msg_size;
  mbuf->msgptr = mbuf->rdptr = mbuf->base;

  mbuf_check_invariant (mbuf);
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
