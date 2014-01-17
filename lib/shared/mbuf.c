/*
 * Copyright 2007-2014 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file mbuf.c
 * \brief Managed buffer (MBuffer) abstraction providing functions for reading, writing and memory resizing used to hold a message.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "mem.h"
#include "mbuf.h"

#define DEF_BUF_SIZE 512
#define DEF_MIN_BUF_RESIZE (size_t)(0.1 * DEF_BUF_SIZE)

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

/** Create an MBuf with the default parameters.
 * \return a pointer to the newly allocated MBuffer
 * \see mbuf_create2, DEF_BUF_SIZE, DEF_MIN_BUF_RESIZE
 */
MBuffer*
mbuf_create (void)
{
  return mbuf_create2(DEF_BUF_SIZE, DEF_MIN_BUF_RESIZE);
}

/** Create an MBuf.
 *
 * If buffer_length or min_resize are negative or NULL, the default values are used instead.
 *
 * \param buffer_length initial memory allocation length
 * \param min_resize minimun length by which the buffer should be grown when it is
 * \return a pointer to the newly allocated MBuffer
 * \see DEF_BUF_SIZE, DEF_MIN_BUF_RESIZE
 */
MBuffer*
mbuf_create2 (size_t buffer_length, size_t min_resize)
{
  MBuffer* mbuf = oml_malloc (sizeof (MBuffer));
  if (mbuf == NULL) return NULL;

  mbuf->length = buffer_length > 0 ? buffer_length : DEF_BUF_SIZE;
  mbuf->min_resize = min_resize > 0 ? min_resize : DEF_MIN_BUF_RESIZE;
  mbuf->base = oml_malloc (mbuf->length);

  if (mbuf->base == NULL) {
    oml_free (mbuf);
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

/** Destroy an MBuffer and its storage.
 *
 * Frees both the MBuffer object and its allocated buffer.
 *
 * \param mbuf MBuffer to free
 */
void
mbuf_destroy (MBuffer* mbuf)
{
  if(!mbuf)
    return;

  logdebug("Destroying MBuffer %p\n", mbuf);

  oml_free (mbuf->base);
  oml_free (mbuf);
}

/** Get an MBuffer's storage.
 *
 * \param mbuf MBuffer to manipulate
 * \return a pointer to mbuf's memory buffer
 */
uint8_t*
mbuf_buffer (MBuffer* mbuf)
{
  return mbuf->base;
}

/** Get the length of the MBuffer's allocated buffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the allocated size as an size_t
 */
size_t
mbuf_length (MBuffer* mbuf)
{
  return mbuf->length;
}

/** Get the remaining amount of data to read in MBuffer
 * XXX: Use mbuf_rd_remaining instead
 */
inline size_t
mbuf_remaining (MBuffer* mbuf)
{
  return mbuf_rd_remaining(mbuf);
}

/** Get the remaining amount of data to read in MBuffer
 *
 * XXX: Shall we use rd_remaining directly?
 *
 * \param mbuf MBuffer to manipulate
 * \return the number of unread bytes in the buffer
 */
size_t
mbuf_rd_remaining (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->rdptr;
}

/** Get the remaining amount of data to write in MBuffer
 *
 * XXX: Shall we use wr_remaining directly?
 *
 * \param mbuf MBuffer to manipulate
 * \return the number of unread bytes in the buffer
 */
size_t
mbuf_wr_remaining (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->rdptr;
}


/** Get the currently occupied size of the MBuffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the stored data size as an size_t
 */
size_t
mbuf_fill (MBuffer* mbuf)
{
  return mbuf->fill;
}

/** Get the currently occupied size of the MBuffer, not including the message
 * currently being written.
 *
 * \param mbuf MBuffer to manipulate
 * \return the stored data size as an size_t, or 0 if a message is currently being read
 */
size_t
mbuf_fill_excluding_msg (MBuffer* mbuf)
{
  if (mbuf->msgptr > mbuf->rdptr) /* message message being written */
    return mbuf->msgptr - mbuf->rdptr;
  return 0; /* message being read; don't know how long it will be */
}

/** Get the read offset from the base of the MBuffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the stored data size as an size_t
 */
size_t
mbuf_read_offset (MBuffer* mbuf)
{
  return mbuf->rdptr - mbuf->base;
}

/** Get the write offset from the base of the MBuffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the stored data size as an size_t
 */
size_t
mbuf_write_offset (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->base;
}

/** Get the message offset from the base of the MBuffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the stored data size as an size_t
 */
size_t
mbuf_message_offset (MBuffer* mbuf)
{
  return mbuf->msgptr - mbuf->base;
}

/** Get the current message's index.
 *
 * \param mbuf MBuffer to manipulate
 * \return the difference between rdptr and msgptr
 */
size_t
mbuf_message_index (MBuffer* mbuf)
{
  return mbuf->rdptr - mbuf->msgptr;
}

/** Get the current message's length.
 *
 * \param mbuf MBuffer to manipulate
 * \return the difference between wrptr and msgptr
 */
size_t
mbuf_message_length (MBuffer* mbuf)
{
  return mbuf->wrptr - mbuf->msgptr;
}

/** Advance the message pointer, if room.
 *
 * XXX: Fails silently otherwise.
 *
 * \param mbuf MBuffer to manipulate
 * \param n the length by which the pointer is advanced
 */
void
mbuf_message_start_advance (MBuffer *mbuf, size_t n)
{
  if (mbuf && n < mbuf->fill - mbuf_message_offset (mbuf)) {
    mbuf->msgptr += n;
  }
}

/** Get the current message.
 *
 * \param mbuf MBuffer to manipulate
 * \return a pointer to the beginning of the current message (msgptr)
 */
uint8_t*
mbuf_message (MBuffer* mbuf)
{
  return mbuf->msgptr;
}

/** Get the read pointer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the read pointer rdptr
 */
uint8_t*
mbuf_rdptr (MBuffer* mbuf)
{
  return mbuf->rdptr;
}

/** Get the write pointer.
 *
 * \param mbuf MBuffer to manipulate
 * \return the write pointer wrptr
 */
uint8_t*
mbuf_wrptr (MBuffer* mbuf)
{
  return mbuf->wrptr;
}

/** Resize an MBuffer's allocated storage.
 *
 * If new_length is smaller than that of the current storage, do nothing.
 *
 * If new_length is larger than the current storage by less than the min_resize
 * specified for the MBuffer, the allocated memory is grown by min_resize.
 * (XXX: This behaviour is currently disabled).
 *
 * \param mbuf MBuffer to manipulate
 * \param new_length length to which the buffer should be grown
 * \return 0 on success, or -1 on error
 * \see mbuf_create, mbuf_create2, DEF_MIN_BUF_RESIZE
 */
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

  uint8_t* new = oml_realloc (mbuf->base, new_length);
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

/** Resize an MBuffer if it cannot contain the specified amount of data.
 *
 * \param mbuf MBuffer to manipulate
 * \param bytes amount of data which need to be stored into the MBuffer
 * \return 0 on success, or -1 on error
 * \see mbuf_resize
 */
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

/** Write data into an MBuffer.
 *
 * Write len bytes from the raw buffer pointed to by buf into the MBuffer.  If the
 * MBuffer doesn't have enough remaining space to fit len bytes, nothing is
 * written and the function fails.
 *
 * If the function succeeds, the data is written starting at the MBuffer's write
 * pointer, and the write pointer is advanced by len bytes.
 *
 * \param mbuf MBuffer to write data into
 * \param buf data to write
 * \param len length of the buffer of data (not including \0 for strings)
 * \return 0 on success, -1 on failure.
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

/**  Append the printed string described by format to the MBuffer.
 *
 * Write the string described by a format string and arguments to the MBuffer,
 * resizing it if need be.
 *
 * If the function succeeds, the string is written starting at the MBuffer's
 * write pointer, and the write pointer is advanced by the size of the string
 * (excluding the '\0').
 *
 * NOTE: At the end, the write pointer points to the terminating NULL character
 * of the string.
 *
 * \param mbuf MBuffer to write the string to
 * \param format format string
 * \param ... arguments for the format string
 * \return 0 on success, -1 on failure
 * \see vsnprintf(3)
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

  /* XXX: should we advance by one more to keep the '\0'? */
  mbuf->wrptr += len;
  mbuf->fill += len;
  mbuf->wr_remaining -= len;
  mbuf->rd_remaining += len;

  mbuf_check_invariant (mbuf);
  return 0;
}

/** Read bytes from an MBuffer.
 *
 * Read len bytes from the MBuffer into the raw buffer pointed to by buf. If
 * the MBuffer doesn't have at least len bytes available to read, the function
 * fails.
 *
 * If the function succeeds, the bytes are written to buf, with reading
 * starting at the MBuffer's read pointer, and the read pointer is then
 * advanced by len bytes.
 *
 * \param mbuf MBuffer to read from
 * \param buf buffer into which the read data should be put
 * \param len size of data to read
 * \return 0 on success, -1 on failure
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

/** Skip some data to be read from an MBuffer.
 * 
 * This simply advances the read pointer.
 *
 * \param mbuf MBuffer to manipulate
 * \param len amount of data to skip
 * \return 0 on success, or -1 otherwise
 */
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

/** Read a single byte from an MBuffer.
 *
 * \param mbuf MBuffer to read from
 * \return the read byte, or -1 if no data was available
 */
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

/** Find a byte in an MBuffer.
 *
 * \param mbuf MBuffer to search
 * \param c byte to search for
 * \return offset from the read pointer to the first matching byte, or -1 if not found
 */
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

/** Find the first byte in an MBuffer different from argument.
 *
 * \param mbuf MBuffer to search
 * \param c byte to *not* find
 * \return offset from the read pointer to the first non-matching byte, or -1 if not found
 */
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

/** Prepare to read a message from an MBuffer.
 *
 * Adjust the msgptr to the current rdptr.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 * \see mbuf_message, mbuf_message_length, mbuf_consume_message, mbuf_repack_message, mbuf_repack_message2
 */
int
mbuf_begin_read (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  mbuf->msgptr = mbuf->rdptr;

  mbuf_check_invariant (mbuf);

  return 0;
}

/** Prepare to write a new message to an MBuffer.
 *
 * Adjust the msgptr to the current wrptr.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 * \see mbuf_message, mbuf_consume_message, mbuf_repack_message, mbuf_repack_message2
 */
int
mbuf_begin_write (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  mbuf->msgptr = mbuf->wrptr;

  mbuf_check_invariant (mbuf);

  return 0;
}

/** Clear the content of an MBuffer and zero it out.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 otherwise
 * \see mbuf_clear2
 */
int
mbuf_clear (MBuffer* mbuf)
{
  return mbuf_clear2(mbuf, 1);
}

/** Clear the content of an MBuffer and optionaly zero it out.
 *
 * Resets the read, write and message pointers to the base.
 *
 * \param mbuf MBuffer to manipulate
 * \param zero_buffer non-zero if the memory must be actively cleared
 * \return 0 on success, -1 otherwise
 */
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

/** Reset the write pointer to the beginning of the message.
 *
 * Adjust the wrptr to the current msgptr.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 */
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

/** Reset the read pointer to the beginning of the message.
 *
 * Adjust the rdptr to the current msgptr.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 */
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

/** Advance to a new message.
 *
 * This is syntactic sugar to be called when all of a message has been
 * processed by the caller, to advance the message pointer to the current read
 * pointer iff it is behind (i.e., not writing). 
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 otherwise
 * \see mbuf_begin_read
 */
int
mbuf_consume_message (MBuffer* mbuf)
{
  mbuf_check_invariant (mbuf);

  if (mbuf == NULL) return -1;

  // Can't consume message if we're in the middle of writing it
  if (mbuf->msgptr > mbuf->rdptr)
    return -1;

  mbuf->msgptr = mbuf->rdptr; /* Not using mbuf_begin_read to avoid redundant calls to mbuf_check_invariant */

  mbuf_check_invariant (mbuf);
  return 0;
}

/** Repack an MBuffer so the next data to read is at the start of the allocated
 * memory.
 *
 * This adjust the read and write pointers accordingly, and resets the message
 * pointer to the base of the allocated memory.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 otherwise
 * \see mbuf_repack_message, mbuf_repack_message2
 */
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

/** Repack an MBuffer to start with the current message, keeping R/W pointers.
 *
 * The read pointer is preserved so it points to the same location of the
 * message after repacking, while the write pointer points right after the end
 * of the data still to read in the buffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 */
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

/** Repack an MBuffer to start with the current message, resetting read
 * pointer.
 *
 * The read pointer is reset to point at the beginning of the message, while
 * the write pointer points to the end of the data still to read in the buffer.
 *
 * \param mbuf MBuffer to manipulate
 * \return 0 on success, -1 on failure
 */
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
