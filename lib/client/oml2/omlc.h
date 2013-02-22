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
/** Public API of the OML client library. */

#ifndef OML_OMLC_H_
#define OML_OMLC_H_

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <ocomm/o_log.h>

/** Declaration from internal "mem.h" */
void *xmalloc (size_t size);
void xfree (void *ptr);
size_t xmalloc_usable_size(void *ptr);

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _omlValueE {
  /* Meta value types */
  OML_INPUT_VALUE = -2,
  OML_UNKNOWN_VALUE = -1,
  /* Concrete value types */
  OML_DOUBLE_VALUE = 0,
  OML_LONG_VALUE,
  OML_PADDING1_VALUE,
  OML_STRING_VALUE,
  OML_INT32_VALUE,
  OML_UINT32_VALUE,
  OML_INT64_VALUE,
  OML_UINT64_VALUE,
  OML_BLOB_VALUE
} OmlValueT;

#define omlc_is_integer_type(t)                             \
  (((t) == OML_LONG_VALUE) ||                               \
   ((t) == OML_INT32_VALUE) ||                              \
   ((t) == OML_UINT32_VALUE) ||                             \
   ((t) == OML_INT64_VALUE) ||                              \
   ((t) == OML_UINT64_VALUE))

#define omlc_is_numeric_type(t)                             \
  ((omlc_is_integer_type(t)) ||                             \
  ((t) == OML_DOUBLE_VALUE))

#define omlc_is_string_type(t) \
  ((t) == OML_STRING_VALUE)
#define omlc_is_blob_type(t) \
  ((t) == OML_BLOB_VALUE)

#define omlc_is_integer(v) \
  omlc_is_integer_type((v).type)
#define omlc_is_numeric(v) \
  omlc_is_numeric_type((v).type)
#define omlc_is_string(v) \
  omlc_is_string_type((v).type)
#define omlc_is_blob(v) \
  omlc_is_blob_type((v).type)

/**  Representation of a string measurement value.
 */
typedef struct _omlString {
  /** pointer to a nul-terminated C string */
  char *ptr;

  /** length of string */
  size_t length;

  /** size of the allocated underlying storage (>= length + 1) */
  size_t size;

  /** true if ptr references const storage */
  int   is_const;

} OmlString;

/**  Representation of a blob measurement value.
 */
typedef struct _omlBlob {
  /** pointer to blob data storage */
  void *ptr;

  /** length of the blob */
  size_t length;

  /** size of the allocated underlying storage (>= length) */
  size_t size;

} OmlBlob;

/** Multi-typed variable container without type information.
 *
 * WARNING: OmlValueU MUST be omlc_zero()d out before use. Additionally, if the
 * last type of data they contained was an OML_STRING_VALUE or OML_BLOB_VALUE,
 * they should be omlc_reset_(string|blob)(). Not doing so might result in
 * memory problems (double free or memory leak).
 *
 * When wrapped in OmlValue, the right thing is done, by the initialisation/reset functions.
 *
 * \see omlc_zero, omlc_zero_array, omlc_reset_string, omlc_reset_blob
 * \see oml_value_init, oml_value_array_init, oml_value_reset, oml_value_array_reset
 */
typedef union _omlValueU {
  long      longValue;
  double    doubleValue;
  OmlString stringValue;
  int32_t   int32Value;
  uint32_t  uint32Value;
  int64_t   int64Value;
  uint64_t  uint64Value;
  OmlBlob   blobValue;
} OmlValueU;

/** Zero out a freshly declared OmlValueU.
 *
 * \param var OmlValueU to manipulate
 */
#define omlc_zero(var) \
  memset(&(var), 0, sizeof(OmlValueU))
/** Zero out a freshly declared array of OmlValueU.
 *
 * \param var OmlValueU to manipulate
 * \param n number of OmlValueU in the array
 */
#define omlc_zero_array(var, n) \
  memset((var), 0, n*sizeof(OmlValueU))

/** Get an intrinsic C value from an OmlValueU.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to manipulate
 * \param type type of data contained in the OmlValueU
 * \return the value
 * \see omlc_get_int32, omlc_get_uint32, omlc_get_int64, omlc_get_uint64, omlc_get_double
 */
#define _omlc_get_intrinsic_value(var, type) \
  ((var).type ## Value)
/** Set an intrinsic C value in an OmlValueU.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to manipulate
 * \param type type of data contained in the OmlValueU
 * \param val value to store in the OmlValueU
 * \return the new value (val)
 * \see omlc_set_int32, omlc_set_uint32, omlc_set_int64, omlc_set_uint64, omlc_set_double
 */
#define _omlc_set_intrinsic_value(var, type, val) \
  ((var).type ## Value = (val))

/** \see _omlc_get_intrinsic_value */
#define omlc_get_int32(var) \
  ((int32_t)(_omlc_get_intrinsic_value(var, int32)))
/** \see _omlc_get_intrinsic_value */
#define omlc_get_uint32(var) \
  ((uint32_t)(_omlc_get_intrinsic_value(var, uint32)))
/** \see _omlc_get_intrinsic_value */
#define omlc_get_int64(var) \
  ((int64_t)(_omlc_get_intrinsic_value(var, int64)))
/** \see _omlc_get_intrinsic_value */
#define omlc_get_uint64(var) \
  ((uint64_t)(_omlc_get_intrinsic_value(var, uint64)))
/** \see _omlc_get_intrinsic_value */
#define omlc_get_double(var) \
  ((double)(_omlc_get_intrinsic_value(var, double)))
/** DEPRECATED \see omlc_get_uint32, omlc_get_uint64 */
#define omlc_get_long(var) \
  ((long)(_omlc_get_intrinsic_value(var, long)))

/** \see _omlc_set_intrinsic_value */
#define omlc_set_int32(var, val) \
  _omlc_set_intrinsic_value(var, int32, (int32_t)(val))
/** \see _omlc_set_intrinsic_value */
#define omlc_set_uint32(var, val) \
  _omlc_set_intrinsic_value(var, uint32, (uint32_t)(val))
/** \see _omlc_set_intrinsic_value */
#define omlc_set_int64(var, val) \
  _omlc_set_intrinsic_value(var, int64, (int64_t)(val))
/** \see _omlc_set_intrinsic_value */
#define omlc_set_uint64(var, val) \
  _omlc_set_intrinsic_value(var, int64, (uint64_t)(val))
/** \see _omlc_set_intrinsic_value */
#define omlc_set_double(var, val) \
  _omlc_set_intrinsic_value(var, double, (double)(val))
/** DEPRECATED \see omlc_set_uint32, omlc_set_uint64 */
#define omlc_set_long(var, val) \
  _omlc_set_intrinsic_value(var, long, (long)(val))

/** Get fields of an OmlValueU containing pointer to possibly dynamically allocated storage.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour, but have less parameters.
 *
 * \param var OmlValueU to manipulate
 * \param type type of data contained in the OmlValueU
 * \param field field to access
 * \return the value of the field
 * \see OmlString, omlc_get_string_ptr, omlc_get_string_length, omlc_get_string_size, omlc_get_string_is_const
 * \see OmlBlob, omlc_get_blob_ptr, omlc_get_blob_length, omlc_get_blob_size
 */
#define _oml_get_storage_field(var, type, field) \
  ((var).type ## Value.field)

/** Set fields of an OmlValueU containing pointer to possibly dynamically
 * allocated storage.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to manipulate
 * \param type type of data contained in the OmlValueU
 * \param field field to access
 * \param val value to set the field to
 * \return the newly-set value of the field (val)
 * \see OmlString, omlc_set_string_ptr, omlc_set_string_length, omlc_set_string_size, omlc_set_string_is_const
 * \see OmlBlob, omlc_set_blob_ptr, omlc_set_blob_length, omlc_set_blob_size
 */
#define _oml_set_storage_field(var, type, field, val) \
  ((var).type ## Value.field = (val))


/** \see _oml_get_storage_field */
#define omlc_get_string_ptr(var) \
  (_oml_get_storage_field(var, string, ptr))
/** \see _oml_get_storage_field */
#define omlc_get_string_length(var) \
  (_oml_get_storage_field(var, string, length))
/** \see _oml_get_storage_field */
#define omlc_get_string_size(var) \
  (_oml_get_storage_field(var, string, size))
/** \see _oml_get_storage_field */
#define omlc_get_string_is_const(var) \
  (_oml_get_storage_field(var, string, is_const))

/** \see _oml_set_storage_field */
#define omlc_set_string_ptr(var, val) \
  _oml_set_storage_field(var, string, ptr, (char*)(val))
/** \see _oml_set_storage_field */
#define omlc_set_string_length(var, val) \
  _oml_set_storage_field(var, string, length, (size_t)(val))
/** \see _oml_set_storage_field */
#define omlc_set_string_size(var, val) \
  _oml_set_storage_field(var, string, size, (size_t)(val))
/** \see _oml_set_storage_field */
#define omlc_set_string_is_const(var, val) \
  _oml_set_storage_field(var, string, is_const, (int)(val))

/** \see _oml_get_storage_field */
#define omlc_get_blob_ptr(var) \
  ((void *)_oml_get_storage_field(var, blob, ptr))
/** \see _oml_get_storage_field */
#define omlc_get_blob_length(var) \
  (_oml_get_storage_field(var, blob, length))
/** \see _oml_get_storage_field */
#define omlc_get_blob_size(var) \
  (_oml_get_storage_field(var, blob, size))

/** \see _oml_set_storage_field */
#define omlc_set_blob_ptr(var, val) \
  _oml_set_storage_field(var, blob, ptr, (char*)(val))
/** \see _oml_set_storage_field */
#define omlc_set_blob_length(var, val) \
  _oml_set_storage_field(var, blob, length, (size_t)(val))
/** \see _oml_set_storage_field */
#define omlc_set_blob_size(var, val) \
  _oml_set_storage_field(var, blob, size, (size_t)(val))

/** Free the storage contained in an OmlValueU if needed.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to operate on
 * \param type type of data contained in the OmlValueU
 * \see omlc_free_string, omlc_free_blob, xfree
 */
#define _omlc_free_storage(var, type)                     \
  do {                                                    \
    if (_oml_get_storage_field((var), type, size) > 0) {  \
      xfree(_oml_get_storage_field((var), type, ptr));    \
      _oml_set_storage_field((var), type, size, 0);       \
    }                                                     \
  } while(0)

/** Reset the storage contained in an OmlValueU, freeing allocated memory if
 * needed.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to operate on
 * \param type type of data contained in the OmlValueU
 * \see omlc_reset_string, omlc_reset_blob
 */
#define _omlc_reset_storage(var, type)                    \
  do {                                                    \
    _omlc_free_storage((var), type);                      \
    _oml_set_storage_field((var), type, ptr, NULL);       \
    _oml_set_storage_field((var), type, length, 0);       \
  } while(0)

/** Copy data into the dedicated storage of an OmlValueU, allocating memory if
 * needed.
 *
 * DO NOT USE THIS MACRO DIRECTLY!
 *
 * It is a helper for specific manipulation macros, which share its behaviour,
 * but have less parameters.
 *
 * \param var OmlValueU to manipulate
 * \param type type of data contained in the OmlValueU
 * \param str data to copy
 * \param len length of the data
 * \see omlc_set_string_copy, omlc_set_blob, xmalloc
 * \see xmalloc, memcpy(3)
 */
/* XXX: Does not check result of xmalloc */
#define _omlc_set_storage_copy(var, type, data, len)                          \
  do {                                                                        \
    if (len >= _oml_get_storage_field((var), type, size)) {                   \
      _omlc_reset_storage((var), type);                                       \
      _oml_set_storage_field((var), type, ptr, xmalloc(len));                 \
      _oml_set_storage_field((var), type, size,                               \
          xmalloc_usable_size(_oml_get_storage_field((var), type, ptr)));     \
    }                                                                         \
    memcpy(_oml_get_storage_field((var), type, ptr), (void*)(data), (len));   \
    _oml_set_storage_field((var), type, length, (len));                       \
  } while(0)

/** \see _omlc_free_storage */
#define _omlc_free_string(var) \
  _omlc_free_storage((var), string)
/** \see _omlc_reset_storage */
#define omlc_reset_string(var)          \
  do {                                  \
    _omlc_reset_storage((var), string); \
    omlc_set_string_is_const(var, 0);   \
  } while(0)

/** Copy a string into the dedicated storage of an OmlValueU.
 *
 * Allocate (or reuse) a buffer of size len+1, copy the string, and
 * nul-terminate it.
 *
 * The length attribute is the length of the string; not how much of the
 * storage is used (len + 1), as is the case for generic storage (blobs).
 *
 * \param var OmlValueU to manipulate
 * \param str terminated string to copy
 * \param len length of the string (not including nul terminator, i.e., output of strlen(3))
 * \see _omlc_set_storage_copy, strlen(3)
 */
#define omlc_set_string_copy(var, str, len)                    \
  do {                                                         \
    _omlc_set_storage_copy((var), string, (str), (len) + 1);   \
    ((char*)(var).stringValue.ptr)[len] = '\0';                \
    omlc_set_string_length(var, len);                          \
    omlc_set_string_is_const(var, 0);                          \
  } while(0)

/** Duplicate an OmlValueU containing a string, allocating storage for an actual copy of the C string.
 *
 * Allocate (or reuse) a buffer of size len+1, copy the string, and
 * nul-terminate it. As the string is actually copied. the destination string
 * is never const, regardless of the source.
 *
 * \param dst destination OmlValueU
 * \param src source OmlValueU
 * \see omlc_set_string_copy
 */
#define omlc_copy_string(dst, src) \
  omlc_set_string_copy((dst), omlc_get_string_ptr(src), omlc_get_string_length(src))

/** Store a pointer to a C string in an OmlValueU's string storage.
 * \param var OmlValueU to operate on
 * \param str C-string pointer to use
 */
#define omlc_set_string(var, str)                               \
  do {                                                          \
    omlc_reset_string(var);                                     \
    omlc_set_string_ptr((var), (str));                          \
    omlc_set_string_length((var), ((str)==NULL)?0:strlen(str)); \
  } while (0)

/** Store a pointer to a constant C string in an OmlValueU's string storage.
 * \param var OmlValueU to operate on
 * \param str constant C-string pointer to use
 */
#define omlc_set_const_string(var, str)                         \
  do {                                                          \
    _omlc_free_string(var);                                     \
    omlc_set_string_ptr((var), (char *)(str));                  \
    omlc_set_string_is_const((var), 1);                         \
    omlc_set_string_length((var), ((str)==NULL)?0:strlen(str)); \
  } while (0)

/** \see _omlc_free_storage */
#define omlc_free_blob(var) \
  _omlc_free_storage((var), blob)
/** \see _omlc_reset_storage */
#define omlc_reset_blob(var) \
  _omlc_reset_storage((var), blob)
/** Convenience alias to omlc_set_blob_copy */
#define omlc_set_blob(var, val, len) \
  omlc_set_blob_copy(var, val, len)
/** \see _omlc_set_storage */
#define omlc_set_blob_copy(var, val, len) \
  _omlc_set_storage_copy((var), blob, (val), (len))

/** Duplicate an OmlValueU containing a blob, allocating storage for an actual copy of the data.
 *
 * Allocate (or reuse) a buffer of size len and copy the data
 *
 * \param dst destination OmlValueU
 * \param src source OmlValueU
 * \see _omlc_set_storage_copy
 */
#define omlc_copy_blob(dst, src) \
  _omlc_set_storage_copy((dst), blob, omlc_get_blob_ptr(src), omlc_get_blob_length(src))

/** Typed container for an OmlValueU
 *
 * WARNING: OmlValue MUST be oml_value_init()ialised before use and oml_value_reset() after.
 * Not doing so might result in memory problems (double free or memory leak).
 *
 * This takes care of manipulating the contained OmlValueU properly.
 *
 * \see oml_value_init, oml_value_array_init, oml_value_reset,oml_value_array_reset
 */
typedef struct _omlValue {
  /** Type of value */
  OmlValueT type;

  /** Value */
  OmlValueU value;

} OmlValue;

/**
 * Definition of one field of an MP.
 *
 * An array of these create a full measurement point.
 * \see omlc_add_mp, OmlMP */
typedef struct _omlMPDef
{
    /** Name of the field */
    const char* name;

    /** Type of the field */
    OmlValueT  param_types;

} OmlMPDef;

/* Forward declaration, see below */
struct _omlMStream;

/** Definition of a Measurement Point.
 *
 * This structure contains an array of OmlMPDef defining the fields of the MP,
 * and an array of OmlMStream defining which streams need to receive output
 * from this MP.
 *
 * \see omlc_inject, OmlMStream, omlc_add_mp, OmlMP
 */
typedef struct _omlMP
{
    /** Name of this MP */
    const char* name;

    /** Array of the fields of this MP */
    OmlMPDef*   param_defs;
    /** Length of the param_defs (i.e., number of fields) */
    int         param_count;

    /** Number of MSs associated to this MP */
    int         table_count;

    /** Linked list of MSs */
    struct _omlMStream* streams;
#define firstStream streams

    /** Set to 1 if this MP is active (i.e., there is at least one MS) */
    int         active;

    /** Mutex for the streams list */
    pthread_mutex_t* mutexP;
    /** Mutex for this storage */
    pthread_mutex_t  mutex;

    /** Next MP in the instance's linked list */
    struct _omlMP*   next;

} OmlMP;

/* Forward declaration from oml_filter.h */
struct _omlFilter;   // can't include oml_filter.h yet
struct _omlWriter;   // forward declaration

/** Definition of a Measurement Stream.
 *
 * A measurement stream links an MP to an output, defined by a writing function
 * (OmlWriter), passing some or all of the fields into a filter (OmlFilter).
 *
 * All the samples injected into an MP are received, but through filtering and
 * aggregation, the output rate of the MS might be different (e.g., 1/n
 * samples, or with a time-based periodicity).
 *
 * \see OmlMP, OmlWriter, OmlFilter, omlc_inject
 */
typedef struct _omlMStream
{
    /** Name of this stream (and, usually, the database table it get stored in) */
    char table_name[64];

    /** MP associated to this stream */
    OmlMP* mp;

    /** Current output values */
    OmlValue** values;


    /** Linked list of the filters associated to this MS */
    struct _omlFilter* filters;
#define firstFilter filters

    /** Index of this stream */
    int index;

    /** Number of samples received in the last window */
    int sample_size;
    /** Number of samples to receive before producing an output (if > 1)*/
    int sample_thres;

    /** Interval between periodic reporting [s] */
    double sample_interval;

    /** Output's sequence number (i.e., number of samples produced so far */
    long seq_no;

    /** Condition variable for sample-mode filter (XXX: Never used) */
    pthread_cond_t  condVar;
    /** Filtering thread */
    pthread_t  filter_thread;

    /** Outputting function */
    struct _omlWriter* writer;

    /** Next MS in this MP's linked list */
    struct _omlMStream* next;

    /** Output's sequence number for the metadata associated to this stream */
    long meta_seq_no;

} OmlMStream;

/* Initialise the measurement library. */
int omlc_init(const char *appName, int *argcPtr, const char **argv, o_log_fn oml_log);

/*  Register a measurement point. */
OmlMP *omlc_add_mp(const char *mp_name, OmlMPDef *mp_def);

/* Get ready to start the measurement collection. */
int omlc_start(void);

/*  Inject a measurement sample into a Measurement Point.  */
int omlc_inject(OmlMP *mp, OmlValueU *values);

/** Inject metadata (key/value) for a specific MP.  */
int omlc_inject_metadata(OmlMP *mp, const char *key, const OmlValueU *value, OmlValueT type, const char *fname);

// DEPRECATED
void omlc_process(OmlMP* mp, OmlValueU* values);

/**  Terminate all open connections. */
int omlc_close(void);

#ifdef __cplusplus
}
#endif


#endif /*OML_OMLC_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
