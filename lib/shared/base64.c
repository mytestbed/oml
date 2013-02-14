/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#include "base64.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>

/**
 * The character set used for BASE64 encoding and decoding.
 */
static const char *BASE64 = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/**
 * Return the exact size of the string buffer needed to hold a
 * BASE64-encoded blob of blob_sz bytes.
 *
 * The string size is given by the equation size = 4 * &lceil;
 * blob_sz / 3 &rceil; for the buffer size 1 extra byte added for the
 * terminating NUL.
 *
 * \param blob_sz The size of the blob to encode.
 * \return The size of the BASE64 string buffer to hold the blob.
 */
size_t
base64_size_string(size_t blob_sz)
{
  return 4 * ((blob_sz + 2) / 3) + 1;
}

/**
 * Encode a blob as a string using BASE64 encoding.
 *
 * Note that the output string buffer must be large enough to hold the
 * BASE64-encoded string plus the NUL terminator. You should determine
 * the exact size for the buffer by calling the
 * base64_size_string(size_t) function.
 *
 * \param blob_sz The size of the input blob.
 * \param blob A pointer to the input blob.
 * \param s A non-NULL pointer to the output string buffer.
 * \return The size of the encoded string.
 */
size_t
base64_encode_blob(size_t blob_sz, const void *blob, char *s)
{
  size_t i;
  uint32_t x;
  char *begin = s;
  const uint8_t *p;  
  assert(0 == blob_sz || blob);
  assert(s);
  p = blob;
  begin = s;
  for(i = 0; i < blob_sz; i += 3) {
    switch(blob_sz - i) {
    case 1:
      x = p[i] << 16;
      *s++ = BASE64[x >> 18 & 0x3f];
      *s++ = BASE64[x >> 12 & 0x3f];
      *s++ = '=';
      *s++ = '=';
      break;
    case 2:
      x = p[i] << 16 | p[i+1] << 8;
      *s++ = BASE64[x >> 18 & 0x3f];
      *s++ = BASE64[x >> 12 & 0x3f];
      *s++ = BASE64[x >> 6 & 0x3f];
      *s++ = '=';
      break;
    default:
      x = p[i] << 16 | p[i+1] << 8 | p[i+2];
      *s++ = BASE64[x >> 18 & 0x3f];
      *s++ = BASE64[x >> 12 & 0x3f];
      *s++ = BASE64[x >> 6 & 0x3f];
      *s++ = BASE64[x & 0x3f];
      break;
    }
  }
  *s = '\0';
  return s - begin;
}

/**
 * Validate a BASE64-encoded string.
 *
 * A properly formed string will have a length which is a multiple of
 * 4 bytes and all characters will be drawn from the BASE64 character
 * set. Note that the empty string is considered to be a valid BASE64
 * string.
 *
 * \param s A non-NULL pointer to the BASE64-encoded string.
 * \return If valid the length of the string minus any padding; otherwise -1.
 */
ssize_t
base64_validate_string(const char *s)
{
  size_t s_sz;
  assert(s);
  s_sz = strlen(s);
  if(s_sz % 4 == 0) {
    size_t n = strspn(s, BASE64);
    if((s_sz == n) ||
      (s_sz - 1 == n && '=' == s[n]) ||
      (s_sz - 2 == n && '=' == s[n] && '=' == s[n+1])) {
      return n;
    }
  }
  return -1;
}

/**
 * Returns the size of a blob which is encoded using BASE64.
 *
 * This function takes the *unpadded* string size (as returned by
 * base64_validate_string(const char*). The size is given by the
 * equation size = in_sz * 3 / 4.
 * 
 * \param s_sz The size of the BASE64-encoded string.
 * \return The size of the blob.
 */
size_t
base64_size_blob(size_t s_sz)
{
  return s_sz * 3 / 4;
}

/**
 * Decode a BASE64-encoded string into a blob.
 *
 * You *must* call base64_validate_string(const char*) on the string
 * *before* calling this function (it will tell you the correct value
 * for s_sz and ensures the string is valid).
 *
 * \param s_sz The size of the *unpadded* BASE64-encoded string.
 * \param s A pointer to the BASE64-encoded string.
 * \param blob_sz The size of the output buffer.
 * \param blob A pointer to the blob.
 * \return The blob size or -1 if an error occurred.
 */
ssize_t
base64_decode_string(size_t s_sz, const char *s, size_t blob_sz, void *blob)
{
  uint32_t x;
  size_t i, j;
  uint8_t *p, *begin;

  static const int8_t DECODE[] = {
    -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, -1, -1, -1, -1, -1, 
    -1, -1, -1, 62, -1, -1, -1, 63, 
    52, 53, 54, 55, 56, 57, 58, 59, 
    60, 61, -1, -1, -1, -1, -1, -1, 
    -1,  0, 1,   2,  3, 4,   5,  6, 
     7,  8, 9, 10, 11, 12, 13, 14, 
    15, 16, 17, 18, 19, 20, 21, 22, 
    23, 24, 25, -1, -1, -1, -1, -1, 
    -1, 26, 27, 28, 29, 30, 31, 32, 
    33, 34, 35, 36, 37, 38, 39, 40, 
    41, 42, 43, 44, 45, 46, 47, 48, 
    49, 50, 51, -1, -1, -1, -1, -1, 
  };


  assert(s_sz == 0 || s);
  assert(blob_sz == 0 || blob);
  p = begin = blob;
  for(i = 0; i < s_sz; i += 4) {
    x = 0;
    for(j = i; j < i + 4 &&  j < s_sz; j++) {
      char c = s[j];
      if(!(0 <= c && c < sizeof(DECODE)))
        return -1;
      else if(-1 == DECODE[c])
        return -1;
      x <<= 6;
      x |=  DECODE[c];
    }
    switch(j % 4) {
    case 0:
      *p++ = x >> 16;
      *p++ = x >> 8;
      *p++ = x;
      break;
    case 3:
      *p++ = x >> 10;
      *p++ = x >> 2;
      break;
    case 2:
      *p++ = x >> 4;
      break;
    case 1:
      return -1;
    }
  }
  return p - begin;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
