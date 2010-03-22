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
/*!\file api.c
  \brief Implements most of OML's client side exposed API.

*/

#include <ocomm/o_log.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <oml2/oml_filter.h>
#include "client.h"

extern OmlClient* omlc_instance;

/**
 * \brief Called when the particular MStream has been filled.
 * \param ms the oml stream to process
 */
static void
omlc_ms_process(
  OmlMStream* ms
);

/**
 * \brief DEPRECIATED
 */
void
omlc_process(
  OmlMP*      mp,
  OmlValueU*  values
) {
  o_log(O_LOG_WARN, "'omlc_process' is deprecated, use 'omlc_inject' instead\n");
  omlc_inject(mp, values);
}

/**
 * \brief Called when a measure has to be treated
 * \param mp the measurement point in the application
 * \param values the new values to analyse
 * \return
 */
void
omlc_inject(
  OmlMP*      mp,
  OmlValueU*  values
) {
  if (omlc_instance == NULL) return;
  if (mp == NULL) return;
  if (mp_lock(mp)) return;

  OmlMStream* ms = mp->firstStream;
  while (ms) {
    OmlFilter* f = ms->firstFilter;
    for (; f != NULL; f = f->next) {
      OmlValue v;

      /* FIXME:  Should validate this indexing */
      v.value = *(values + f->index);
      v.type = mp->param_defs[f->index].param_types;

      f->input(f, &v);
    }
    omlc_ms_process(ms);
    ms = ms->next;
  }
  mp_unlock(mp);
}

/**
 * \brief Called when the particular MStream has been filled.
 * \param ms the oml stream to process
 */
static void
omlc_ms_process(
  OmlMStream* ms
) {
  if (ms == NULL) return;

  if (ms->sample_thres > 0 && ++ms->sample_size >= ms->sample_thres) {
    // sample based filters fire
    filter_process(ms);
    if (pthread_cond_signal(&ms->condVar)) {
      o_log(O_LOG_WARN, "%s: Couldn't signal condVar (%s)\n",
        ms->table_name, strerror(errno));
    }
    ms->sample_size = 0;
  }
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
