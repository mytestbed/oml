/*
 * Copyright 2007-2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file oml_utils.c
 * \brief Various utility functions, mainly strings and memory-buffer related.
 */
#include <stdio.h> // For snprintf
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/time.h>
#include <time.h>
#include <assert.h>
#include <regex.h>

#include "ocomm/o_log.h"
#include "mem.h"
#include "string_utils.h"
#include "oml_utils.h"

/** Regular expression for URI parsing.
 *  Adapted from RFC 3986, Appendix B to allow missing '//' before the authority, separate port and host,
 *  allow bracketted IPs, and be more specific on schemes */
#define URI_RE "^(((zlib\\+|gzip\\+)?(tcp|(flush)?file)):)?((//)?(([a-zA-Z0-9][-0-9A-Za-z+.]+|\\[[0-9a-fA-F:.]+])(:([0-9]+))?))?([^?#]*)(\\?([^#]*))?(#(.*))?"
/*               123                 4    5                67    89                                                a b            c       d   e        f 10
 *                `scheme                                        |`host                                              `port        `path       `query     `fragment
 *                                                               `authority
 */
/** Number of parenthesised groups in URI_RI */
#define URI_RE_NGROUP (0xD)
/* Offsets in the list of matched substring */
#define URI_RE_SCHEME 2
#define URI_RE_AUTHORITY_WITH_SLASHES 6
#define URI_RE_AUTHORITY 8
#define URI_RE_HOST 9
#define URI_RE_PORT 0xB
#define URI_RE_PATH 0xC
#define URI_RE_QUERY 0xE
#define URI_RE_FRAGMENT 0x10

/** Dump the contents of a buffer as a string of hex characters
 *
 * \param buf array of data to dump
 * \param len length of buf
 * \return a character string which needs to be oml_free()'d
 *
 * \see oml_malloc, oml_free
 */
char* to_octets(unsigned char *buf, int len)
{
  const int octet_width = 2;
  const int columns = 16;
  len = (len>0xff)?0xff:len; /* Limit the output to something manageable */
  const int rows = len / columns + 2; /* Integer division plus first line */
  /* Each row has 7 non-data characters (numbers and spaces), one more space, and columns*ASCII characters, plus a '\n' */
  const int rowlength = (octet_width * columns + 7 + 1 + columns + 1);
  const int outlength = rows * rowlength + 1;
  char *out = oml_malloc (outlength);
  char strrep[columns + 1];
  int n = 0, i, col=0, rw=0;

  strrep[columns] = 0;

  if(out) {
    /* Don't forget nil-terminator in snprintf's size */
    n += snprintf(out, outlength - n, "   0 1 2 3  4 5 6 7   8 9 a b  c d e f  0123456789abcdef\n%2x ", rw++);
    for (i = 0; i < len; i++) {
      col = i % columns;

      if (i == 0) {
        while(0); /* Do nothing */

      } else if (col == 0) {
        n += snprintf(&out[n], outlength - n,  " %s\n%2x ", strrep, rw++);

        /* Add some spacing for readability */
      } else if(0 == (col % 8)) {
        n += snprintf(&out[n], outlength - n, "  ");

      } else if (0 == (col % 4)) {
        n += snprintf(&out[n], outlength - n, " ");
      }

      n += snprintf(&out[n], outlength - n, "%02x", (unsigned int)buf[i]);
      if (isprint(buf[i])) {
        strrep[col] = buf[i];
      } else {
        strrep[col] = '.';
      }
    }
    if(col != 0) {
      while(++col<columns) {
        /* Add padding to align ASCII output */
        if (0 == (col % 8)) {
          n += snprintf(&out[n], outlength - n, "    ");
        } else if (0 == (col % 4)) {
          n += snprintf(&out[n], outlength - n, "   ");
        } else {
          n += snprintf(&out[n], outlength - n, "  ");
        }
      }
      strrep[col] = 0;
      n += snprintf(&out[n], outlength - n, " %s", strrep);
    }
  }
  out[outlength - 1] = 0;

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

/** Parse the scheme of an URI and return its type as an +OmlURIType+
 *
 * \param [in] uri the URI to parse
 * \return an +OmlURIType+ indicating the type of the URI
 * \see oml_uri_is_file, oml_uri_is_network
 */
OmlURIType oml_uri_type(const char* uri) {
  size_t len;
  const char *next_scheme, *colon;
  int ret=OML_URI_UNKNOWN;

  if (!uri) {
    return OML_URI_UNKNOWN;
  }

  len = strlen(uri);

#define URI_MATCH(uri, match) ((len>=sizeof(match)-1) && !strncmp(uri, match, sizeof(match)-1))

  if(URI_MATCH(uri, "flush")) { /* len == 5 */
    ret = OML_URI_FILE_FLUSH;

  } else if(URI_MATCH(uri, "file")) { /* len == 4 */
    ret = OML_URI_FILE;

  } else if(URI_MATCH(uri, "gzip")) {
    ret = OML_URI_GZIP;

  } else if(URI_MATCH(uri, "zlib")) {
    ret = OML_URI_ZLIB;

  } else if(URI_MATCH(uri, "tcp")) { /* len == 3 */
    ret = OML_URI_TCP;

  } else if(URI_MATCH(uri, "udp")) {
    ret = OML_URI_UDP;
  }

#undef URI_MATCH

  if (ret != OML_URI_UNKNOWN) {
    /* XXX: we re-search the colon in each recursive iteration.
     * This is inefficient, and could be avoided by passing the length
     * information to a recursive helper for oml_uri_type.
     * Here, we assume that we don't do this often, so we don't care.*/
    colon = find_charn(uri, ':', len) + 1;
    if ((next_scheme = find_charn(uri, '+', colon?(colon-uri):len))) {
      ret |= oml_uri_type(next_scheme+1);
    }
  }

  return ret;
}

/** Test OmlURIType as a file URI
 * \param t OmlURIType
 * \return 1 if t is a file type (file or flushfile schemes), 0 otherwise
 * \see oml_uri_type
 */
inline int oml_uri_is_file(OmlURIType t) {
  return (t & OML_URI_FILE) != 0;
}

/** Test OmlURIType as a network URI
 * \param t OmlURIType
 * \return 1 if t is a network type (tcp or udp schemes), 0 otherwise
 * \see oml_uri_type
 */
inline int oml_uri_is_network(OmlURIType t) {
  return (t & OML_URI_NET) != 0;
}

/** Test OmlURIType as a compressed URI
 * \param t OmlURIType
 * \return 1 if t is a network type (zlib or gzip schemes), 0 otherwise
 * \see oml_uri_type
 */
inline int oml_uri_is_compressed(OmlURIType t) {
  return (t & OML_URI_COMPRESSED) != 0;
}

/** Parse a collection URI of the form [scheme:][host[:port]][/path].
 *
 * Either host or path are mandatory.
 *
 * If under-qualified, the URI scheme is assumed to be 'tcp', the port '3003',
 * and the rest is used as the host; path is invalid for a tcp URI (only valid
 * for file).
 *
 * \param uri string containing the URI to parse
 * \param scheme pointer to be updated to a string containing the selected scheme, to be oml_free()'d by the caller
 * \param host pointer to be updated to a string containing the host, to be oml_free()'d by the caller
 * \param port pointer to be updated to a string containing the port, to be oml_free()'d by the caller
 * \param path pointer to be updated to a string containing the path, to be oml_free()'d by the caller
 * \return 0 on success, -1 otherwise
 *
 * \see oml_strndup, oml_free, oml_uri_type, URI_RE, URI_RE_NGROUP
 */
int
parse_uri (const char *uri, const char **scheme, const char **host, const char **port, const char **path)
{
  static regex_t preg;
  static int preg_valid = 0;
  char error[80];
  size_t nmatch = URI_RE_NGROUP+1;
  regmatch_t pmatch[nmatch];
  int bracket_offset = 0;
  int ret;
  char *str;
  int len, authlen;

  assert(scheme);
  assert(host);
  assert(port);
  assert(path);

  *scheme = *host = *port = *path = NULL;

  /* XXX: static so we only compile it once, but we cannot free it */
  if (!preg_valid) {
    if((ret = regcomp(&preg, URI_RE, REG_EXTENDED))) {
      regerror(ret , &preg, error, sizeof(error));
      logerror("Unable to compile RE /%s/ for URI parsing: %s\n", URI_RE, error);
      return -1;
    }
    preg_valid = 1;
  }

  if ((ret = regexec(&preg, uri, nmatch, pmatch, 0))) {
      regerror(ret , &preg, error, sizeof(error));
      logerror("Unable to match uri '%s' against RE /%s/: %s\n", uri, URI_RE, error);
      return -1;
  }

  logdebug("URI '%s' parsed as scheme: '%s', host: '%s', port: '%s', path: '%s'\n",
      uri,
      (pmatch[URI_RE_SCHEME].rm_so>=0)?(uri+pmatch[URI_RE_SCHEME].rm_so):"(n/a)",
      (pmatch[URI_RE_HOST].rm_so>=0)?(uri+pmatch[URI_RE_HOST].rm_so):"(n/a)",
      (pmatch[URI_RE_PORT].rm_so>=0)?(uri+pmatch[URI_RE_PORT].rm_so):"(n/a)",
      (pmatch[URI_RE_PATH].rm_so>=0)?(uri+pmatch[URI_RE_PATH].rm_so):"(n/a)"
      );

  if (pmatch[URI_RE_SCHEME].rm_so >= 0) {
    *scheme = oml_strndup(uri+pmatch[URI_RE_SCHEME].rm_so,
        pmatch[URI_RE_SCHEME].rm_eo - pmatch[URI_RE_SCHEME].rm_so);
  }
  if (pmatch[URI_RE_HOST].rm_so >= 0) {
    /* Host may contain bracketted IP addresses, clean that up. */
    if ('[' == *(uri + pmatch[URI_RE_HOST].rm_so)) {
      if (']' != *(uri + pmatch[URI_RE_HOST].rm_eo - 1)) {
        logerror("Unbalanced brackets in host part of '%s' (%d %d): %c\n",
            uri,
            pmatch[URI_RE_HOST].rm_so,
            pmatch[URI_RE_HOST].rm_eo,
            uri + pmatch[URI_RE_HOST].rm_eo - 1);
        return -1;
      }
      bracket_offset += 1;
    }
    *host = oml_strndup(uri+pmatch[URI_RE_HOST].rm_so + bracket_offset,
        pmatch[URI_RE_HOST].rm_eo - pmatch[URI_RE_HOST].rm_so - 2 * bracket_offset);
  }
  if (pmatch[URI_RE_PORT].rm_so >= 0) {
    *port = oml_strndup(uri+pmatch[URI_RE_PORT].rm_so,
        pmatch[URI_RE_PORT].rm_eo - pmatch[URI_RE_PORT].rm_so);
  }
  if (pmatch[URI_RE_PATH].rm_so >= 0) {
    *path= oml_strndup(uri+pmatch[URI_RE_PATH].rm_so,
        pmatch[URI_RE_PATH].rm_eo - pmatch[URI_RE_PATH].rm_so);
    /*if(!strncmp(*path, "::", 2) ||
        oml_uri_is_network(oml_uri_type(*path))) {
      logwarn("Parsing URI '%s' as 'file:%s'\n", uri, *path);
    }*/
  }

  /* Fixup parsing inconsistencies (mainly due to optionality of //) so we don't break old behaviours */
  if (!(*scheme)) {
    *scheme = oml_strndup("tcp", 3);
  }
  /* FIXME: return port for zlib if not specified */
  if(oml_uri_is_network(oml_uri_type(*scheme))) {
    if(!(*host)) {
      logerror("Network URI '%s' does not contain host"
          " (did you forget to put literal IPv6 addresses in brackets?)'\n", uri);
      return -1;
    }
    if (pmatch[URI_RE_SCHEME].rm_so >= 0 && pmatch[URI_RE_HOST].rm_so >= 0 &&
        pmatch[URI_RE_AUTHORITY_WITH_SLASHES].rm_so == pmatch[URI_RE_AUTHORITY].rm_so) {
      logwarn("Network URI without a double slash before authority part is deprecated: '%s' should be '%s://%s%s%s'\n",
          uri, *scheme, *host, *port?":":"", *port?*port:"");
    }
    if(!(*port)) {
      *port = oml_strndup(DEF_PORT_STRING, sizeof(DEF_PORT_STRING));
    }

  } else if ((*host) && oml_uri_is_file(oml_uri_type(*scheme))) {
    /* We split the filename into host and path in a URI without host;
     * concatenate them back together, adding all the leading slashes that were initially present */
    authlen = len = pmatch[URI_RE_AUTHORITY_WITH_SLASHES].rm_eo - pmatch[URI_RE_AUTHORITY_WITH_SLASHES].rm_so;
    if (*path) { len += strlen(*path); }
    str = oml_malloc(len + 1);
    if (!str) {
      logerror("Memory error parsing URI '%s'\n", uri);
      return -1;
    }
    *str=0;
    strncat(str, uri+pmatch[URI_RE_AUTHORITY_WITH_SLASHES].rm_so, authlen);
    if (*path) {
      len -= authlen;
      strncat(str, *path, len);
    }
    oml_free((void*)*host);
    *host = NULL;
    oml_free((void*)*path);
    *path = str;

  }

  return 0;
}

/** Generate default file name to use when no output parameters are given.
 *
 * This function is not reentrant!
 *
 * \param app_ame	the name of the application
 * \param name		the OML ID of the instance
 * \param domain	the experimental domain
 *
 * \return a statically allocated buffer containing the URI of the output
 */
char*
default_uri(const char *app_name, const char *name, const char *domain)
{
  /* Use a statically allocated buffer to avoid having to free it,
   * just like other URI sources in omlc_init() */
  static char uri[256];
  int remaining = sizeof(uri) - 1; /* reserve 1 for the terminating null byte */
  char *scheme = "file:";
  char time[25];
  struct timeval tv;

  gettimeofday(&tv, NULL);
  strftime(time, sizeof(time), "%Y-%m-%dt%H.%M.%S%z", localtime(&tv.tv_sec));

  *uri = 0;
  strncat(uri, scheme, remaining);
  remaining -= sizeof(scheme);

  strncat(uri, app_name, remaining);
  remaining -= strlen(app_name);

  if (name) {
    strncat(uri, "_", remaining);
    remaining--;
    strncat(uri, name, remaining);
    remaining -= strlen(name);
  }

  if (domain) {
    strncat(uri, "_", remaining);
    remaining--;
    strncat(uri, domain, remaining);
  }

  strncat(uri, "_", remaining);
  remaining--;
  strncat(uri, time, remaining);

  return uri;
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
