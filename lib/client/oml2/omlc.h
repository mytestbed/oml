/*
 * Copyright 2007-2010 National ICT Australia (NICTA), Australia
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
/**
 * \file omlc.h
 * \brief Header file for OML's client library.
 */

#ifndef OML_OMLC_H_
#define OML_OMLC_H_

#include <pthread.h>
#include <ocomm/o_log.h>


#ifdef __cplusplus
extern "C" {
#endif

#define omlc_is_numeric_type(t)								\
	(((t) == OML_LONG_VALUE) || ((t) == OML_DOUBLE_VALUE))

#define omlc_set_long(var, val) \
    (var).longValue = (val);

#define omlc_set_double(var, val) \
    (var).doubleValue = (val);

#define omlc_set_string(var, val)				\
	do {														   \
    (var).stringValue.ptr = (val); (var).stringValue.is_const = 0; \
    (var).stringValue.size = (var).stringValue.length = 0;		   \
	} while (0);

#define omlc_set_const_string(var, val)			\
	do {														   \
    (var).stringValue.ptr = (val); (var).stringValue.is_const = 1; \
    (var).stringValue.size = (var).stringValue.length = 0;		   \
	} while (0);

struct _omlFilter;   // can't include oml_filter.h yet
struct _omlWriter;   // forward declaration

/**
 *  Representation of a string measurement value.
 */
typedef struct _omlString {
  char* ptr;
  int   is_const; // true if ptr references const storage
  int   size;     // size of string
  int   length;   // length of internally allocated storage
} OmlString;

typedef union _omlValueU {
  long longValue;
  double doubleValue;
  OmlString stringValue;
} OmlValueU;

typedef enum _omlValueE {
  /* Meta value types */
  OML_INPUT_VALUE = -2,
  OML_UNKNOWN_VALUE = -1,
  /* Concrete value types */
  OML_DOUBLE_VALUE = 0,
  OML_LONG_VALUE,
  OML_STRING_PTR_VALUE,
  OML_STRING_VALUE
  //OML_STRING32_VALUE
} OmlValueT;

/**
 * \struct
 * \brief
 */
typedef struct _omlValue {
  OmlValueT type;
  OmlValueU value;
} OmlValue;

//! Copy the content of one value to another one.
extern int oml_value_copy(OmlValueU* value, OmlValueT type, OmlValue* to);

//! Set all the values to either zero or the empty string.
extern int oml_value_reset(OmlValue* v);

/**
 * Structure to store the definition of a measurement point.
 */
typedef struct _omlMPDef
{
	const char* name;       ///< Name of MP
	OmlValueT  param_types; ///< Types of paramters
} OmlMPDef;

struct _omlMP;

/**
 * Structure to store how to collect a measurement point.
 */
typedef struct _omlMStream
{
	char table_name[64]; ///< Name of database table this stream is stored in

	struct _omlMP* mp; ///< Encompasing MP

	OmlValue** values;

	struct _omlFilter* firstFilter; ///< Linked list of filters on this MS.

	int index; ///< Index to identify this stream

	int sample_size; ///< Counts number of samples produced.

	int sample_thres; ///< Number of samples to collect before creating next measurment.

	double sample_interval; ///< Interval between measurements in seconds

	long seq_no; ///< Counting the number of samples produced

	pthread_cond_t  condVar; ///< CondVar for filter in sample mode

	pthread_t  filter_thread; ///< Thread for filter on this stream

	struct _omlWriter* writer; ///< Associated writer

	struct _omlMStream* next; ///< Next MP structure for same measurement point.
} OmlMStream;

/**
 * Structure to store how to collect a measurement point.
 */
typedef struct _omlMP
{
	const char* name;
	OmlMPDef*   param_defs;
	int         param_count;

	//! Count how many MS are associated with this MP.
	//! Used for creating unique table names
	int         table_count;

	//! Link to first stream.
	//! If NULL then nobody is interested in this MP.
	OmlMStream* firstStream;

	int         active;  ///< Set to 1 if MP is active

	pthread_mutex_t* mutexP; ///< Mutex for entire group of streams.
	pthread_mutex_t  mutex;  ///< STORAGE

	struct _omlMP*   next; ///< Next MP structure for same measurement point.
} OmlMP;

/**
 *  Read command line parameters and build primary datastructures.
 *
 *  The appName must be the name of the application, and cannot
 *  contain spaces (or tabs, etc.).  It may contain forward slashes
 *  '/', in which case it is truncated to the substring following the
 *  final slash.  This allows using a UNIX pathname such as from
 *  argv[0].  An invalid application name will cause omlc_init() to
 *  fail.
 *
 *  The call to omlc_init() must be the first call of an OML function
 *  in the application.  Calling any other OML function before
 *  omlc_init() has been called, or after a call to omlc_init() that
 *  fails (returns -1), will result in undefined behaviour.
 *
 *  All OML-specific arguments are removed from the argument vector
 *  after being processed.  This means that the client application's
 *  command line argument processing does not need to be modified as
 *  long as omlc_init() is called before any application-specific
 *  command line processing is performed.
 *
 *  \param appName the name of the application.
 *  \param argcPtr pointer to an integer containing the number of
 *                 command line arguments.  When the function ends, if
 *                 any OML arguments have been processed, *argcPtr
 *                 will be set to the number of arguments remaining
 *                 after the OML arguments are removed.
 *  \param argv    pointer to the command line argument vector to
 *                 process.  Any OML options are removed from the
 *                 vector during processing.  When this function
 *                 completes, argv will contain a contiguous vector of
 *                 the original command line arguments minus any OML
 *                 arguments that were processed.
 *  \param oml_log A custom logging function to use, as per o_set_log().
 *
 *  \return 0 on success, -1 on failure.
 */
int
omlc_init(
  const char* appName,
  int* argcPtr,
  const char** argv,
  o_log_fn oml_log
);

/**
 *  Register a measurement point.
 *
 *  This function should be called after omlc_init() and before
 *  omlc_start().  It can be called multiple times, once for each
 *  measurement point that the application needs to define.
 *
 *  The returned +OmlMP+ must be the first argument in every
 *  +omlc_inject+ call for this specific measurement point.
 *
 *  The MP's input structure is defined by the mp_def parameter.
 *
 *  \param mp_name The name of this MP.  The name must not contain
 *                 whitespace.
 *  \param mp_def  Definition of this MP's input tuple, as an array
 *                 of OmlMPDef objects.
 *
 *  \return 0 on success, -1 on failure.
 */
OmlMP*
omlc_add_mp(
  const char* mp_name,
  OmlMPDef*  mp_def
);

/**
 *  Finalize the initialization process and enable measurement
 *  collection.
 *
 *  This function must be called after +omlc_init+ and after any calls
 *  to +omlc_add_mp+.  It finalizes the initialization process and
 *  initializes filters on all measurement points, according to the
 *  current configuration (based on either command line options or the
 *  XML config file named by the --oml-config command line option).
 *
 *  Once this function has been called, and if it succeeds, the
 *  application is free to start creating measurement samples by
 *  calling +omlc_inject+.
 *
 *  If this function fails, subsequent calls to +omlc_inject+ will
 *  result in undefined behaviour.
 *
 *  \return 0 on success, -1 on failure.
 */
int
omlc_start(
  void
);

/**
 *  Inject a measurement sample into a Measurement Point.
 *
 *  \param mp     the measurement point to inject into, as returned by
 *                +omlc_add_mp+.
 *  \param values the measurement sample vector.  The types of the
 *                values in this vector should match the definition
 *                for this MP given in the call to +omlc_add_mp+.
 */
void
omlc_inject(
  OmlMP*      mp,
  OmlValueU*  values
);

// DEPRECATED
void
omlc_process(
  OmlMP*      mp,
  OmlValueU*  values
);

/**
 *  Finalize all open connections.
 *
 *  Once this function has been called, any futher calls to
 *  +omlc_inject+ will be ignored.
 *
 *  @note This call doesn't free all memory used by OML
 *  immediately. There may be a few threads which will take some
 *  time to finish.
 */
int
omlc_close(
  void
);

#ifdef __cplusplus
}
#endif


#endif /*OML_OMLC_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
