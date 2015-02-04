/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file gzip input_filter.c
 * \brief Input filter inflating the received data
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <strings.h>
#include <assert.h>

#include "ocomm/o_log.h"
#include "oml_utils.h"
#include "mbuf.h"
#include "input_filter.h"
#include "client_handler.h"

static ssize_t gzip_filter_in (InputFilter *self, MBuffer *mbuf);
static ssize_t gzip_filter_out (InputFilter *self, MBuffer *mbuf);

/** Allocate a Zlib InputFilter
 * \copydetails input_filter_initialise
 * \see input_filter_initialise
 * */
InputFilter*
gzip_filter_create (ClientHandler* ch) {
  InputFilter *self = input_filter_initialise(ch);
  logdebug("%s: %s: initialised filter %p\n", self->owner->name, __FUNCTION__, self);
  self->in = gzip_filter_in;
  self->out = gzip_filter_out;
  return self;
}

/** Allocate a Zlib InputFilter
 * \copydetails input_filter_initialise
 * \see input_filter_initialise
 * */
static ssize_t
gzip_filter_in (InputFilter *self, MBuffer *mbuf) {
  /* input_filter_in should make sure these are not NULL */
  assert(self);
  assert(mbuf);
  logdebug("%s: %s: received %dB of data in %p\n", self->owner->name, __FUNCTION__, mbuf_fill(mbuf), mbuf);
  return mbuf_fill(mbuf);
}

static ssize_t
gzip_filter_out (InputFilter *self, MBuffer *mbuf) {
  /* input_filter_in should make sure these are not NULL */
  assert(self);
  assert(mbuf);
  logdebug("%s: %s: outputing %dB of data in %p\n", self->owner->name, __FUNCTION__, mbuf_fill(mbuf), mbuf);
  return mbuf_fill(mbuf);
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
