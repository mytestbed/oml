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
#ifndef MBUF_H__
#define MBUF_H__
/** \file mbuf.h
 * \brief Interface for MBuffer
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>

/** An MBuffer is an auto-expanding chunk of memory, with various pointers for
 * writing, reading, and composing a message (e.g., packet) in multiple calls.
 *
 * Several functions of MBuffers rely on the internal pointers, as illustrated below.
 *
 * \code
 * ¦< - - - - - - - - - mbuf_length(); length - - - - - - - - - - >¦
 * ¦                                                               ¦
 * ¦                 mbuf_message_length()                         ¦
 * ¦                      ¦< - - - >¦                              ¦
 * ¦                      ¦         ¦                              ¦
 * ¦< - - - - mbuf_fill()-¦- - - - >¦                              ¦
 * mbuf_fill_excluding_msg()        ¦                              ¦
 * ¦< - - - - - - - - - ->¦         ¦                              ¦
 * ¦                      ¦         ¦                              ¦
 * ¦              mbuf_rd_remaining()                              ¦
 * ¦              ¦< - - -¦- - - - >¦< - - mbuf_wr_remaining() - ->¦
 * +--------------+-------+---------+------------------------------+
 * |rrrrrrrrrrrrrrRRRRRRRRMMMMMMMMMM...............................|
 * +--------------+-------+---------+------------------------------+
 *  ^             ^       ^         ^                             :
 *  |- - - - - - -|- - - -|- - - - >| mbuf_write_offset()         :
 *  |             |       |         `- mbuf_wrptr()               :
 *  |             |       |         :                             :
 *  |- - - - - - -|- - - >| mbuf_message_offset()                 :
 *  |             |       |- mbuf_message()                       :
 *  |             |       |         :                             :
 *  |             |- - - >| mbuf_message_index()                  :
 *  |- - - - - - >| mbuf_read_offset()                            :
 *  |             `- mbuf_rdptr()   :                             :
 *  |             :                 :                             :
 *  `- mbuf_buffer()                :                             :
 *                :                 :                             :
 *                :mbuf_read_skip() :                             :
 * Movement:      |------------->   X                             :
 *                                  |--mbuf_write_extend()->      X
 *
 * Legend:
 *  r: data already read
 *  R: data to be read (e.g., mbuf_read())
 *  .: data which can be overwritten  (e.g., mbuf_write())
 *  M: data already written for the current message (see mbuf_message() and others)
 *  X: function cannot move beyond this point
 *
 * \endcode
 * XXX: The message can be currently read or written, with no clear distinction.
 */
typedef struct {
  uint8_t* base;            /**< Underlying storage */
  size_t   length;          /**< size of allocated buffer */
  size_t   wr_remaining;    /**< Number of bytes unfilled \see mbuf_wr_remaining */
  size_t   rd_remaining;    /**< Number of data bytes remaining to be read \see mbuf_rd_remaining*/
  size_t   fill;            /**< Number of bytes 'valid' data (number of bytes filled) \see mbuf_fill */

  uint8_t* rdptr;           /**< Pointer at which to read the next byte from the buffer \see mbuf_rdptr */
  uint8_t* wrptr;           /**< Pointer at which to write the next byte into the buffer \see mbuf_wrptr*/
  uint8_t* msgptr;          /**< Begining of message if more than one in buffer \see mbuf_message */

  size_t   min_resize;      /**< Minimum increase in buffer size on resize */
  uint8_t  resized;         /**< Keeps track on how much initial buffer got extended */
  uint8_t  allow_resizing;  /**< If true, allow resizing, otherwise fail */
} MBuffer;

/** Create an MBuffer with the default parameters. */
MBuffer* mbuf_create (void);
/** Create an MBuffer. */
MBuffer* mbuf_create2 (size_t buffer_length, size_t min_resize);
/** Destroy an MBuffer and its storage. */
void mbuf_destroy (MBuffer* mbuf);

/** Get an MBuffer's storage. */
uint8_t* mbuf_buffer (MBuffer* mbuf);

/** Get the current message. */
uint8_t* mbuf_message (MBuffer* mbuf);
/** Get the read pointer. */
uint8_t* mbuf_rdptr (MBuffer* mbuf);
/** Get the write pointer. */
uint8_t* mbuf_wrptr (MBuffer* mbuf);

/** Get the length of the MBuffer's allocated buffer. */
size_t mbuf_length (MBuffer* mbuf);
/** Get the remaining amount of data to read in MBuffer */
size_t mbuf_remaining (MBuffer* mbuf) __attribute__((deprecated));
/** Get the remaining amount of data to read in MBuffer */
size_t mbuf_rd_remaining (MBuffer* mbuf);
/** Get the remaining amount of data to write in MBuffer */
size_t mbuf_wr_remaining (MBuffer* mbuf);
/** Get the currently occupied size of the MBuffer. */
size_t mbuf_fill (MBuffer* mbuf);
/** Get the currently occupied size of the MBuffer, not including the message currently being written. */
size_t mbuf_fill_excluding_msg (MBuffer* mbuf);
/** Get the read offset from the base of the MBuffer. */
size_t mbuf_read_offset (MBuffer* mbuf);
/** Get the write offset from the base of the MBuffer. */
size_t mbuf_write_offset (MBuffer* mbuf);
/** Get the message offset from the base of the MBuffer. */
size_t mbuf_message_offset (MBuffer* mbuf);
/** Get the message offset from the base of the MBuffer. */
size_t mbuf_message_index (MBuffer* mbuf);
/** Get the current message's length. */
size_t mbuf_message_length (MBuffer* mbuf);

/** Resize an MBuffer's allocated storage. */
int mbuf_resize (MBuffer* mbuf, size_t new_length);
/** Resize an MBuffer if it cannot contain the specified amount of data. */
int mbuf_check_resize (MBuffer* mbuf, size_t bytes);

/** Prepare to write a new message to an MBuffer. */
int mbuf_begin_write (MBuffer* mbuf);
/** Reset the write pointer to the beginning of the message. */
int mbuf_reset_write (MBuffer* mbuf);
/** Write data into an MBuffer. */
int mbuf_write (MBuffer* mbuf, const uint8_t* buf, size_t len);
/** Record that more data has been written to the MBuffer */
int mbuf_write_extend (MBuffer* mbuf, size_t len);
/**  Append the printed string described by format to the MBuffer. */
int mbuf_print(MBuffer* mbuf, const char* format, ...);

/** Prepare to read a message from an MBuffer. */
int mbuf_begin_read (MBuffer* mbuf);
/** Reset the read pointer to the beginning of the message. */
int mbuf_reset_read (MBuffer* mbuf);
/** Read bytes from an MBuffer. */
int mbuf_read (MBuffer* mbuf, uint8_t* buf, size_t len);
int mbuf_read_skip (MBuffer* mbuf, size_t len);
/** Read a single byte from an MBuffer. */
int mbuf_read_byte (MBuffer* mbuf);
/** Find a byte in an MBuffer. */
size_t mbuf_find (MBuffer* mbuf, uint8_t c);
/** Find the first byte in an MBuffer different from argument. */
size_t mbuf_find_not (MBuffer* mbuf, uint8_t c);

/** Advance to a new message for reading. */
int mbuf_consume_message (MBuffer* mbuf);
/** Advance the message pointer, if room. */
void mbuf_message_start_advance (MBuffer *mbuf, size_t n);
/** Repack an MBuffer so the next data to read is at the start of the allocated memory. */
int mbuf_repack (MBuffer* mbuf);
/** Repack an MBuffer to start with the current message, keeping R/W pointers. */
int mbuf_repack_message (MBuffer* mbuf);
/** Repack an MBuffer to start with the current message, resetting read
 * pointer. */
int mbuf_repack_message2 (MBuffer* mbuf);

/** Clear the content of an MBuffer and zero it out. */
int mbuf_clear (MBuffer* mbuf);
/** Clear the content of an MBuffer and optionaly zero it out. */
int mbuf_clear2 (MBuffer* mbuf, int zeroBuffer);

/** Concatenate content of src MBuffer into dst */
int mbuf_concat(MBuffer *src, MBuffer *dst);
/** Copy content of src MBuffer into dst MBuffer */
int mbuf_copy(MBuffer *src, MBuffer *dst);

#endif // MBUF_H__

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
