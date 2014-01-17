/*
 * Copyright 2007-2014 National ICT Australia (NICTA), Australia
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
#ifndef MBUF_H__
#define MBUF_H__

#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

typedef struct MBuffer
{
  /** Underlying storage */
  uint8_t* base;
  /** size of allocated buffer */
  size_t   length;
  /** Number of bytes unfilled \see mbuf_wr_remaining */
  size_t   wr_remaining;
  /** Number of data bytes remaining to be read \see mbuf_rd_remaining*/
  size_t   rd_remaining;
  /** number of bytes 'valid' data (number of bytes filled) \see mbuf_fill */
  size_t   fill;

  /** Pointer at which to read the next byte from the buffer \see mbuf_rdptr */
  uint8_t* rdptr;
  /** Pointer at which to write the next byte into the buffer \see mbuf_wrptr*/
  uint8_t* wrptr;
  /** Begining of message if more than one in buffer \see mbuf_message */
  uint8_t* msgptr;

  /** Mnimum increase in buffer size on resize */
  size_t   min_resize;
  /** Keeps track on how much initial buffer got extended */
  uint8_t  resized;
  /** If true, allow resizing, otherwise fail */
  uint8_t  allow_resizing;

  /** Supports chaining together of MBuffer instances */
  struct MBuffer* next;
} MBuffer;

MBuffer* mbuf_create (void);
MBuffer* mbuf_create2 (size_t buffer_length, size_t min_resize);
void mbuf_destroy (MBuffer* mbuf);

uint8_t* mbuf_buffer (MBuffer* mbuf);
uint8_t* mbuf_message (MBuffer* mbuf);
uint8_t* mbuf_rdptr (MBuffer* mbuf);
uint8_t* mbuf_wrptr (MBuffer* mbuf);

size_t mbuf_length (MBuffer* mbuf);
size_t mbuf_remaining (MBuffer* mbuf) __attribute__((deprecated));
size_t mbuf_rd_remaining (MBuffer* mbuf);
size_t mbuf_wr_remaining (MBuffer* mbuf);
size_t mbuf_fill (MBuffer* mbuf);
size_t mbuf_fill_excluding_msg (MBuffer* mbuf);
size_t mbuf_read_offset (MBuffer* mbuf);
size_t mbuf_write_offset (MBuffer* mbuf);
size_t mbuf_message_offset (MBuffer* mbuf);
size_t mbuf_message_length (MBuffer* mbuf);
size_t mbuf_message_index (MBuffer* mbuf);

int mbuf_resize (MBuffer* mbuf, size_t new_length);
int mbuf_check_resize (MBuffer* mbuf, size_t bytes);

int mbuf_begin_write (MBuffer* mbuf);
int mbuf_reset_write (MBuffer* mbuf);
int mbuf_write (MBuffer* mbuf, const uint8_t* buf, size_t len);
int mbuf_print(MBuffer* mbuf, const char* format, ...);

int mbuf_begin_read (MBuffer* mbuf);
int mbuf_reset_read (MBuffer* mbuf);
int mbuf_read (MBuffer* mbuf, uint8_t* buf, size_t len);
int mbuf_read_skip (MBuffer* mbuf, size_t len);
int mbuf_read_byte (MBuffer* mbuf);
size_t mbuf_find (MBuffer* mbuf, uint8_t c);
size_t mbuf_find_not (MBuffer* mbuf, uint8_t c);


int mbuf_consume_message (MBuffer* mbuf);
void mbuf_message_start_advance (MBuffer *mbuf, size_t n);
int mbuf_repack (MBuffer* mbuf);
int mbuf_repack_message (MBuffer* mbuf);
int mbuf_repack_message2 (MBuffer* mbuf);
int mbuf_clear (MBuffer* mbuf);
int mbuf_clear2 (MBuffer* mbuf, int zeroBuffer);


#endif // MBUF_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
