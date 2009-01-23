/*
 *  C Implementation: app_pcap
 *
 * Description:  An application based on the libpcap that communicates with 
 * 				the oml library
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



#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <pcap.h>
#include <malloc.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h> 
#include <net/ethernet.h>
#include <netinet/ether.h> 
#include <netinet/ip.h> 
/* tcpdump header (ether.h) defines ETHER_HDRLEN) */
//#ifndef ETHER_HDRLEN 
//#define ETHER_HDRLEN 14
//#endif
#include <string.h>
#include "oml2/omlc.h"
//#include "oml2/omlc_pcap.h"
#include "oml2/oml_filter.h"
//#include "client.h"
//#include "filter/builtin.h"
//#include "version.h"
#include "app_pcap.h"


//static OmlPcap pcap;

OmlPcap* pcap_mp;

extern int errno;

static void o_log_default(int log_level, const char* format, ...); 

static FILE* logfile;
int o_log_level = O_LOG_INFO;

o_log_fn o_log = o_log_default;

void
o_set_log_file(
		  char* name
	      ) {
  if (*name == '-') {
      logfile = NULL;
      setlinebuf(stdout);
  } else {
      logfile = fopen(name, "a");
      if (logfile == NULL) {
          fprintf(stderr, "Can't open logfile '%s'\n", name);
          return;
      }
  setvbuf ( logfile, NULL , _IOLBF , 1024 );
  }
}

void
o_set_log_level(
		  int level
	       ) {
  o_log_level = level;
}

o_log_fn 
o_set_log(
	  o_log_fn new_log_fn
	 ) {
  if ((o_log = new_log_fn) == NULL) {
      o_log = o_log_default;
  }
  return o_log;
}

void 
o_log_default(
  int log_level, 
  const char* format, 
  ...) {
  va_list va;
  if (log_level > o_log_level) return;
  va_start(va, format);

  if (logfile == NULL) {
    switch (log_level) {
        case O_LOG_INFO:
            printf("# ");
            vprintf(format, va);
          break;
     case O_LOG_WARN:
        fprintf(stdout, "# WARN ");
      vfprintf(stdout, format, va);
           break;
     case O_LOG_ERROR:
          fprintf(stderr, "# ERROR ");
        vfprintf(stderr, format, va);
      break;
      default:
          printf("# ");
          for (; log_level > 0; log_level--) {
              printf("..");
        }
        printf(" ");
      vprintf(format, va);
            break;
   }
			        
   } else {
       time_t t;
       time(&t);
       struct tm* ltime = localtime(&t);
       char now[20];
       strftime(now, 20, "%b %d %H:%M:%S", ltime);
       if (log_level > O_LOG_INFO) {
            int dlevel = log_level - O_LOG_INFO;
            if (dlevel > 1)
	          fprintf(logfile, "%s  DEBUG%d ", now, dlevel);
           else
     	          fprintf(logfile, "%s  DEBUG  ", now);
           vfprintf(logfile, format, va);
       } else {
           switch (log_level) {
	      case O_LOG_INFO:
                  fprintf(logfile, "%s  INFO   ", now);
                  vfprintf(logfile, format, va);
                  break;
              case O_LOG_WARN:
                  fprintf(logfile, "%s  WARN   ", now);
                  vfprintf(logfile, format, va);
                  break;
              case O_LOG_ERROR:
                  fprintf(logfile, "%s  ERROR  ", now);
                  vfprintf(logfile, format, va);
                  break;
              default:
                  fprintf(logfile, "%s  UNKNOWN  ", now);
                  vfprintf(logfile, format, va);
                  break;
           }
        }    
       }
       va_end(va);
}


/**
 * \fn void packet_treatment( uchar* useless, const struct pcap_pkthdr* pkthdr, const u_char* pkt)
 * \brief function that will be called each time a packet is capture
 * 
 * \param useless not used but required by the pcap library
 * \param pkthdr the header of the packet at the ethernet level
 * \param pkt a representation of the packet
 */
void packet_treatment( u_char* useless, const struct pcap_pkthdr* pkthdr, const u_char* pkt)
{
	OmlValueU* value = NULL;

	if(strcmp(pcap_mp->name,"default") == 0){
		value = (OmlValueU*) malloc(4*sizeof(OmlValueU));
		memset(value, 0, 4*sizeof(OmlValueU));
	}else{
		value = (OmlValueU*) malloc(5*sizeof(OmlValueU));
		memset(value, 0, 5*sizeof(OmlValueU));
	}
	u_int16_t type = handle_ethernet(useless,pkthdr,pkt,value);


	if(type == ETHERTYPE_IP)
	{/* handle IP packet */
		handle_IP(useless,pkthdr,pkt, value);
	}
	//printf("value %s %s %d \n", value[0].stringPtrValue, value[1].stringPtrValue, value[2].longValue);



	if(value != NULL){
		//printf("value %d %d %d \n", &value[0].stringPtrValue, &value[1].stringPtrValue, value[2].longValue);
		omlc_process(pcap_mp->mp, value);

	}
	free(value);

}

/**
 * \fn OmlPcap* create_pcap_measurement(char* file)
 * \brief Function called at the initialisation of the OML client 
 * 
 * \param file the file that contains the pcap filtering command
 * \return an instance of an OmlPcap
 */
OmlPcap* create_pcap_measurement(char* file)
{
	OmlPcap* self = (OmlPcap*)malloc(sizeof(OmlPcap));
	memset(self, 0, sizeof(OmlPcap));

	self->name = file;
	OmlMPDef* def = create_pcap_filter(file);
	self->def = def;

	if(strcmp(file,"default") == 0)
		;
	else{
		FILE *fp1;
		char oneword[256];
		char *c;
		fp1 = fopen(file, "r");
		if (fp1 == NULL)
		{
			// printf("File failed to open\n");
			// exit (EXIT_FAILURE);
		}
		c =  fgets(oneword, 256, fp1);
		strcat(self->filter_exp,c);
		fclose(fp1);
	}


	return self;
}
/**
 * \fn void pcap_engine_start(OmlPcap* pcap)
 * \brief creation of a new thread that will make the pcap measurement
 *
 * \param pcap an instance of the OmlPcap structure 
 */
void
pcap_engine_start(
		OmlPcap* pcap
) {
	pthread_create(&pcap->thread_pcap, NULL, thread_pcapstart, (void*)pcap);
}

static void* thread_pcapstart(void* handle) 
{
	OmlPcap* pcap = (OmlPcap*)handle;

	//pcap->dev = pcap_lookupdev(self->errbuf);
	if(pcap->dev == NULL)
	{ 
		pcap->dev = pcap_lookupdev(pcap->errbuf);
		if(pcap->dev == NULL){
			printf("%s\n",pcap->errbuf); 
			exit(1); 
		}
	}
	pcap_lookupnet(pcap->dev,&pcap->netp,&pcap->maskp,pcap->errbuf);

	//pcap->mp = omlc_add_mp("pcap", omlc_instance->pcap_mp->def);

	pcap->descr = pcap_open_live(pcap->dev,BUFSIZ,pcap->promiscuous,-1,pcap->errbuf);
	if(pcap->descr == NULL)
	{ printf("pcap_open_live(): %s\n",pcap_mp->errbuf); exit(1); }


	if(pcap->filter_exp != NULL)
	{

		//     /* Lets try and compile the program.. non-optimized */
		if(pcap_compile(pcap->descr,&pcap->fp,pcap->filter_exp,0,pcap->netp) == -1)
		{ 
			fprintf(stderr,"Error calling pcap_compile\n"); 
			exit(1); 
		}
		// 
		/* set the compiled program as the filter */
		if(pcap_setfilter(pcap->descr,&pcap->fp) == -1){ 
			fprintf(stderr,"Error setting filter\n"); 
			exit(1); 
		}
	}

	/* ... and loop */ 
	pcap_loop(pcap_mp->descr,-1,packet_treatment,(unsigned char*)NULL);
}


/**
 * \fn void preparation_pcap(OmlPcap* pcap)
 * \brief creation of a measurement point
 *
 * \param pcap an instance of the OmlPcap structure 
 */
void preparation_pcap(OmlPcap* pcap){

	pcap->mp = omlc_add_mp("pcap", pcap->def);

}



/**
 * \fn OmlMPDef* createPcapFilter(char* file)
 * \brief function called to create OML Definition 
 *
 * \param file the file that contains the pcap filter command
 * \return the OML Definition for the creation of the Measurement points
 */
OmlMPDef* create_pcap_filter(char* file)
{
	int cnt = 0;
	OmlMPDef* self = NULL;
	int j = 0;
	if(strcmp(file,"default")==0){
		o_log(O_LOG_INFO, "Creation of pcap default conf\n");
		cnt = 5; 
		self = (OmlMPDef*)malloc(cnt*sizeof(OmlMPDef));

		self[0].name = "mac_src";
		self[0].param_types = OML_STRING_PTR_VALUE;

		self[1].name = "ip_src";
		self[1].param_types = OML_STRING_PTR_VALUE;
		self[2].name = "ip_dst";
		self[2].param_types = OML_STRING_PTR_VALUE;
		self[3].name = "length";
		self[3].param_types = OML_LONG_VALUE;
		self[4].name = NULL;
		self[4].param_types = (OmlValueT)0;
	}else {
		o_log(O_LOG_INFO, "Creation of pcap default conf\n");
		cnt = 6;
		self = (OmlMPDef*)malloc(cnt*sizeof(OmlMPDef));

		self[0].name = "mac_src";
		self[0].param_types = OML_STRING_PTR_VALUE;

		self[1].name = "ip_src";
		self[1].param_types = OML_STRING_PTR_VALUE;
		self[2].name = "ip_dst";
		self[2].param_types = OML_STRING_PTR_VALUE;
		self[3].name = "length";
		self[3].param_types = OML_LONG_VALUE;
		self[4].name = "seq_num";
		self[4].param_types = OML_LONG_VALUE;
		self[5].name = NULL;
		self[5].param_types = (OmlValueT)0;


	}



	return self;
}
OmlValueU* handle_IP(u_char *args,const struct pcap_pkthdr* pkthdr,const u_char* packet, OmlValueU* value)
{
	const IP_Header* ip;
	const UDP_Header* udp;
	const char* payload = NULL;
	u_int length = pkthdr->len;
	u_int hlen,off,version;
	long lengthOML;
	int i;
	// = (OmlValueU*) malloc(4*sizeof(OmlValueU));
	int len;
	int j = 0;

	/* jump pass the ethernet header */
	ip = (IP_Header*)(packet + sizeof(struct ether_header));
	length -= sizeof(struct ether_header); 

	/* check to see we have a packet of valid length */
	if (length < sizeof(IP_Header))
	{
		printf("truncated ip %d",length);
		return NULL;
	}

	len     = ntohs(ip->ip_len);
	hlen    = IP_HL(ip); /* header length */
	version = IP_V(ip);/* ip version */

	/* check version */
	if(version != 4)
	{
		fprintf(stdout,"Unknown version %d\n",version);
		return NULL;
	}

	/* check header length */
	if(hlen < 5 )
	{
		fprintf(stdout,"bad-hlen %d \n",hlen);
	}

	/* see if we have as much packet as we should */
	if(length < len)
		printf("\ntruncated IP - %d bytes missing\n",len - length);

	/* Check to see if we have the first fragment */
	off = ntohs(ip->ip_off);
	if((off & 0x1fff) == 0 )/* aka no 1's in first 13 bits */
	{

		value[1].stringPtrValue = inet_ntoa(ip->ip_src);
		value[1].stringPtrValue = strcpy(malloc(strlen(value[1].stringPtrValue)+1),value[1].stringPtrValue);
		value[2].stringPtrValue = inet_ntoa(ip->ip_dst);
		value[2].stringPtrValue = strcpy(malloc(strlen(value[2].stringPtrValue)+1),value[2].stringPtrValue);

		lengthOML = (long) len;
		value[3].longValue = lengthOML;


	}
	if(strcmp(pcap_mp->name,"default") == 0){
		;

	}else{
		udp = (UDP_Header*)(packet + sizeof(struct ether_header) + sizeof(IP_Header));
		payload = (const char*)(packet + sizeof(struct ether_header) + sizeof(IP_Header) + sizeof(UDP_Header) + 4);
		value[4].longValue = (long) atol(payload);

	}

	return value;
}

/* handle ethernet packets, much of this code gleaned from
 * print-ether.c from tcpdump source
 */
u_int16_t handle_ethernet(u_char *args, const struct pcap_pkthdr* pkthdr, const u_char* packet,  OmlValueU* value)
{
	char* eth_dst = (char*) malloc(18*sizeof(char));
	char* eth_src = (char*) malloc(18*sizeof(char));
	u_int caplen = pkthdr->caplen;
	u_int length = pkthdr->len;
	struct ether_header *eptr;  /* net/ethernet.h */
	u_short ether_type;

	if (caplen < ETHER_HDRLEN)
	{
		fprintf(stdout,"Packet length less than ethernet header length\n");
		return -1;
	}

	/* lets start with the ether header... */
	eptr = (struct ether_header *) packet;
	ether_type = ntohs(eptr->ether_type);

	/* Lets print SOURCE DEST TYPE LENGTH */
	//fprintf(stdout,"ETH: ");
	//fprintf(stdout,"%s ",ether_ntoa((struct ether_addr*)eptr->ether_shost));
	//fprintf(stdout,"%s ",ether_ntoa((struct ether_addr*)eptr->ether_dhost));

	eth_src = (char*) ether_ntoa_r((struct ether_addr*)eptr->ether_shost, eth_src);
	value[0].stringPtrValue = eth_src;
	//value[0].stringPtrValue = (char*) strcpy( malloc( strlen(eth_src) + 1),eth_src);

	//eth_dst = (char*) ether_ntoa_r((struct ether_addr*)eptr->ether_dhost, eth_dst);
	//value[1].stringPtrValue =  eth_dst;
	//value[1].stringPtrValue = (char*) strcpy( malloc( strlen(eth_dst) + 1), eth_dst);
	//  value[1].stringPtrValue = (char*) ether_ntoa_r((struct ether_addr*)eptr->ether_dhost, malloc(18));
	//  value[1].stringPtrValue = (char*) strcpy((char*) malloc(strlen(value[1].stringPtrValue)+1),value[1].stringPtrValue);



	//printf("ethernet address: %d %d \n ",  &value[0].stringPtrValue, &value[1].stringPtrValue);
	/* check to see if we have an ip packet */
	if (ether_type == ETHERTYPE_IP)
	{
		//fprintf(stdout,"(IP)");
	}else  if (ether_type == ETHERTYPE_ARP)
	{
		//fprintf(stdout,"(ARP)");
	}else  if (eptr->ether_type == ETHERTYPE_REVARP)
	{
		//fprintf(stdout,"(RARP)");
	}else {
		//fprintf(stdout,"(?)");
	}
	//fprintf(stdout," %d\n",length);

	return ether_type;
}

int
main(
		int argc,
		const char *argv[]
) {
	int* argc_;
	  const char** argv_;
	const char* name = argv[ 0 ];
	const char* p = name + strlen(argv[ 0 ]);
	while (! (p == name || *p == '/')) p--; 
	if (*p == '/') p++;
	name = p; 
	omlc_init(name, &argc, argv, o_log);

	argc_ =&argc;
	argv_ = argv;

	OmlPcap* pcap = NULL; 
	char* filter_pcap = NULL;
	char* dev_pcap = NULL;
	char src_pcap[50] = " ";
	char dst_pcap[50] = " ";
	int promisc = 1;
	int disconnected = 0;
	int i =0;

	for (i = *argc_; i > 0; i--, *argv_++) {
		if (strcmp(*argv_, "--pcap") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--oml-pcap'\n");
				return 0;
			}
			printf("Creation of pcap");
			pcap = create_pcap_measurement(*++argv_);
			*argc_ -= 2;
		} else if (strcmp(*argv_, "--pcap-ip-src") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--oml-pcap-ip-src'\n");
				return 0;
			}
			printf("Creation of pcap 2");
			strcat(src_pcap,  " src host ");
			strcat(src_pcap, *++argv_);      
			//strcat(src_pcap, *++argvPtr);
			*argc_ -= 2;
		}else if (strcmp(*argv_, "--pcap-ip-dst") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--oml-pcap-ip-dst'\n");
				return 0;
			}
			//printf("Creation of pcap %s \n", dst_pcap);
			strcat(dst_pcap,  " dst host ");
			strcat(dst_pcap, *++argv_);
			//dst_pcap =  *++argvPtr;
			*argc_ -= 2;
		}else if (strcmp(*argv_, "--pcap-if") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--oml-pcap-if'\n");
				return 0;
			}
			printf("Creation of pcap 3");
			dev_pcap = (char*)*++argv_;
			*argc_ -= 2;
		}else if (strcmp(*argv_, "--pcap-promiscuous") == 0) {
			if (--i <= 0) {
				o_log(O_LOG_ERROR, "Missing argument for '--oml-pcap-promiscuous'\n");
				return 0;
			}
			printf("Creation of pcap 4");
			promisc = atoi(*++argv_);
			*argc_ -= 2;
		}
			printf("Creation of pcap %s\n ", *argv_);

	}
	
	pcap_mp = pcap;
	
	if(pcap_mp != NULL){
		if(strcmp(src_pcap, " ") != 0)
			strcat(pcap_mp->filter_exp, src_pcap);
		if(strcmp(dst_pcap, " ") != 0){
			if(strcmp(src_pcap, " ") != 0){
				strcat(pcap_mp->filter_exp, " and "); 
				strcat(pcap_mp->filter_exp, dst_pcap);
			}else
				strcat(pcap_mp->filter_exp, dst_pcap);
		}

		strcat(pcap_mp->filter_exp, " and not ether proto \\arp");
		//printf(" filter %s \n", omlc_instance->pcap_mp->filter_exp);
		//omlc_instance->pcap_mp->filter_exp = "dst host 10.211.55.4"; 
		pcap_mp->dev = dev_pcap;
		pcap_mp->promiscuous = promisc;
		
		preparation_pcap(pcap_mp);
		pcap_engine_start(pcap_mp);
		
		omlc_start();

		
		while(1)
			;
	}else{
		printf("exit");
		exit(0);
	}
	exit(0);	
}
