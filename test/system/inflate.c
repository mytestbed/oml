/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

/**\file inflate.c
 * \brief Error-tolerant GZip inflater, for system tests
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 *
 * By using oml_zlib_inf, this tool is able to skip over non-GZip data (such as
 * uncompressed headers), and missing bits due to disconnections.
 *
 * \see oml_zlib_inf
 */
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "zlib_utils.h"

int
main(int argc, char **argv)
{
  FILE *in = stdin, *out = stdout;

  switch(argc) {
  case 3:
    out = fopen(argv[2], "w");
    if (!out) {
      fprintf(stderr, "error: cannot open '%s' for writing: %s\n",
          argv[2], strerror(errno));
      return -3;
    }
  case 2:
    in = fopen(argv[1], "r");
    if (!in) {
      fprintf(stderr, "error: cannot open '%s' for reading: %s\n",
          argv[1], strerror(errno));
      return -2;
    }
  case 1:
    break;
  default:
      fprintf(stderr, "usage: %s [IN [OUT]]\n",
          argv[0]);
      return -1;
  }

  int ret = 0;
  while(!feof(in)) {
    if((ret = oml_zlib_inf(in, out))) {
      fflush(stdout);
      fprintf(stderr, "oml_zlib_inf() failed with %d, %d\n", ret, feof(in));
    }
  }
  return ret;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 vim: sw=2:sts=2:expandtab
*/
