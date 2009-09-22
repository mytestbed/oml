/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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

#include <stdint.h>

typedef struct
{
	uint8_t* base;             //! Underlying storage
	size_t   length;           //! size of allocated buffer
	size_t   wr_remaining;     //! Number of bytes unfilled.
	size_t   rd_remaining;     //! Number of data bytes remaining to be read
	size_t   fill;             //! number of bytes 'valid' data (number of bytes filled)

	uint8_t* rdptr;            //! Pointer at which to read the next byte from the buffer
	uint8_t* wrptr;            //! Pointer at which to write the next byte into the buffer
	uint8_t* message_start;    //! Begin of message if there are more than one in buffer

} OmlMBufferEx;

OmlMBufferEx* mbuf_create (void);
void mbuf_destroy (OmlMBufferEx* mbuf);
uint8_t* mbuf_buffer (OmlMBufferEx* mbuf);
size_t mbuf_length (OmlMBufferEx* mbuf);
size_t mbuf_remaining (OmlMBufferEx* mbuf);
size_t mbuf_fill (OmlMBufferEx* mbuf);
size_t mbuf_read_offset (OmlMBufferEx* mbuf);
size_t mbuf_write_offset (OmlMBufferEx* mbuf);
size_t mbuf_message_offset (OmlMBufferEx* mbuf);
int mbuf_resize (OmlMBufferEx* mbuf, size_t new_length);
int mbuf_check_resize (OmlMBufferEx* mbuf, size_t bytes);
int mbuf_write (OmlMBufferEx* mbuf, const uint8_t* buf, size_t len);
int mbuf_read (OmlMBufferEx* mbuf, uint8_t* buf, size_t len);
int mbuf_begin_read (OmlMBufferEx* mbuf);
int mbuf_begin_write (OmlMBufferEx* mbuf);
int mbuf_clear (OmlMBufferEx* mbuf);
int mbuf_reset_write (OmlMBufferEx* mbuf);
int mbuf_reset_read (OmlMBufferEx* mbuf);
int mbuf_consume_message (OmlMBufferEx* mbuf);
int mbuf_repack (OmlMBufferEx* mbuf);
int mbuf_repack_message (OmlMBufferEx* mbuf);

char* to_octets (unsigned char* buf, int len);

#endif // MBUF_H__

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
