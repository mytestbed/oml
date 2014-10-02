/*
 * Copyright 2007-2014 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file string_utils.c
 * \brief Utility functions for processing strings. Contains functions to convert to/from backslash-encoded format.
 *
 * XXX: Shouldn't this code be moved into oml_utils.c?
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>

/** Remove trailing space from a string
 * \param[in,out] str nil-terminated string to chomp, with the first trailing space replaced by '\0'
 */
void chomp(char *str)
{
  char *p = str + strlen (str);

  while (p != str && isspace (*--p));

  *++p = '\0';
}

/** Template code to search for a matching condition in a string or array up
 * to a given length or nil-terminator.
 *
 * p should be of a dereferenceable type that can be compared to 0.
 *
 * If len is negative, this allows for a very long string to be parsed. Very
 * long here being entirely machine dependent, one should not really rely on
 * this...
 *
 * \param[in,out] p pointer to the beginning of the string or array; updated to the matching element, or the end of the search area if not found
 * \param cond success condition involving p
 * \param[in,out] len maximum length to search for; updated to the remaining number of characters when finishing (-1 if not found)
 */
#define find_matching(p, cond, len)  \
do {                                 \
  while (len-- && *p && !(cond)) {  \
    p++;                             \
  }                                  \
} while(0)

/** Skip whitespaces in a string
 *
 * \param p string to skip whitespace out of
 * \return a pointer to the first non-white character of the string, which might be the end of the string
 * \see isspace(3)
 */
const char* skip_white(const char *p)
{
  int l = -1;
  find_matching(p, !(isspace (*p)), l);
  return p;
}

/** Find first whitespace in a string
 *
 * \param p string to search for whitespace in
 * \return a pointer to the first whitespace character of the string, which might be the end of the string
 * \see isspace(3)
 */
const char* find_white(const char *p)
{
  int l = -1;
  find_matching(p, (isspace (*p)), l);
  return p;
}

/** Find the first occurence of a character in a string up to a nul character
 * or a given number of characters, whichever comes first,
 *
 * \param str a possibly nul-terminated string
 * \param c the character to look for
 * \param len the maximum length to search the string for
 * \return a pointer to character c in str, or NULL
 */
const char* find_charn(const char *str, char c, int len)
{
  find_matching(str, (*str == c), len);
  if (len<0 || !*str) {
    return NULL;
  }
  return str;
}

/**
 * Return the worst-case size of the buffer needed to encode a string
 * of size in_sz.
 *
 * \param in_sz The size of the input string.
 * \return The size of a buffer large enough to hold the encoded string.
 */
size_t
backslash_encode_size(size_t in_sz)
{
  return 2 * in_sz + 1;
}

/**
 * Encode "magic" characters using backslash encoding.
 *
 * The worst-case output string may be up to twice as long as the
 * input string so "out" must be large enough to accommodate that.
 *
 * \param in A non-NULL pointer to the input string.
 * \param out A non-NULL pointer to the output string.
 * \return The length of the encoded string.
 */
size_t
backslash_encode(const char *in, char *out)
{
  char *begin;
  assert(in);
  assert(out);
  assert(in != out);
  for(begin=out; *in; in++) {
    switch(*in) {
    case '\t':
      *out++ = '\\';
      *out++ = 't';
      break;
    case '\n':
      *out++ = '\\';
      *out++ = 'n';
      break;
    case '\r':
      *out++ = '\\';
      *out++ = 'r';
      break;
    case '\\':
      *out++ = '\\';
      *out++ = '\\';
      break;
    default:
      *out++ = *in;
    }
  }
  *out = '\0';
  return out - begin;
}

/**
 * Decode a backslash-encoded character string.
 *
 * Process a string replacing each backslash-encoded character pair
 * with the appropriate character. Note that the output string will
 * always be the same size, or smaller than, the input string.
 *
 * \param in A non-NULL pointer to the NUL-terminated input string.
 * \param out A non-NULL pointer to the NUL-terminated output string.
 * \return The length of the decoded string.
 */
size_t
backslash_decode(const char *in, char *out)
{
  char *begin;
  assert(in);
  assert(out);
  assert(in != out);
  for(begin=out; *in; in++) {
    if('\\' == *in) {
      switch(*++in) {
      case 't':
        *out++ = '\t';
        break;
      case 'n':
        *out++ = '\n';
        break;
      case 'r':
        *out++ = '\r';
        break;
      case '\\':
        *out++ = '\\';
        break;
      default:
        *out++ = *in;
      }
    } else {
      *out++ = *in;
    }
  }
  *out = '\0';
  return out - begin;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
