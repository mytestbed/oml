/*
 * Copyright 2007-2013 National ICT Australia (NICTA)
 *
 * This software may be used and distributed solely under the terms of
 * the MIT license (License).  You should find a copy of the License in
 * COPYING or at http://opensource.org/licenses/MIT. By downloading or
 * using this software you accept the terms and the liability disclaimer
 * in the License.
 */
/**\file filter.c
 * \brief Implements OML's client side filter engine
 */

#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#define _XOPEN_SOURCE 500     /* Or: #define _BSD_SOURCE; for useconds_t to be defined */
#include <unistd.h>

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "oml2/oml_writer.h"
#include "ocomm/o_log.h"
#include "client.h"

static void* thread_start(void* handle);

extern OmlClient* omlc_instance;

/** Start the filtering engine on the given MS
 * \param ms pointer to OmlMStream to start filtering on
 */
void
filter_engine_start(OmlMStream* ms)
{
  pthread_create(&ms->filter_thread, NULL, thread_start, (void*)ms);
}

/** Filtering thread
 *
 * Loops until the stream is not active anymore, or an error occurs
 *
 * \param handle pointer to OmlMStream to use the filters on (cast as void*)
 *
 * \return NULL, inconditionally; shouldn't return in normal operation
 */
static void*
thread_start(void* handle)
{
  OmlMStream* ms = (OmlMStream*)handle;
  OmlMP* mp = ms->mp;
  useconds_t usec = (useconds_t)(1000000 * ms->sample_interval);
  int status = 0;

  while (1) {
    usleep(usec);
    if (!mp_lock(mp)) {
      if (!mp->active) {
        mp_unlock(mp);
        return NULL;  // we are done
      }

      status = filter_process(ms);
      mp_unlock(mp);
    }

    if (status == -1) {
      return NULL; // Fatal error --> exit thread
    }

  }
}

/** Run filters associated to an MS.
 *
 * Get the writer associated to the MS, and generate and write initial metadata
 * (seqno and time). Then, instruct all the filters, in sequence, to write
 * their filtered sample to this writer before finalising the write.
 *
 * \param ms MS to generate output for
 * \return 0 if success, -1 otherwise
 *
 * \see OmlWriter, oml_writer_row_start, oml_writer_out, oml_writer_row_end
 */
int
filter_process(OmlMStream* ms)
{
  struct timeval tv;
  double now;
  OmlFilter *f;

  if (ms == NULL || omlc_instance == NULL) {
    logerror("Could not process filters because of null measurement stream or instance\n");
    return -1;
  }

  OmlWriter* writer = ms->writer;

  if (writer == NULL) {
    logerror("Could not process filters because of null writer\n");
    return -1;
  }

  gettimeofday(&tv, NULL);
  now = tv.tv_sec - omlc_instance->start_time + 0.000001 * tv.tv_usec;

  /* Be aware that row_start is obtaining a lock on the writer
   * which is released in row_end. Always ensure that row_end is
   * called, even if there is a problem somewhere along the way.
   * \see oml_writer_row_start, oml_writer_out, oml_writer_row_end
   */
  ms->seq_no++;
  writer->row_start(writer, ms, now);
  f = ms->firstFilter;
  for (; f != NULL; f = f->next) {
    f->output(f, writer);
  }
  writer->row_end(writer, ms);
  ms->sample_size = 0;

  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 2
 indent-tabs-mode: nil
 End:
 vim: sw=2:sts=2:expandtab
*/
