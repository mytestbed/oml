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
/** \file api.c
 * Implement the user-visible API of OML.
 */

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "oml2/oml_filter.h"
#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "client.h"

/**
 * \brief Called when the particular MStream has been filled.
 * \param ms the oml stream to process
 */
static void
omlc_ms_process(OmlMStream* ms);

/** DEPRECATED
 * \see omlc_inject
 */
void
omlc_process(OmlMP *mp, OmlValueU *values)
{
  logwarn("'omlc_process' is deprecated, use 'omlc_inject' instead\n");
  omlc_inject(mp, values);
}

/**  Inject a measurement sample into a Measurement Point.
 *
 * \param mp pointer to OmlMP into which the new sample is being injected
 * \param values an array of OmlValueU to be processed
 * \return 0 on success, <0 otherwise
 *
 * The values' types is assumed to be the same as what was passed to omlc_add_mp
 * Type information is stored in (stored in mp->param_defs[].param_types)
 *
 * Traverse the list of MSs attached to this MP and, for each MS, the list of
 * filters to apply to the sample. Input the relevant field of the MP to each
 * filter, the call omlc_ms_process() to determine whether a new sample has to
 * be output on that MS.
 *
 * The content of values is deep-copied into the MSs' storage, so values can be
 * directly freed/reused when inject returns.
 *
 * \see omlc_add_mp, omlc_ms_process, oml_value_set
 */
int
omlc_inject(OmlMP *mp, OmlValueU *values)
{
  OmlMStream* ms;
  OmlValue v;

  if (omlc_instance == NULL) return -1;
  if (mp == NULL || values == NULL) return -1;

  oml_value_init(&v);

  if (mp_lock(mp) == -1) {
    logwarn("Cannot lock MP '%s' for injection\n", mp->name);
    return -1;
  }
  ms = mp->streams;
  while (ms) {
    OmlFilter* f = ms->filters;
    for (; f != NULL; f = f->next) {

      /* FIXME:  Should validate this indexing */
      oml_value_set(&v, &values[f->index], mp->param_defs[f->index].param_types);

      f->input(f, &v);
    }
    omlc_ms_process(ms);
    ms = ms->next;
  }
  mp_unlock(mp);
  oml_value_reset(&v);

  return 0;
}

/** Inject metadata (key/value) for a specific MP.
 *
 * With the current storage backends, the key will be a concatenation following
 * this pattern: MPNAME_[FIELDNAME_]KEY. This transformation is done on the
 * client's side. Additionally any later injection of metadata in an already
 * existing key will override its provious value.
 *
 * \param mp pointer to the OmlMP to which the metadata relates
 * \param key base name for the key (keys are unique)
 * \param value OmlValueU containing the value for the given key
 * \param type OmlValueT of value, currently only OML_STRING_VALUE is valid
 * \param fname optional field name to which this metadata relates
 * \return 0 on success, -1 otherwise
 *
 * \see omlc_add_mp
 */
int
omlc_inject_metadata(OmlMP *mp, const char *key, const OmlValueU *value, OmlValueT type, const char *fname)
{
  return -1;
}

/** Called when the particular MS has been filled.
 *
 * Determine whether a new sample must be issued (in per-sample reporting), and
 * ask the filters to generate it if need be.
 *
 * A lock for the MP containing that MS must be held before calling this function.
 *
 * \param ms pointer to the OmlMStream to process
 * \see filter_process
 */
static void
omlc_ms_process(OmlMStream *ms)
{
  if (ms == NULL) return;

  if (ms->sample_thres > 0 && ++ms->sample_size >= ms->sample_thres) {
    // sample based filters fire
    filter_process(ms);
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
