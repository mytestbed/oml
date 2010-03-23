/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "mstring.h"

#define DEFAULT_MSTRING_SIZE 64

MString*
mstring_create (void)
{
  MString* mstr = (MString*)malloc (sizeof (MString));

  if (mstr)
	{
	  mstr->buf = (char*)malloc (DEFAULT_MSTRING_SIZE * sizeof (char));
	  if (mstr->buf)
		{
		  mstr->size = DEFAULT_MSTRING_SIZE;
		  mstr->length = 0;
		  memset (mstr->buf, 0, mstr->size);

		  return mstr;
		}
	  else
		{
		  free (mstr);
		  return NULL;
		}
	}
  else
	return NULL;
}

int
mstring_set (MString* mstr, const char* str)
{
  if (mstr == NULL || str == NULL) return -1;
  size_t len = strlen (str);

  if (mstr->size < len + 1)
	{
	  size_t new_size = len + DEFAULT_MSTRING_SIZE + 1;
	  char* new = (char*) malloc (new_size * sizeof (char));
	  if (new == NULL) return -1;

	  memset (new, 0, new_size);
	  free (mstr->buf);
	  mstr->buf = new;
	  mstr->size = new_size;
	}

  assert (mstr->size > len);

  strncpy (mstr->buf, str, len + 1);
  mstr->length = len;

  assert (mstr->length < mstr->size);

  return 0;
}

int
mstring_cat (MString* mstr, const char* str)
{
  if (mstr == NULL || str == NULL) return -1;
  size_t len = strlen (str);

  if (mstr->size < mstr->length + len + 1)
	{
	  size_t new_size = mstr->size + len + DEFAULT_MSTRING_SIZE + 1;
	  char* new = (char*) malloc (new_size * sizeof (char));
	  if (new == NULL) return -1;

	  assert (new_size > mstr->length);

	  memset (new, 0, new_size);
	  strncpy (new, mstr->buf, new_size);

	  free (mstr->buf);
	  mstr->buf = new;
	  mstr->size = new_size;
	}

  assert (mstr->size > mstr->length + len);

  strncat (mstr->buf, str, len);
  mstr->length += len;

  assert (mstr->length < mstr->size);

  return 0;
}

size_t
mstring_len (MString* mstr)
{
  if (mstr)
	{
	  assert (mstr->buf && mstr->length == strlen (mstr->buf));
	  return mstr->length;
	}
  else
	return 0;
}

char*
mstring_buf (MString* mstr)
{
  if (mstr)
	return mstr->buf;
  else
	return NULL;
}

void
mstring_delete (MString* mstr)
{
  if (mstr)
	{
	  free (mstr->buf);
	  free (mstr);
	}
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
