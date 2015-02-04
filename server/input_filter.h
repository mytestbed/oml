/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file input_filter.h
 * \brief Interface for InputFilters, for use by ClientHandlers.
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */
#ifndef INPUT_FILTER_H_
#define INPUT_FILTER_H_

#include <sys/types.h>

#include "mbuf.h"

/* Advance declarations */
struct InputFilter;
struct ClientHandler;

/** Allocate and initialise a basic InputFilter */
struct InputFilter* input_filter_initialise(struct ClientHandler *ch);
/** Create an InputFilter of a given type */
struct InputFilter* input_filter_create(const char *type, struct ClientHandler *ch);
/** Input more data into InputFilter */
ssize_t input_filter_in (struct InputFilter *self, MBuffer *mbuf);

/** Process data through one filter */
ssize_t input_filter_process(struct InputFilter *self, MBuffer *inbuf, MBuffer *outbuf);
/** Request output from InputFilter */
ssize_t input_filter_out (struct InputFilter *self, MBuffer *mbuf);
/** Destroy an InputFilter and free its allocated memory */
struct InputFilter* input_filter_destroy(struct InputFilter *self);

/** A factory for InputFilters
 * \copydetails input_filter_initialise
 */
typedef struct InputFilter* (*input_filter_factory) (struct ClientHandler *ch);

/** Input function for InputFilter
 * Called by input_filter_in
 * \param self InputFilter instance
 * \param mbuf MBuffer where data can be read from
 * \return 0 on success, size >0 of data available, <0 on error
 * \see input_filter_in
 */
typedef ssize_t (*input_filter_in_fn) (struct InputFilter *self, MBuffer *mbuf);

/** Output function for InputFilter
 * \param self InputFilter instance
 * \param mbuf MBuffer where output data is to be written
 * \warning Called by input_filter_out, data MUST be concatenated into the output buffer
 * \return size >=0 of all data available in mbuf, <0 on error
 * \see input_filter_out, mbuf_concat
 */
typedef ssize_t (*input_filter_out_fn) (struct InputFilter *self, MBuffer *mbuf);

/**  Cleanup function for InputFilter
 * \param self InputFilter to destroy
 */
typedef void (*input_filter_destroy_fn) (struct InputFilter *self);

/** Input handler header describing functions on input to extract usable OMSP.
 *
 * When new data is received, the ClientHandler will call each InputFilter,
 * passing the output of the previous one as input to the next one. The
 * following pseudocode illustrates this.
 *
 *     while(input_filter) {
 *       if (ret = input_filter->in(input_filter, mbuf)) {
 *         ret = input_filter->out(input_filter, mbuf))
 *       }
 *       input_filter=input_filter->next;
 *     }
 *     // At this stage, mbuf should contain OMSP if ret > 0
 *
 * \warning Though the ClientHandler is passed to the
 * input_filter_out/input_filter_out functions, InputFilters should
 * manipulate it as an opaque reference.
 */
typedef struct InputFilter {
  struct InputFilter *next;         /**< Link to the next InputFilter in the chain */
  input_filter_in_fn in;            /**< Input function for this handler */
  input_filter_out_fn out;          /**< Output function for this handler */
  input_filter_destroy_fn destroy;  /**< Cleanup function for this handler */
  struct ClientHandler *owner;      /**< The ClientHandler using this InputFilter */
  void *state;                      /**< Opaque state data to be used by implementations */
} InputFilter;

/* Creation functions for specific filters */

# if HAVE_LIBZ
InputFilter *gzip_filter_create (struct ClientHandler* ch);
# endif /* HAVE_LIBZ */

#endif /* INPUT_FILTER_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
