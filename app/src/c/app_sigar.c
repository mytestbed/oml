/*
 *  C Implementation: app_sigar
 *
 * Description:  An application based on the libsigar that communicates with 
 * 				 the oml library
 *
 *
 * Author: Guillaume Jourjon <guillaume.jourjon@nicta.com.au>, (C) 2008
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




#include <errno.h>
#include <string.h>
#include "oml2/omlc.h"
#include "app_sigar.h"
#include "oml2/oml_filter.h"
#include "client.h"
#include "version.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pcap.h>
#include <malloc.h>
#include <errno.h>
#include "log.h"


OmlSigar* sigar_mp; 


char *ulltostr(sigar_uint64_t value, char *ptr, int base)
{
  sigar_uint64_t t = 0, res = 0;
  sigar_uint64_t tmp = value;
  int count = 0;
 
  if (NULL == ptr)
  {
    return NULL;
  }
 
  if (tmp == 0)
  {
    count++;
  }
 
  while(tmp > 0)
  {
    tmp = tmp/base;
    count++;
  }
 
  ptr += count;
 
  *ptr = '\0';
 
  do
  {
    res = value - base * (t = value / base);
    if (res < 10)
    {
      *--ptr = '0' + res;
    }
    else if ((res >= 10) && (res < 16))
    {
        *--ptr = 'A' - 10 + res;
    }
  } while ((value = t) != 0);
 
  return(ptr);
}







OmlSigar* create_sigar_measurement(
    char* name
){
	OmlSigar* self = (OmlSigar*)malloc(sizeof(OmlSigar));
	  memset(self, 0, sizeof(OmlSigar));
	  
	  OmlMPDef* def = create_sigar_filter(name);
	  self->def = def;
	  
	  self->mp = omlc_add_mp("omlsigar", def);

	  //sigar_open(&self->system_info);
	  
//	  sigar_net_interface_stat_get(self->system_info,
//	                               ,
//	                               self->interface_stat);
//	TODO put this in another configuration function just before the omlc_start
	  
	  //sigar_mem_get(self->system_info, self->memory_info);
	  
	  //sigar_cpu_get(self->system_info, self->cpu_info);
	  
	  
	  return self;
	
}

/**
 * \fn OmlMPDef* createSigarFilter(char* file)
 * \brief function called to create OML Definition 
 *
 * \param file the file that contains the pcap filter command
 * \return the OML Definition for the creation of the Measurement points
 */
OmlMPDef* create_sigar_filter(char* file)
{
  int cnt = 0;
  OmlMPDef* self = NULL;
  int j = 0;

    //o_log(O_LOG_INFO, "Creation of pcap default conf\n");
    cnt = 6; 
    self = (OmlMPDef*)malloc(cnt*sizeof(OmlMPDef));
 
    self[0].name = "rx_bytes";
    self[0].param_types = OML_LONG_VALUE;

    self[1].name = "tx_bytes";
    self[1].param_types = OML_LONG_VALUE;
    self[2].name = "ram_used";
    self[2].param_types = OML_STRING_PTR_VALUE;
    self[3].name = "cpu_user";
    self[3].param_types = OML_LONG_VALUE;
    self[4].name = "cpu_total_used";
    self[4].param_types = OML_LONG_VALUE;
    self[5].name = NULL;
    self[5].param_types = (OmlValueT)0;

  
  
  
  return self;
}

int
main(
		int argc,
		const char *argv[]
) {
	OmlValueU v[4];
    sigar_t *sigar_t;
    sigar_mem_t memory_info;
    sigar_cpu_t cpu_info;
    sigar_net_interface_stat_t interface_stat;
	int* argc_;
	const char** argv_;
	char mem_used[64];
	omlc_init(argv[ 0 ], &argc, argv, o_log);

	argc_ =&argc;
	argv_ = argv;

	OmlSigar* sigar = NULL; 
	char* if_sigar = NULL;
	char src_pcap[50] = " ";
	char dst_pcap[50] = " ";
	int period = 1;
	int disconnected = 0;
	int i =0;

	
	for (i = *argc_; i > 0; i--, *argv_++) {
		if (strcmp(*argv_, "--sigar") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--sigar'\n");
				return 0;
			}
			
			sigar = create_sigar_measurement(*++argv_);
			*argc_ -= 2;
		}else if (strcmp(*argv_, "--sigar-if") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--sigar-if'\n");
				return 0;
			}
			
			if_sigar = (char*)*++argv_;
			*argc_ -= 2;
		}else if (strcmp(*argv_, "--sigar-period") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--sigar-period'\n");
				return 0;
			}
			
			period = atoi(*++argv_);
			*argc_ -= 2;
		}
			//printf("Creation of pcap %s\n ", *argv_);

	}
	
	sigar_mp = sigar;
	
	if(sigar_mp != NULL){
		sigar_mp->name_interface = if_sigar;
		//pcap_mp->promiscuous = promisc;
		sigar_mp->granularity = period;
		
		omlc_start();

	while(1){
		sigar_open(&sigar_t);
		sigar_mem_get(sigar_t, &memory_info);
		sigar_net_interface_stat_get(sigar_t, if_sigar/*"eth1"self->name_interface*/, &interface_stat);	  
			  
		
		sigar_cpu_get(sigar_t, &cpu_info);
		
		ulltostr(memory_info.used,(char*) mem_used, 10);
		
		v[0].longValue = (long)interface_stat.rx_bytes;
		v[1].longValue = (long)interface_stat.tx_bytes;
		v[2].stringPtrValue = (char*) mem_used;
		//printf("memory %s \n", mem_used);
		v[3].longValue = (long)cpu_info.user;
		v[4].longValue = (long)cpu_info.total;
		
		
		omlc_process(sigar_mp->mp, v);

	    sigar_close(sigar_t);
	    
	    sleep(1);
		
	}
		
		
	}else{
		printf("exit");
		exit(0);
	}
	exit(0);	
}
