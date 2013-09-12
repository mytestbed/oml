/*
 * Copyright 2013 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

/** \file json.c
 * \brief Source code for converting internal data to JSON format (used for storing vectors on DB).
 */

#include <assert.h>
#include <errno.h>
#include <float.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "json.h"
#include "mem.h"


/**
 * Resize the output buffer to at least new_sz octets. The buffer
 * pointed at by *str must be either NULL or have been allocated by
 * oml_malloc(). On return the output buffer pointed to by *str will have
 * been zeroed.
 *
 * \param str A non-NULL pointer pointer to the buffer.
 * \param new_sz The news size for the output buffer.
 * \return The size of the buffer or -1 if buffer resize fails.
 */
static ssize_t
resize_buffer(char **str, size_t new_sz)
{
  assert(str);
  ssize_t str_sz = -1;
  if(NULL == *str || oml_malloc_usable_size(*str) < new_sz) {
    char *new = oml_realloc(*str, new_sz);
    if(new) {
      *str = new;
      str_sz = new_sz;
      memset(*str, 0, str_sz);
    }
  } else {
    str_sz = oml_malloc_usable_size(*str);
    memset(*str, 0, str_sz);
  }
  return str_sz;
}


/**
 * Convert a vector of double values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.
 *
 * \param v Pointer to an array of double values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_double_to_json(const double *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * (DBL_DIG + 3)) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [double] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %.*g ]", DBL_DIG, v[i]);
    else
      n = snprintf(*str + m, str_sz - m, "[ %.*g ]", DBL_DIG, v[i]);
    
    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [double] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/**
 * Convert a vector of int32_t values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.
 *
 * \param v Pointer to an array of int32_t values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_int32_to_json(const int32_t *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * 13) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [int32] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %" PRId32 " ]", v[i]);
    else
      n = snprintf(*str + m, str_sz - m, "[ %" PRId32 " ]", v[i]);

    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [int32] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/**
 * Convert a vector of uint32_t values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.
 *
 * \param v Pointer to an array of uint32_t values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_uint32_to_json(const uint32_t *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * 13) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [uint32] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %" PRIu32 " ]", v[i]);
    else
      n = snprintf(*str + m, str_sz - m, "[ %" PRIu32 " ]", v[i]);

    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [uint32] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/**
 * Convert a vector of int64_t values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.  It is, therefore, mandatory to
 * allocate the output buffer using oml_malloc().
 *
 * \param v Pointer to an array of int64_t values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_int64_to_json(const int64_t *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * 22) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [int64] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %" PRId64 " ]", v[i]);
    else
      n = snprintf(*str + m, str_sz - m, "[ %" PRId64 " ]", v[i]);

    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [int64] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/**
 * Convert a vector of uint64_t values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.  It is, therefore, mandatory to
 * allocate the output buffer using oml_malloc().
 *
 * \param v Pointer to an array of uint64_t values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_uint64_to_json(const uint64_t *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * 22) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [uint64] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %" PRIu64 " ]", v[i]);
    else
      n = snprintf(*str + m, str_sz - m, "[ %" PRIu64 " ]", v[i]);

    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [uint64] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/**
 * Convert a vector of uint64_t values to a JSON string. The output
 * string is written to an output buffer which is oml_realloc()ed if too
 * small to accommodate the output.
 *
 * \param v Pointer to an array of uint64_t values.
 * \param v_sz Number of elements in v.
 * \param str Non-NULL pointer pointer to the output buffer.
 * \return If the call succeeds then the buffer; otherwise NULL.
 */
ssize_t
vector_bool_to_json(const bool *v, size_t v_sz, char **str)
{
  assert(v || 0 == v_sz);
  assert(str);
  ssize_t str_sz = resize_buffer(str, 4 + (v_sz * 22) + 1);
  if(-1 == str_sz) {
    o_log(O_LOG_ERROR, "%s(): failed to resize buffer for [bool] (v_sz=%zu)\n", __func__, v_sz);
    return -1;
  }

  size_t i;
  ssize_t m = 0, n = 0;
  memset(*str, 0, str_sz);
  for(i = 0; i < v_sz; i++) {
    if(i)
      n = snprintf(*str + m, str_sz - m, ", %s ]", (v[i] ? "true" : "false"));
    else
      n = snprintf(*str + m, str_sz - m, "[ %s ]", (v[i] ? "true" : "false"));

    if(0 <= n && n < str_sz - m)
      m += n - 2;
    else {
      o_log(O_LOG_ERROR, "%s(): failed to convert [bool] (v_sz=%zu, n=%zd, str_sz=%zu, m=%zu)\n", __func__, v_sz, n, str_sz, m);
      return -1;
    }
  }
  return strlen(*str);
}


/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
 */
