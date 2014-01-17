/*
 * Copyright 2013-2014 National ICT Australia Limited (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */

#ifndef OML_JSON_H
#define OML_JSON_H

#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

extern ssize_t
vector_double_to_json(const double *v, size_t v_sz, char **str);

extern ssize_t
vector_int32_to_json(const int32_t *v, size_t v_sz, char **str);

extern ssize_t
vector_uint32_to_json(const uint32_t *v, size_t v_sz, char **str);

extern ssize_t
vector_int64_to_json(const int64_t *v, size_t v_sz, char **str);

extern ssize_t
vector_uint64_to_json(const uint64_t *v, size_t v_sz, char **str);

extern ssize_t
vector_bool_to_json(const bool *v, size_t v_sz, char **str);

#endif /* OML_JSON_H */
