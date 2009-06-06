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

#ifndef NMETRICS_OML_H
#  define NMETRICS_OML_H

#include <oml2/omlc.h>

static OmlMPDef oml_memory_def[] = {
  {"ram", OML_LONG_VALUE},
  {"total", OML_LONG_VALUE},
  {"used",  OML_LONG_VALUE},
  {"free", OML_LONG_VALUE},
  {"actual_used", OML_LONG_VALUE},
  {"actual_free", OML_LONG_VALUE},
  {NULL, (OmlValueT)0}
};

static OmlMPDef oml_cpu_def[] = {
  {"user",  OML_LONG_VALUE},
  {"sys", OML_LONG_VALUE},
  {"nice", OML_LONG_VALUE},
  {"idle", OML_LONG_VALUE},
  {"wait", OML_LONG_VALUE},
  {"irq", OML_LONG_VALUE},
  {"soft_irq", OML_LONG_VALUE},
  {"stolen", OML_LONG_VALUE},
  {"total", OML_LONG_VALUE},
  {NULL, (OmlValueT)0}
};

static OmlMPDef oml_network_def[] = {
  {"name", OML_STRING_PTR_VALUE},
  {"rx_packets", OML_LONG_VALUE},
  {"rx_bytes", OML_LONG_VALUE},
  {"rx_errors", OML_LONG_VALUE},
  {"rx_dropped", OML_LONG_VALUE},
  {"rx_overruns", OML_LONG_VALUE},
  {"rx_frame", OML_LONG_VALUE},
  {"tx_packets", OML_LONG_VALUE},
  {"tx_bytes", OML_LONG_VALUE},
  {"tx_errors", OML_LONG_VALUE},
  {"tx_dropped", OML_LONG_VALUE},
  {"tx_overruns", OML_LONG_VALUE},
  {"tx_collisions", OML_LONG_VALUE},
  {"tx_carrier", OML_LONG_VALUE},
  {"speed", OML_LONG_VALUE},
  {NULL, (OmlValueT)0}
};

static OmlMPDef oml_proc_def[] = {
  {"cpu_id", OML_LONG_VALUE},
  {"total", OML_LONG_VALUE},
  {"sleeping", OML_LONG_VALUE},
  {"running", OML_LONG_VALUE},
  {"zombie", OML_LONG_VALUE},
  {"stopped", OML_LONG_VALUE},
  {"idle", OML_LONG_VALUE},
  {"threads", OML_LONG_VALUE},
  {NULL, (OmlValueT)0}
};
// int sigar_proc_stat_get(sigar_t *sigar, sigar_proc_stat_t *procstat);

static OmlMPDef oml_proc_time_def[] = {
  {"pid", OML_LONG_VALUE},
  {"start_time", OML_LONG_VALUE},
  {"user", OML_LONG_VALUE},
  {"sys", OML_LONG_VALUE},
  {"total", OML_LONG_VALUE},
  {NULL, (OmlValueT)0}
};

#endif
