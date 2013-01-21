/*
 * Copyright 2010-2013 National ICT Australia (NICTA), Australia
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

#include "headers.h"
#include "mem.h"
#include "oml_util.h"

static const struct {
  const char *name;
  size_t namelen;
  enum HeaderTag tag;
} header_map [] = {
  { "protocol",      8,  H_PROTOCOL },
  { "experiment-id", 13, H_DOMAIN },
  { "content",       7,  H_CONTENT },
  { "app-name",      8,  H_APP_NAME },
  { "schema",        6,  H_SCHEMA },
  { "sender-id",     9,  H_SENDER_ID },
  { "start_time",    10, H_START_TIME }, /* This one will be deprecated at some point */
  { "start-time",    10, H_START_TIME },
  { NULL, 0, H_NONE }
};

/**
 * @brief Convert a string header name into the tag for that header.
 *
 * The +str+ should be a null terminated string.  The parameter +n+
 * sets the bounds of the string to be considered for extracting the
 * tag.  All characters from the start to the n-th are considered part
 * of the name, so any trailing whitespace should be after the n-th
 * character.
 *
 * The function takes +str+ and compares it against the list of known
 * headers.  If it finds a match, it converts the header to its
 * corresponding tag; otherwise it returns H_NONE.
 *
 * @param str -- string name to convert.
 * @param n -- the number of bytes of str to consider
 * @return -- the tag.
 */
enum HeaderTag
tag_from_string (const char *str, size_t n)
{
  int i = 0;
  char first;
  char second;

  if (str == NULL || n <= 2)
    return H_NONE;

  first = *str;
  second = *(str+1);

  /* Premature optimization for fun and profit */
  for (; header_map[i].name != NULL; i++) {
    char h_first  = header_map[i].name[0];
    char h_second = header_map[i].name[1];
    size_t h_len = header_map[i].namelen;

    if (n < h_len)
      continue;

    if (first != 's') {
      /* We can fail on mismatched first char if it's not 's' */
      if ((first == h_first) &&
          (strncmp (str+1, header_map[i].name+1, n - 1) == 0))
        return header_map[i].tag;
    } else {
      /* If 's', we can fast-fail on the second char */
      if (first == h_first && second == h_second &&
          (strncmp (str+2, header_map[i].name+2, n - 2) == 0)) {
        return header_map[i].tag;
      }
    }
  }
  return H_NONE;
}

const char *
tag_to_string (enum HeaderTag tag)
{
  int i = 0;
  for (i = 0; header_map[i].name != NULL; i++)
    if (header_map[i].tag == tag)
      return header_map[i].name;
  return NULL;
}

/**
 *  @brief Parse a protocol header line into a struct header.
 *
 *  Each header consists of a tag, followed by a colon, followed by a
 *  value, terminated by a new line character '\n'.  This function
 *  parses such a header into the tag, converted from a string into an
 *  enum HeaderTag, and the value, which is kept as a string.  They
 *  are returned as a newly allocated struct header object; the caller
 *  owns the memory and should deallocated it when it is no longer
 *  needed, using header_free().
 *
 *  Whitespace is not permitted at the start of the string, but is
 *  permitted before and after the colon ':'.  This space is stripped
 *  from the value, but trailing space is not.
 *
 *  The string str should be null-terminated, but can be longer than
 *  +n+ characters.  Characters after the n-th will be ignored.  The
 *  +value+ part of the returned +struct header+ will not have any
 *  trailing whitespace removed.
 *
 *  If the tag part is not recognized, NULL is returned.  If memory
 *  can't be allocated for the +struct header+, then NULL is returned
 *  instead.  If there is no colon ':' in the first +n+ characters of
 *  +str+, then NULL is returned.
 *
 *  @param str Input string to parse
 *  @param n Number of characters of str to parse.
 *  @return The header, or NULL if the header could not be parsed.
 */
struct header*
header_from_string (const char *str, size_t n)
{
  if (!str) return NULL;

  const char *p = str;
  const char *q = memchr (p, ':', n);
  if (!q)
    return NULL;

  /* Strip trailing whitespace on the header name */
  p = find_white(p);

  int namelen;
  if (p < q)
    namelen = p - str;
  else
    namelen = q - str;

  enum HeaderTag tag = tag_from_string (str, namelen);
  if (tag == H_NONE)
    return NULL;

  /* Skip the ':' and strip leading whitespace on header value */
  if (*q)
    q = skip_white (++q);

  char * value = NULL;
  size_t valuelen = n - (q - str);
  struct header *header = NULL;

  if ((int)valuelen > 0) {
    value = xstrndup (q, valuelen);
    if (!value)
      return NULL;

    header = xmalloc (sizeof (struct header));
    if (!header) {
      xfree (value);
      return NULL;
    }
    header->tag = tag;
    header->value = value;
    return header;
  }

  return NULL;
}

/**
 *  Free the memory of a +struct header+ object.
 */
void
header_free (struct header *header)
{
  if (header) {
    if (header->value)
      xfree (header->value);
    xfree (header);
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
