/*
 * Copyright 2010 National ICT Australia (NICTA), Australia
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

#include <string.h>
#include <stdlib.h>
#include "validate.h"

static int
valid_name_char (char c)
{
  return isalnum (c) || (c == '_');
}

/**
 *  Validate the name of an OML identifier (schema name, schema
 *  column, measurement point, application.)
 *
 *  The only valid characters for the name of an identifier are
 *  letters [A-Za-z], numbers, and the underscore character.  The
 *  first character must not be a digit.
 *
 *  @param name the name to validate.
 *
 *  @return If the name is valid, a pointer to the name
 *  returned.  If the name is not valid, returns NULL.  If the
 *  returned pointer is not NULL, it may be different from name.
 *
 */
const char*
validate_name (const char* name)
{
  size_t len = strlen (name);
  const char* p = name + len - 1;

  do {
    if (!valid_name_char (*p))
      return NULL;
  } while (--p >= name);

  if (isdigit (*++p))
    return NULL;

  return p;
}
