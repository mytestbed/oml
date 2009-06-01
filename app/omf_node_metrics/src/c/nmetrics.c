/*
 *  OML application reporting various node metrics, such as cpu, memory, ...
 *
 * Description:  This application is using the libsigar library to report regularily
 *    through the OML framework on various node metrics, such as cpu load, memory 
 *    and network usage.
 *
 * Author: Guillaume Jourjon <guillaume.jourjon@nicta.com.au>, (C) 2008
 * Author: Max Ott  <max.ott@nicta.com.au>, (C) 2009
 *
 * Copyright (c) 2007-2009 National ICT Australia (NICTA)
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
*/

#include <stdlib.h>
#include <string.h>

#define USE_OPTS
#include "nmetrics_opts.h"

#include "nmetrics_oml.h"

#include "o_log.h"

OmlMP*   cpu_mp;
OmlMP*   memory_mp;
OmlMP*   net_mp;

#include <sigar.h>

typedef struct _if_monitor_t {
  char           if_name[64];

  int            not_first;
  sigar_uint64_t start_rx_packets;
  sigar_uint64_t start_rx_bytes;
  sigar_uint64_t start_rx_errors;
  sigar_uint64_t start_rx_dropped;
  sigar_uint64_t start_rx_overruns;
  sigar_uint64_t start_rx_frame;
  sigar_uint64_t start_tx_packets;
  sigar_uint64_t start_tx_bytes;
  sigar_uint64_t start_tx_errors;
  sigar_uint64_t start_tx_dropped;
  sigar_uint64_t start_tx_overruns;
  sigar_uint64_t start_tx_collisions;
  sigar_uint64_t start_tx_carrier;
  
  struct _if_monitor_t* next;
} if_monitor_t;

static void  run(if_monitor_t* first_if);

int
main(
  int argc,
  const char *argv[]
) {
  char c;
  if_monitor_t* first = NULL;
  if_monitor_t* if_p;

  omlc_init(argv[0], &argc, argv, NULL); 

  // parsing command line arguments
  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  while ((c = poptGetNextOpt(optCon)) >= 0) {
    switch (c) {
    case 'i': {
      if_monitor_t* im = (if_monitor_t *)malloc(sizeof(if_monitor_t));
      memset(im, 0, sizeof(if_monitor_t));
      sprintf(im->if_name, "%s", g_opts->if_name);
      im->next = first;
      first = im;

      //printf("IF: %s\n", if_name);
      break;
    }
    }
  }

  // Initialize measurment points
  if (g_opts->report_cpu) {
    cpu_mp = omlc_add_mp("cpu", oml_cpu_def);
  }
  if (g_opts->report_memory) {
    memory_mp = omlc_add_mp("memory", oml_memory_def);
  }
  if (first) {
    net_mp = omlc_add_mp("net_if", oml_network_def);
  }
  omlc_start();

  run(first);
  return(0);
}

static void
cpu(
  sigar_t* sigar_p,
  OmlMP*   mp_p
) {
  OmlValueU v[9];
  sigar_cpu_t c;

  sigar_cpu_get(sigar_p, &c);
  
  // NOTE: We cast from a u_64 to a long
  v[0].longValue = (long)c.user;
  v[1].longValue = (long)c.sys;
  v[2].longValue = (long)c.nice;
  v[3].longValue = (long)c.idle;
  v[4].longValue = (long)c.wait;
  v[5].longValue = (long)c.irq;
  v[6].longValue = (long)c.soft_irq;
  v[7].longValue = (long)c.stolen;
  v[8].longValue = (long)c.total;
  omlc_process(mp_p, v);
}

static void
memory(
  sigar_t* sigar_p,
  OmlMP*   mp_p
) {
  OmlValueU v[6];
  sigar_mem_t m;

  sigar_mem_get(sigar_p, &m);

  // NOTE: We cast from a u_64 to a long
  v[0].longValue = (long)(m.ram / 1000);
  v[1].longValue = (long)(m.total / 1000);
  v[2].longValue = (long)(m.used / 1000);
  v[3].longValue = (long)(m.free / 1000);
  v[4].longValue = (long)(m.actual_used / 1000);
  v[5].longValue = (long)(m.actual_free / 1000);
  omlc_process(mp_p, v);
}

static void
network_if(
  sigar_t*      sigar_p,
  if_monitor_t* net_if
) {
  OmlValueU v[15];
  sigar_net_interface_stat_t is;

  sigar_net_interface_stat_get(sigar_p, net_if->if_name, &is);
  if (! net_if->not_first) {
    net_if->start_rx_packets = is.rx_packets;
    net_if->start_rx_bytes = is.rx_bytes;
    net_if->start_rx_errors = is.rx_errors;
    net_if->start_rx_dropped = is.rx_dropped;
    net_if->start_rx_overruns = is.rx_overruns;
    net_if->start_rx_frame = is.rx_frame;
    net_if->start_tx_packets = is.tx_packets;
    net_if->start_tx_bytes = is.tx_bytes;
    net_if->start_tx_errors = is.tx_errors;
    net_if->start_tx_dropped = is.tx_dropped;
    net_if->start_tx_overruns = is.tx_overruns;
    net_if->start_tx_collisions = is.tx_collisions;
    net_if->start_tx_carrier = is.tx_carrier;
    net_if->not_first = 1;
  }

  static char name[34] = "foo";

  v[0].stringPtrValue = net_if->if_name;
  //  v[0].stringPtrValue = &name;
  v[1].longValue = (long)(is.rx_packets - net_if->start_rx_packets);
  v[2].longValue = (long)(is.rx_bytes - net_if->start_rx_bytes);
  v[3].longValue = (long)(is.rx_errors - net_if->start_rx_errors);
  v[4].longValue = (long)(is.rx_dropped - net_if->start_rx_dropped);
  v[5].longValue = (long)(is.rx_overruns - net_if->start_rx_overruns);
  v[6].longValue = (long)(is.rx_frame - net_if->start_rx_frame);
  v[7].longValue = (long)(is.tx_packets - net_if->start_tx_packets);
  v[8].longValue = (long)(is.tx_bytes - net_if->start_tx_bytes);
  v[9].longValue = (long)(is.tx_errors - net_if->start_tx_errors);
  v[10].longValue = (long)(is.tx_dropped - net_if->start_tx_dropped);
  v[11].longValue = (long)(is.tx_overruns - net_if->start_tx_overruns);
  v[12].longValue = (long)(is.tx_collisions - net_if->start_tx_collisions);
  v[13].longValue = (long)(is.tx_carrier); // - net_if->start_tx_carrier);
  v[14].longValue = (long)(is.speed / 1000000);

  omlc_process(net_mp, v);
}


static void
run(
  if_monitor_t* first_if
) {
  sigar_t* sigar_p;
  if_monitor_t* if_p;

  while(1) {
    sigar_open(&sigar_p);

    if (g_opts->report_cpu) {
      cpu(sigar_p, cpu_mp);
    }
    if (g_opts->report_memory) {
      memory(sigar_p, memory_mp);
    }

    if_p = first_if;
    while (if_p) {
      network_if(sigar_p, if_p);
      if_p = if_p->next;
    }
    
    sigar_close(sigar_p);
    sleep(g_opts->sample_interval);
  }
}
