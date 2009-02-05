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
/*!\file filter.c
  \brief Implements OML's client side filter engine
*/

#include <ocomm/o_log.h>
#include <string.h>
#include <malloc.h>
#include <pthread.h>
#include <errno.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#define _XOPEN_SOURCE 500     /* Or: #define _BSD_SOURCE */
#include <unistd.h>

#define useconds_t unsigned long

#include "oml2/omlc.h"
#include "oml2/oml_filter.h"
#include "oml2/oml_writer.h"
#include "client.h"

static void* thread_start(void* handle);
//static int process(OmlMStream* mp, OmlValue* values,  int max_value_count);
//static void sample_based(OmlMStream* ms);
//static void send(OmlValue* values, int value_count);

extern OmlClient* omlc_instance;
/**
 * \fn void filter_engine_start( OmlMStream* ms )
 * \brief function that will start the filter thread
 * \param ms the stream to filter
 */
void
filter_engine_start(
  OmlMStream* ms
) {
  pthread_create(&ms->filter_thread, NULL, thread_start, (void*)ms);
}
/**
 * \fn static void* thread_start(void* handle)
 * \brief start the filter thread
 * \param handle the stream to use the filters on
 */
static void*
thread_start(
  void* handle
) {
  OmlMStream* ms = (OmlMStream*)handle;
  OmlMP* mp = ms->mp;
  useconds_t usec = (useconds_t)(1000000 * ms->sample_interval);

  while (1) {
    usleep(usec);

    if (!mp_lock(mp)) {
      if (!mp->active) {
        mp_unlock(mp);
        return;  // we are done
      }

      //o_log(O_LOG_INFO, "Sample3 %d\n", usec);
      filter_process(ms);
      mp_unlock(mp);
    }
  }
}


/**
 * \fn int filter_process( OmlMStream* ms )
 * \brief Run filters on all queues in MP.
 * \param ms the stream to filterise
 * \return 1 if success, 0 if not.
 */
int
filter_process(
  OmlMStream* ms
) {
  int i;
  OmlWriter* writer = ms->writer;

  struct timeval tv;
  gettimeofday(&tv, NULL);
  double now = tv.tv_sec - omlc_instance->start_time + 0.000001 * tv.tv_usec;

  ms->seq_no++;
  writer->row_start(writer, ms, now);
  OmlFilter* f = ms->firstFilter;
  for (; f != NULL; f = f->next) {
    f->process(f, writer);
  }
  writer->row_end(writer, ms);
  ms->sample_size = 0;
  return 0;
}

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
