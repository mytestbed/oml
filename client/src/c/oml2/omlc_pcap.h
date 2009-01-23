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
/**
 * C++ Interface: omlc_pcap
 *
 * Description:
 * \file   omlc_pcap.h
 * \brief  the lib pcap for OML
 * \author Guillaume Jourjon <guillaume.jourjon@nicta.com.au>, (C) 2008
 *
 * Copyright: See COPYING file that comes with this distribution
 *
 */
#ifndef OMLC_PCAP_H_
#define OMLC_PCAP_H_

#include <pcap.h>
#include <stdio.h>
#include <malloc.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/if_ether.h>
#include <net/ethernet.h>
#include <netinet/ether.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <pthread.h>
#include <oml2/omlc.h>
typedef u_int32_t tcp_seq;

#ifndef ETHER_HDRLEN
#define ETHER_HDRLEN 14
#endif

/**
 * Structure of an internet header, naked of options.
 *
 * Stolen from tcpdump source (thanks tcpdump people)
 *
 * We declare ip_len and ip_off to be short, rather than u_short
 * pragmatically since otherwise unsigned comparisons can result
 * against negative integers quite easily, and fail in subtle ways.
 */
typedef struct _ip_Header {
  u_char	ip_vhl;/* header length, version */
#define IP_V(ip)	(((ip)->ip_vhl) >> 4)
#define IP_HL(ip)	((ip)->ip_vhl & 0x0f)
  u_char	ip_tos;		/* type of service */
  u_short	ip_len;		/* total length */
  u_short	ip_id;		/* identification */
  u_short	ip_off;		/* fragment offset field */
#define IP_RF 0x8000
#define	IP_DF 0x4000            /* dont fragment flag */
#define	IP_MF 0x2000            /* more fragments flag */
#define	IP_OFFMASK 0x1fff       /* mask for fragmenting bits */
  u_char	ip_ttl;		/* time to live */
  u_char	ip_p;            /* protocol */
  u_short	ip_sum;          /* checksum */
  struct	in_addr ip_src;
  struct	in_addr ip_dst;	/* source and dest address */
}IP_Header;


typedef struct _tcp_Header{
  u_short th_sport;  /* source port */
  u_short th_dport;  /* dest port */
  tcp_seq th_seq;    /* sequence number */
  tcp_seq th_ack;    /* acknoledgement sequence */
  u_char th_offx2;   /* data offset */
#define TH_OFF(th) (((th)->th_offx2 & 0xf0) >> 4)
#define TH_FIN 0x01
#define TH_SYN 0x02
#define TH_RST 0x04
#define TH_PUSH 0x08
#define TH_ACK 0x10
#define TH_URG 0x20
#define TH_ECE 0x40
#define TH_CWR 0x80
#define TH_FLQGS (TH_FIN|TH_SYN|TH_RST|TH_ACK|TH_URG|TH_ECE|TH_CWR)
  u_short th_win;    /* window */
  u_short th_sum;    /* checksum */
  u_short th_urp;    /* urgent pointer*/
}TCP_Header;

typedef struct _udp_Header{
  u_short th_sport;  /* source port */
  u_short th_dport;  /* dest port */
  u_short th_len;    /* length */
  u_short th_sum;    /* checksum */
}UDP_Header;

typedef struct _omlPcap{
  char* name;
  IP_Header header_ip;
  TCP_Header header_tcp;
  int promiscuous;
  OmlMP* mp;
  OmlMPDef* def;
  char filter_exp[250];
  char *dev;
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_t* descr;
  struct bpf_program fp;      /* hold compiled program     */
  bpf_u_int32 maskp;          /* subnet mask               */
  bpf_u_int32 netp;           /* ip                        */
  pthread_t thread_pcap;
} OmlPcap;

void packet_treatment(
    u_char* useless,
    const struct pcap_pkthdr* pkthdr,
    const u_char* pkt
);

OmlPcap* create_pcap_measurement(
    char* name
);

OmlMPDef* create_pcap_filter(
    char* file
                            );

OmlValueU* handle_IP(
    u_char *args,
    const struct pcap_pkthdr* pkthdr,
    const u_char* packet,
    OmlValueU* value
                 );

void preparation_pcap(
    OmlPcap* pcap
                     );

u_int16_t handle_ethernet(
    u_char *args,
    const struct pcap_pkthdr* pkthdr,
    const u_char* packet,
    OmlValueU* value
                         );

static void* thread_pcapstart(void* handle);

void
    pcap_engine_start(
    OmlPcap* pcap
                     );

#endif /* OMLC_PCAP_H_ */

/*
 Local Variables:
 mode: C
 tab-width: 4
 indent-tabs-mode: nil
*/
