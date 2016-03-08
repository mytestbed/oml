/*
 * Copyright 2010-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file validate.c
 * \brief Implements the validation of OML identifiers.
 */

#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "validate.h"

static int
valid_name_char (char c)
{
  return isalnum (c) || (c == '_');
}

/** Validate the name of an OML identifier (schema name, schema  column, measurement point, application.)
 *
 * The only valid characters for the name of an identifier are
 * letters [A-Za-z], numbers, and the underscore character.  The
 * first character must not be a digit.
 *
 * \param name the name to validate.
 * \return If the name is valid, returns 1, otherwise returns 0.
 */
int
validate_name (const char* name)
{
  size_t len = strlen (name);
  const char* p = name;

  if (!p || *p == '\0' || isdigit (*p))
    return 0;

  do {
    if (!valid_name_char (*p))
      return 0;
  } while ((size_t)(++p - name) < len);

  return 1;
}
