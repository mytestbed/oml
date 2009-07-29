/*
 * Copyright 2007-2009 National ICT Australia (NICTA), Australia
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
/*!\file omlc.h
  \brief Header file for OML's client library.
*/

#ifndef OML_OMLC_H_
#define OML_OMLC_H_

#define omlc_is_numeric_type(t)					\
	(((t) == OML_LONG_VALUE) || ((t) == OML_DOUBLE_VALUE))

#define omlc_set_long(var, val) \
    (var).longValue = (val);

#define omlc_set_double(var, val) \
    (var).doubleValue = (val);

#define omlc_set_string(var, val) \
    (var).stringValue.ptr = (val); (var).stringValue.is_const = 0; \
    (var).stringValue.size = (var).stringValue.length = 0;

#define omlc_set_const_string(var, val) \
    (var).stringValue.ptr = (val); (var).stringValue.is_const = 1; \
    (var).stringValue.size = (var).stringValue.length = 0;

//#include <ocomm/queue.h>
#include <pthread.h>
#include <oml2/oml.h>

struct _omlFilter;   // can't include oml_filter.h yet
struct _omlWriter;   // forward declaration

/**
 * \struct
 * \brief
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


////! Structure to store the definition of a measurement point
//typedef struct _omlMPDef {
//  //! Name of MP
//  char* name;
//  //! Index to find first OmlMStream for this MP
//  int index;
//  //! Number of parameters defined
//  int param_count;
//  //! Names of parameters
//  char** param_names;
//  //! Types of paramters
//  OmlValueT* param_types;
//
//  //! Instead of array use linked list
//  struct _omlMPDef* next;
//} XXOmlMPDef;

/**
 * \struct
 * \brief
 */
//! Structure to store the definition of a measurement point
typedef struct _omlMPDef {
  //! Name of MP
  const char* name;
  //! Types of paramters
  OmlValueT  param_types;

  //! Instead of array use linked list
  //struct _omlMPDef* next;
} OmlMPDef;

struct _omlMP;

/**
 * \struct
 * \brief
 */
//! Structure to store how to collect a measurement point
typedef struct _omlMStream {

  //! Name of database table this stream is stored in
  char table_name[64];

  // Encompasing MP
  struct _omlMP* mp;

  //! A queue for each parameter of an MP. If NULL, do not collect.
  //OQueue** queues;

  OmlValue** values;
  struct _omlFilter* firstFilter;

  //! Index to identify this stream
  int index;

  //! Number of expected parameters.
  //int param_count;

  //! Counts number of samples produced.
  int sample_size;

  //! Number of samples to collect before creating next measurment.
  int sample_thres;

  //! Interval between measurements in seconds
  double sample_interval;

  //! Counting the number of samples produced
  long seq_no;

  //! Mutex for entire group of linked MPoints. Only first one initializes it.
//  pthread_mutex_t* mutexP;

  //! CondVar for filter in sample mode
  pthread_cond_t  condVar;

  //! Thread for filter on this stream
  pthread_t  filter_thread;

  //! Associated writer
  struct _omlWriter* writer;

  //! Next MP structure for same measurement point.
  struct _omlMStream* next;

  //! STORAGE
//  pthread_mutex_t mutex;

} OmlMStream;

/**
 * \struct
 * \brief
 */
//! Structure to store how to collect a measurement point
typedef struct _omlMP {
  const char* name;
  OmlMPDef*   param_defs;
  int         param_count;

  int         table_count; // counts how many MS are associated with this
                           // used for creating unique table names

  //! Link to first stream. If NULL nobody is interested in
  // this MP
  OmlMStream*    firstStream;

  int         active;  // Set to 1 if MP is active

  //! Mutex for entire group of streams.
  pthread_mutex_t* mutexP;

  //! STORAGE
  pthread_mutex_t mutex;

  //! Next MP structure for same measurement point.
  struct _omlMP* next;

} OmlMP;

//! Reads command line parameters and builds primary datastructure
//int
//omlc_init(
//  const char* appName,
//  int* argcPtr,
//  const char** argv,
//  oml_log_fn oml_log,
//  int mp_count,
//  OmlMPDef** mp_definitions
//);

int
omlc_init(
  const char* appName,
  int* argcPtr,
  const char** argv,
  oml_log_fn oml_log
);

//! Register a measurement point. Needs to be called
// for every measurment point AFTER +omlc_init2+
// and before a final +omlc_start+.
//
// The returned +OmlMStream+ needs to be the first
// argument in every +omlc_process+ call for this
// specific measurment point.
//
OmlMP*
omlc_add_mp(
  const char* mp_name,
  OmlMPDef*  mp_def
);

//! Finalizes inital configurations and get
// ready for consecutive 'omlc_process' calls
int
omlc_start(
  void
);

void
omlc_inject(
  OmlMP*      mp,
  OmlValueU*  values
);

// DEPRECIATED
void
omlc_process(
  OmlMP*      mp,
  OmlValueU*  values
);

//! Finalizes all open connections. Any
// futher cllas to 'omlc_process' are being
// ignored.
// NOTE: This call doesn't free all the memory
// at this stage. There may also be a few threads
// which will take some time to finish.
//
int
omlc_close(
  void
);

////! Called at the start of every MP wrapper function.
//OmlMStream*
//omlc_mp_start(
//  int index
//);
//
////! Called when the particular MP has been filled.
//void
//omlc_ms_process(
//  OmlMStream* mp
//);
//
////! Called at the end of every MP wrapper function.
//void
//omlc_mp_end(
//  int index
//);



#endif /*OML_OMLC_H_*/

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
