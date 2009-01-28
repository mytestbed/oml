/**
 * C++ Interface: app_sigar
 *
 * Description: 
 * \file   app_sigar.h
 * \brief  the lib sigar for an OML application
 * \author Guillaume Jourjon <guillaume.jourjon@nicta.com.au>, (C) 2009
 *
 * Copyright (c) 2007-2008 National ICT Australia (NICTA)
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
 * THE SOFTWARE. *
 */

#ifndef APP_SIGAR_H_
#define APP_SIGAR_H_


#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include "sigar.h"
#include <pthread.h>

#define O_LOG_ERROR -2
#define O_LOG_WARN  -1
#define O_LOG_INFO  0
#define O_LOG_DEBUG 1
#define O_LOG_DEBUG2 2
#define O_LOG_DEBUG3 3
#define O_LOG_DEBUG4 4



typedef struct _omlSigar{
  char* name;
  sigar_t* system_info;
  sigar_mem_t* memory_info;
  sigar_cpu_t* cpu_info;
  sigar_net_interface_stat_t* interface_stat;//TODO check how to compare with name
  char* name_interface;
  OmlMP* mp;
  OmlMPDef* def;
  unsigned long granularity;
  pthread_t thread_pcap;
} OmlSigar;



OmlSigar* create_sigar_measurement(
    char* name
);

OmlMPDef* create_sigar_filter(
    char* file
                            );
                            
char *ulltostr(sigar_uint64_t value, char *ptr, int base);

#endif /*APP_SIGAR_H_*/
