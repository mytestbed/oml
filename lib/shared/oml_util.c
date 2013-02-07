/*
 * Copyright 2007-2013 National ICT Australia (NICTA), Australia
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
#include <stdio.h> // For snprintf
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "oml_util.h"

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
 * \param[in,out] p pointer to the beginning of the string or array; updated to the matching element, or NULL if not found
 * \param cond success condition involving p
 * \param[in,out] len maximum length to search for; updated to the remaining number of characters when finishing (found or not)
 */
#define find_matching(p, cond, len)  \
do {                              \
  while (p && !(cond)) {          \
    if (!--len || *p == 0) {      \
      p = NULL;                   \
    } else {                      \
      p++;                        \
    }                             \
  }                               \
} while(0)

/** Skip whitespaces in a string
 *
 * \param p string to skip whitespace out of
 * \return a pointer to the first non-white character of the string, or NULL if p is entirely composed of whitespaces
 * \see isspace(3)
 */
const char* skip_white(const char *p)
{
  int l = -1;
  find_matching(p, !(isspace (*p)), l);
  if (*p == '\0') p = NULL;
  return p;
}

/** Find first whitespace in a string
 *
 * \param p string to search for whitespace in
 * \return a pointer to the first whitespace character of the string, or NULL if p does not contain whitspaces
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
  return str;
}

/** Dump the contents of a buffer as a string of hex characters
 *
 * \param buf array of data to dump
 * \param len length of buf
 * \return a character string which needs to be xfree()'d
 *
 * \see xmalloc, xfree
 */
char* to_octets(unsigned char *buf, int len)
{
  const int octet_width = 3;
  const int columns = 16;
  const int rows = len / columns + 2; /* Integer division, and first line */
  char *out = xmalloc (1 + rows * (octet_width * columns + 8)); /* Each row has 8 non-data characters (numbers and spaces) */
  int n = 0, i, col, rw=0;

  if(out) {
    /* Don't forget nil-terminator in snprintf's size */
    n += snprintf(out, columns * octet_width + 9, "   0 1 2 3  4 5 6 7   8 9 a b  c d e f\n%2x ", rw++);
    for (i = 0, col = 1; i < len; i++, col++) {
      n += snprintf(&out[n], octet_width + 1, "%02x", (unsigned int)buf[i]);

      /* Add some spacing for readability */
      if (col >= columns) {
        n += snprintf(&out[n], 5, "\n%2x ", rw++);
        col = 0;
      } else if (col > 0) {
        if(0 == (col % 8)) {
          n += snprintf(&out[n], 3, "  ");
        } else if (0 == (col % 4)) {
          n += snprintf(&out[n], 2, " ");
        }
      }
    }
  }

  return out;
}

/** Resolve a string containing a service or port into an integer port.
 *
 * Setting default to an impossible value (say, -1) is sufficient to catch resolving errors;
 *
 * \param port string containing the textual service name or port
 * \param defport default port value to return if resolving fails
 * \return the port number as a host-order integer, or the default value if something failed
 * \see getservbyname(3)
 */
int resolve_service(const char *service, int defport)
{
  struct servent *sse;
  char *endptr;
  int portnum = defport, tmpport;

  sse = getservbyname(service, NULL);
  if (sse) {
    portnum = ntohs(sse->s_port);
  } else {
    tmpport = strtol(service, &endptr, 10);
    if (endptr > service)
      portnum = tmpport;
    else
      logwarn("Could not resolve service '%s', defaulting to %d\n", service, portnum);
  }

  return portnum;
}

/**
 * Parse the scheme of na URI and return its type as an +OmlURIType+
 * \param [in] uri the URI to parse
 * \return an +OmlURIType+ indicating the type of the URI
 */
OmlURIType oml_uri_type(const char* uri) {
  int len = strlen(uri);

  if(len>5 && !strncmp(uri, "flush", 5))
    return OML_URI_FILE_FLUSH;
  else if(len>4 && !strncmp(uri, "file", 4))
    return OML_URI_FILE;
  else if(len>3 && !strncmp(uri, "tcp", 3))
      return OML_URI_TCP;
  else if(len>3 && !strncmp(uri, "udp", 3))
      return OML_URI_UDP;
  return OML_URI_UNKNOWN;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
