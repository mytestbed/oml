/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#include <assert.h>
#include <stdint.h>
#include <string.h>

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
