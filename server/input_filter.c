/*
 * Copyright 2015 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file input_filter.c
 * \brief Input Handlers for use by Client Handlers to translate network streams (e.g., compressed) into plain OMSP.
 * \author Olivier Mehani <olivier.mehani@nicta.com.au>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <strings.h>

#include "oml_utils.h"

#include "client_handler.h"
#include "input_filter.h"

static struct {
  const char * const encapsulation;
  input_filter_factory factory;
} encapsulation_filters[] = {
  { "null", input_filter_initialise },
#ifdef HAVE_LIBZ
  { "gzip", gzip_filter_create },
#endif /* HAVE_LIBZ */
};

/** Allocate and initialise a basic InputFilter
 * \param ch ClientHandler which requested the creation
 * \return a newly allocated InputFilter, to be freed by the caller, or NULL on error
 * \see oml_malloc, oml_free
 */
InputFilter*
input_filter_initialise(ClientHandler *ch) {
  InputFilter *self = NULL;

  if ((self=oml_malloc(sizeof(InputFilter)))) {
    memset(self, 0, sizeof(InputFilter));
    self->owner = ch;
  }

  return self;
}

/** Create an InputFilter of a given type
 * \param type string describing the type of filter to create
 * \param ch ClientHandler creating using the filter
 * \return a pointer to the newly create InputFilter (to be freed by caller), or NULL on error
 * \see encapsulation_filters, input_filter_destroy
 */
InputFilter*
input_filter_create(const char *type, ClientHandler *ch) {
  size_t i = 0;
  for (i = 0; i < LENGTH (encapsulation_filters); i++) {
    if (!strncmp (type, encapsulation_filters[i].encapsulation, strlen (encapsulation_filters[i].encapsulation))) {
      logdebug("%s: Creating InputFilter of type %s for ClientHandler %p\n", __FUNCTION__, type, ch);
      return encapsulation_filters[i].factory(ch);
    }
  }

  return NULL;
}

/** Process data through one filter
 * 
 * \note inbuf and outbuf *CAN* be the same MBuffer, in which case it will be
 * cleared after input, regardless of whether output was generated
 *
 * \param self InputFilter instance
 * \param inbuf MBuffer where data can be read from
 * \param outbuf MBuffer where data can be written
 * \return the total size of data in outbuf, or <0 on error
 * \see input_filter_in, input_filter_out, mbuf_concat, mbuf_clear
 */
ssize_t
input_filter_process(struct InputFilter *self, MBuffer *inbuf, MBuffer *outbuf)
{
  ssize_t ret = -1;

  logdebug2("%s: Processing data through InputFilter %p...\n", __FUNCTION__, self, ret);
  ret = input_filter_in(self, inbuf);
  if (inbuf == outbuf && ret >= 0) {
    mbuf_clear(outbuf);
  }
  if(ret>0) {
    ret = input_filter_out(self, outbuf);
  }

  return ret;
}

/** Input more data into InputFilter
 * \copydetails input_filter_in_fn
 * \todo make some promises on the state of the mbuf (e.g., message pointer)
 * \see input_filter_in_fn
 */
ssize_t
input_filter_in (struct InputFilter *self, MBuffer *mbuf) {
  ssize_t ret = -1;

  if (self && mbuf) {
    if(self->in) {
      ret = self->in(self, mbuf);
    }
  }

  if (ret>0) {
    logdebug3("%s: InputFilter %p generated %dB of new output\n", __FUNCTION__, self, ret);
  }

  return ret;
}

/** Request output from InputFilter
 * \copydetails input_filter_out_fn
 * \todo make some promises on the state of the mbuf (e.g., message pointer)
 * \see input_filter_out_fn
 */
ssize_t
input_filter_out (struct InputFilter *self, MBuffer *mbuf) {
  ssize_t ret = -1;
  if (self && mbuf) {
    if(self->out) {
      ret = self->out(self, mbuf);
    }
  }

  if (ret>0) {
    logdebug3("%s: InputFilter %p wrote %dB of output\n", __FUNCTION__, self, ret);
  }

  return ret;
}

/** Destroy an InputFilter and free its allocated memory
 *
 * The entire chain of InputFilter of a ClientHandler ch can be destroyed at once  with
 *
 *     while(input_filter_destroy(ch->input_filter);
 *
 * \param self InputFilter to destroy
 * \return a pointer to the next InputFilter in the chain
 */

InputFilter* input_filter_destroy(InputFilter *self) {
  InputFilter *next=NULL;
  if(self) {
    logdebug("%s: Destroying InputFilter %p\n", __FUNCTION__, self);
    next = self->next;
    if (self->destroy) {
      self->destroy(self);
    }
    oml_free(self);
  }
  return next;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
