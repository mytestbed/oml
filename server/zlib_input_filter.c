/*
 * Copyright 2015-2016 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file zlib input_filter.c
 * \brief Input filter inflating the received data
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <strings.h>
#include <assert.h>
#include <zlib.h>

#include "ocomm/o_log.h"
#include "oml_utils.h"
#include "zlib_utils.h"
#include "mbuf.h"
#include "input_filter.h"
#include "client_handler.h"

static ssize_t zlib_filter_in (InputFilter *self, MBuffer *mbuf);
static ssize_t zlib_filter_out (InputFilter *self, MBuffer *mbuf);
static void zlib_filter_destroy (InputFilter *self);

/** Internal state of a ZlibInputFilter */
typedef struct {
  z_stream strm; /**< Zlib stream */
  MBuffer *mbuf; /**< Temporary storage of output data */
} OmlZlibInputFilterState;

/** Allocate a Zlib InputFilter
 * \copydetails input_filter_initialise
 * \see input_filter_initialise
 * */
InputFilter*
zlib_filter_create (ClientHandler* ch) {
  InputFilter *self = input_filter_initialise(ch);
  OmlZlibInputFilterState *state;

  if (self) {
    logdebug("%s: %s: initialised filter %p\n", self->owner->name, __FUNCTION__, self);

    if(!(self->state = (void*) oml_malloc(sizeof(OmlZlibInputFilterState)))) {
      logerror ("%s: cannot allocate memory for filter state)\n", __FUNCTION__);
      oml_free(self);
      self = NULL;

    } else {
      state = (OmlZlibInputFilterState*) self->state;

      self->in = zlib_filter_in;
      self->out = zlib_filter_out;
      self->destroy = zlib_filter_destroy;

      state->mbuf = mbuf_create();
      oml_zlib_init(&state->strm, OML_ZLIB_INFLATE, 0);
    }
  }
  return self;
}

/** Input function for ZlibInputFilter
 * \copydetails input_filter_in_fn
 * \see input_filter_in
 */
static ssize_t
zlib_filter_in (InputFilter *self, MBuffer *mbuf)
{
  assert(self);
  assert(mbuf);
  int ret = -1;
  OmlZlibInputFilterState *state = (OmlZlibInputFilterState*)self->state;
  /* input_filter_initialise should make sure these are not NULL */
  assert(state);

  ret = oml_zlib_inf_mbuf(&state->strm, Z_NO_FLUSH, mbuf, state->mbuf);
  if(Z_OK == ret || Z_STREAM_END == ret) {
    ret = mbuf_fill(state->mbuf);
  } else {
    logwarn("%s: error inflating in %p: %d\n", __FUNCTION__, self, ret);
  }

  return ret;
}

/** Output function for ZlibInputFilter
 * \copydetails input_filter_out_fn
 * \see input_filter_out
 */
static ssize_t
zlib_filter_out (InputFilter *self, MBuffer *mbuf)
{
  assert(self);
  assert(mbuf);
  OmlZlibInputFilterState *state = (OmlZlibInputFilterState*)self->state;
  /* input_filter_initialise should make sure these are not NULL */
  assert(state);

  mbuf_concat(state->mbuf, mbuf);
  mbuf_repack(mbuf);
  mbuf_clear(state->mbuf);
  return mbuf_fill(mbuf);
}

/** Cleanup function for ZlibInputFilter
 * \copydetails input_filter_destroy_fn
 */
static void
zlib_filter_destroy(InputFilter *self)
{
  assert(self);
  OmlZlibInputFilterState *state = (OmlZlibInputFilterState*)self->state;
  if(state) {
    oml_zlib_end(&state->strm, OML_ZLIB_INFLATE, state->mbuf);
    /* I'll store this precious data here... */
    mbuf_destroy((MBuffer*)state->mbuf);
    oml_free(state);
  }
}
/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
