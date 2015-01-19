/*
 * Copyright 2007-2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file mstring.c
 * \brief A simple managed string abstraction which is dynamically allocated and can be transparently resized on demand which provides access to the underlying C string as required.
 *
 * MStrings encapsulate a variable amount of allocated space, and manage it
 * transparently, increasing it when needed. They are allocated with
 * mstring_create and freed with mstring_delete.  Classical manipulation such
 * as, setting, concatenating or appending a formatted string. There also are
 * low-level access function to get the nil-terminated C-string and its length.
 *
 * \see mstring_set, mstring_cat, mstring_sprintf, mstring_buf, mstring_len
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "mem.h"
#include "mstring.h"

/** Increments by which an MString's storage is increased. */
#define DEFAULT_MSTRING_SIZE 64

/** Ensure that an MString has enough space to store a string of a given
 * length.
 *
 * Storage for the nil-terminator is added internally. If no space can be
 * allocated, the previous storage is kept untouched. An additional
 * DEFAULT_MSTRING_SIZE is allocated.
 *
 * \param mstr MString to manipulate
 * \param len desired length to store
 * \return 0 if mstr now has enough space, -1 otherwise
 * \see oml_realloc, oml_free
 */
static int
mstring_ensure_space(MString *mstr, size_t len)
{
  size_t new_size;
  char *new;
  if (NULL == mstr) {
    return -1;
  }

  if (mstr->size < len + 1) { /* This means we only increase the size */
    new_size = len + DEFAULT_MSTRING_SIZE + 1;
    new = oml_realloc (mstr->buf, new_size);
    if (new == NULL) {
      return -1;
    }
    mstr->buf = new;
    mstr->size = new_size;
  }
  assert (mstr->size > len);

  return 0;
}

/** Create a new MString
 * \return a pointer to the newly allocated MString, or NULL on error
 * \see mstring_delete, mstring_ensure_space, oml_malloc, oml_free
 */
MString*
mstring_create (void)
{
  MString* mstr = oml_malloc (sizeof (MString));
  memset(mstr, 0, sizeof(MString));

  if (mstr) {
    if (mstring_ensure_space(mstr, 0) < 0) {
      oml_free (mstr);
      mstr = NULL;
    }
  }

  return mstr;
}

/** Set the content of the MString to a given C string.
 *
 * The C-string is duplicated into the MString's internal storage.
 *
 * \param mstr MString to set
 * \param str C string to copy into the MString
 * \return 0 on success, -1 otherwise
 *
 * \see mstring_ensure_space
 * \see strncpy(3)
 */
int
mstring_set (MString* mstr, const char* str)
{
  size_t len;
  if (mstr == NULL || str == NULL) {
    return -1;
  }
  len = strlen (str);

  if(mstring_ensure_space(mstr, len) < 0) {
    return -1;
  }

  strncpy (mstr->buf, str, len + 1);

  mstr->length = len;
  assert (mstr->length < mstr->size);

  return 0;
}

/** Concatenate a C string to the end of an mstring
 *
 * \param mstr MString to manipulate
 * \param str C string to concatenate into the MString
 * \return 0 on success, -1 otherwise
 *
 * \see mstring_ensure_space
 * \see strncat(3)
 */
int
mstring_cat (MString* mstr, const char* str)
{
  size_t len;
  if (mstr == NULL || str == NULL) {
    return -1;
  }
  len = strlen (str);

  if(mstring_ensure_space(mstr, mstr->length + len) < 0) {
    return -1;
  }

  strncat (mstr->buf, str, len);

  mstr->length += len;
  assert (mstr->length < mstr->size);

  return 0;
}

/** Add formatted string to the end of an MString.
 *
 * The contents of the MString is not overwritten, rather, the newly-formatted
 * string is added to the end.
 *
 * \param mstr MString to manipulate
 * \param fmt, ... sprintf(3)-style format string and arguments
 * \return 0 on success, -1 otherwise
 */
int
mstring_sprintf (MString *mstr, const char *fmt, ...)
{
  char *buf;
  size_t space;
  size_t n;
  va_list va;

  if(NULL == mstr || NULL == fmt) {
    return -1;
  }

  buf = mstr->buf + mstr->length;
  space = mstr->size - mstr->length;

  va_start (va, fmt);
  n = vsnprintf (buf, space, fmt, va);
  if (n >= space) {
    va_end (va);

    if(mstring_ensure_space(mstr, mstr->length + n) < 0) {
      return -1;
    }
    /* mstring_ensure_space's call to oml_realloc might have moved things ... */
    buf = mstr->buf + mstr->length;
    /* ... and we have more space */
    space = mstr->size - mstr->length;

    va_start (va, fmt);
    n = vsnprintf (buf, space, fmt, va);
    if (n >= space) {
      return -1;
    }
  }
  va_end (va);

  mstr->length += n;
  assert (mstr->length < mstr->size);

  return 0;
}

/** Get the current length of an MString.
 *
 * \param mstr MString to manipulate
 * \return the length of the contained C string, or 0 on error
 */
size_t
mstring_len (MString* mstr)
{
  if (mstr) {
    assert (mstr->buf && mstr->length == strlen (mstr->buf));
    return mstr->length;
  }
  return 0;
}

/** Get the C string contained in an MString
 *
 * \param mstr MString to manipulate
 * \return the C string contained in the MString, or NULL on error
 */
char*
mstring_buf (MString* mstr)
{
  if (mstr) {
    return mstr->buf;
  } else {
    return NULL;
  }
}

/** Delete an MString and free its storage
 *
 * \param mstr MString to deallocate
 * \see oml_free
 */
void
mstring_delete (MString* mstr)
{
  if (mstr) {
    oml_free (mstr->buf);
    oml_free (mstr);
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
