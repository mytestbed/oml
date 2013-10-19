/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/** \file api.c
 * \brief Implement the user-visible API of OML.
 *
 * \page api OML User-visible API
 *
 * The OML API is used to generate measurement streams (MS) using the \ref omsp
 * "OML Measurement Stream Protocol".
 *
 * The client library implement Injection Points. The user-visible AP consists
 * in a few primitives:
 *
 *   - init (\ref omlc_init)
 *   - start (\ref omlc_start)
 *   - addMP (\ref omlc_add_mp)
 *   - inject (\ref omlc_inject)
 *   - injectMetadata (\ref omlc_inject_metadata)
 *   - close (\ref omlc_close)
 *
 * The mapping between API calls and \ref omsp "OMSP" output can be summarised as follows.
 *
 * @startuml{api2omsp.png}
 * Application --> InjectionPoint: init
 *
 * Application --> InjectionPoint: addMP(1)
 * InjectionPoint --> InjectionPoint: add MP 1 definition to headers or\nlist to declare later via schema0
 *
 * Application --> InjectionPoint: start
 * note right: with support of schema0 and addition of MP at any time,\nstart could be a noop
 *
 * group Connection/headers can be sent immediately, or wait for first injection
 *
 * InjectionPoint --> CollectionPoint: initial headers
 *
 * Application --> InjectionPoint: addMP(2)
 * InjectionPoint --> CollectionPoint: schema0 MP 2 definition
 *
 *
 * Application --> InjectionPoint: addMP(3)
 * InjectionPoint --> CollectionPoint: schema0 MP 3 definition
 * end
 *
 * group Data can be buffered after timestamping
 * Application --> InjectionPoint: inject(1)
 * InjectionPoint --> InjectionPoint: timestamp data
 * InjectionPoint --> CollectionPoint: schema1 timestamp + data
 *
 * Application --> InjectionPoint: inject(3)
 * note right: injections can happen in any order
 * InjectionPoint --> InjectionPoint: timestamp data
 * InjectionPoint --> CollectionPoint: schema3 timestamp + data
 *
 * Application --> InjectionPoint: inject(2)
 * InjectionPoint --> InjectionPoint: timestamp data
 * InjectionPoint --> CollectionPoint: schema2 timestamp + data
 *
 * Application --> InjectionPoint: injectMetadata
 * note right: subject (.[MPNAME[.FNAME]]), key, value
 * InjectionPoint --> InjectionPoint: timestamp metadata
 * InjectionPoint --> CollectionPoint: schema0 metadata
 *
 * Application --> InjectionPoint: addMP(4)
 * note right: MPs can be added at any time
 * InjectionPoint --> CollectionPoint: schema0 MP 4 definition
 *
 * Application --> InjectionPoint: inject(4)
 * InjectionPoint --> InjectionPoint: timestamp data
 * InjectionPoint --> CollectionPoint: schema4 timestamp + data
 * end
 *
 * == Network error ==
 *
 * Application --> InjectionPoint: inject(3)
 * InjectionPoint --> InjectionPoint: timestamp data
 * InjectionPoint --> CollectionPoint: initial headers
 * note right: first reestablish the connection\nand resend headers
 * InjectionPoint --> CollectionPoint: all schema0
 * note right: schema0 MP definitions are required,\nculling metadata might be considered at some point
 * InjectionPoint --> CollectionPoint: schema3 timestamp + data
 *
 * Application --> InjectionPoint: close
 * InjectionPoint --> CollectionPoint: all buffered data
 * @enduml
 */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include <sys/time.h>

#include "oml2/oml_filter.h"
#include "oml2/omlc.h"
#include "ocomm/o_log.h"
#include "oml_value.h"
#include "validate.h"
#include "mem.h"
#include "client.h"

static void omlc_ms_process(OmlMStream* ms);

extern OmlMP* schema0;

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

  if (NULL == omlc_instance || omlc_instance->start_time <= 0) {
    logerror("Cannot inject samples prior to calling omlc_init and omlc_start\n");
    return -1;
  }
  if (mp == NULL || values == NULL) {
    return -1;
  }

  LOGDEBUG("Injecting data into MP '%s'\n", mp->name);

  oml_value_init(&v);
  if (mp_lock(mp) == -1) {
    logwarn("Cannot lock MP '%s' for injection\n", mp->name);
    return -1;
  }

  uint64_t written = 0;
  uint64_t dropped = 0;
  for (ms = mp->streams; ms; ms = ms->next) {
    LOGDEBUG("Filtering MP '%s' data into MS '%s'\n", mp->name, ms->table_name);
    OmlFilter* f = ms->filters;
    for (; f != NULL; f = f->next) {

      /* FIXME:  Should validate this indexing */
      oml_value_set(&v, &values[f->index], mp->param_defs[f->index].param_types);

      f->input(f, &v);
    }
    omlc_ms_process(ms);
    written += ms->written;
    dropped += ms->dropped;
  }
  mp_unlock(mp);
  oml_value_reset(&v);

  /* do we need to send client instrumentation? */
  if(mp != omlc_instance->client_instr && omlc_instance->instr_interval) {
    time_t now;
    time(&now);
    if(omlc_instance->instr_time + omlc_instance->instr_interval <= now) {
      OmlValueU values[6];
      omlc_zero_array(values, 6);
      omlc_set_uint32(values[0], written);
      omlc_set_uint32(values[1], dropped);
      omlc_set_uint64(values[2], xmemnew());
      omlc_set_uint64(values[3], xmemfreed());
      omlc_set_uint64(values[4], xmembytes());
      omlc_set_uint64(values[5], xmaxbytes());
      omlc_inject(omlc_instance->client_instr, values);
      omlc_instance->instr_time = now;
    }
  }

  return 0;
}

/** Inject metadata (key/value) for a specific MP.
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
  int ret = -1;

  OmlValueU v[3];
  omlc_zero_array(v, 3);
  MString *subject = mstring_create();

  if (NULL == omlc_instance || omlc_instance->start_time <= 0) {
    logerror("Cannot inject metadata prior to calling omlc_init and omlc_start\n");

  } else if (!key || !value) {
    logwarn("Trying to inject metadata with missing key and/or value\n");

  } else if (! validate_name(key)) {
    logerror("%s is an invalid metadata key name\n", key);

  } else if (type != OML_STRING_VALUE) {
    logwarn("Currently, only OML_STRING_VALUE are valid as metadata value\n");

  } else {
    mstring_set (subject, ".");
    if(NULL == mp) {
      logdebug("%s: supplied MP is NULL, assuming experiment metadata\n",
          __FUNCTION__);

    } else {
      /* XXX: At the moment, MS are named as APPNAME_MPNAME.
       * Be consistent with it here, until #1055 is addressed */
      mstring_sprintf(subject, "%s_%s", omlc_instance->app_name, mp->name);
      if (fname) {
        if (find_mp_field(fname, mp) < 0) {
          logwarn("Field %s not found in MP %s, not reporting\n", fname, mp->name);

        } else {
          mstring_sprintf(subject, ".%s", fname);

        }
      }
    }
    omlc_set_string(v[0], mstring_buf(subject));
    omlc_set_string(v[1], key);
    omlc_copy_string(v[2], *value);

    omlc_inject(schema0, v);

    omlc_reset_string(v[0]);
    omlc_reset_string(v[1]);
    omlc_reset_string(v[2]);
    mstring_delete(subject);

    ret = 0;
  }

  return ret;
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
    LOGDEBUG("Generating new sample for MS '%s'\n", ms->table_name);
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
