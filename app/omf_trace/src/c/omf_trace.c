
#include <libtrace.h>

#define USE_OPTS
#include "omf_trace_popt.h"

#define OML_FROM_MAIN
#include "omf_trace_oml.h"

static void 
per_packet(
  libtrace_packet_t* packet
) {
  double   last_ts;
  uint32_t remaining;
  void*    l3;
  uint16_t ethertype;
  void*    transport;
  uint8_t  proto;
  void*    payload;

  last_ts = trace_get_seconds(packet);
  
  l3 = trace_get_layer3(packet, &ethertype, &remaining);
  if (!l3) {
    /* Probable ARP or something */
    return;
  }

  /* Get the UDP/TCP/ICMP header from the IPv4/IPv6 packet */
  switch (ethertype) {
  case 0x0800:
    transport = trace_get_payload_from_ip((libtrace_ip_t*)l3,
					  &proto,
					  &remaining);
    if (!transport)
      return;
    break;
  case 0x86DD:
    transport = trace_get_payload_from_ip6((libtrace_ip6_t*)l3,
					   &proto,
					   &remaining);
    if (!transport)
      return;
    break;
  default:
    return;
  }

  /* Parse the udp/tcp/icmp payload */
  switch(proto) {
  case 1:
    // icmp;
    return;

  case 6:
    payload = trace_get_payload_from_tcp((libtrace_tcp_t*)transport,
					 &remaining);
    if (!payload)
      return;
    break;
  
  case 17:
    payload = trace_get_payload_from_udp((libtrace_udp_t*)transport,
					 &remaining);
    if (!payload)
      return;
    break;
	
  default:
    return;
  }
  printf("PPPPPP\n");
}

static int
run(
  opts_t* opts,
  oml_mps_t* oml_mps
) {
  libtrace_t* trace;
  libtrace_packet_t* packet;
  libtrace_filter_t* filter = NULL;


  trace = trace_create(opts->interface);
  if (trace_is_err(trace)) {
    trace_perror(trace,"Opening trace file");
    return 1;
  }

  if (opts->snaplen > 0) {
    if (trace_config(trace, TRACE_OPTION_SNAPLEN, &opts->snaplen)) {
      trace_perror(trace, "ignoring: ");
    }
  }
   
  if (opts->filter) {
    filter = trace_create_filter(opts->filter);

    if (trace_config(trace, TRACE_OPTION_FILTER, &filter)) {
      trace_perror(trace, "ignoring: ");
    }
  }
  
  if (opts->promisc) {
    if (trace_config(trace, TRACE_OPTION_PROMISC, &opts->promisc)) {
      trace_perror(trace, "ignoring: ");
    }
  }

  if (trace_start(trace)) {
    trace_perror(trace, "Starting trace");
    trace_destroy(trace);
    return 1;
  }

  packet = trace_create_packet();
  while (trace_read_packet(trace, packet) > 0) {
    per_packet(packet);
  }

  trace_destroy_packet(packet);
  if (trace_is_err(trace)) {
    trace_perror(trace, "Reading packets");
  }
  trace_destroy(trace);



  /*
  long val = 1;
  do {
    OmlValueU v[3];

    omlc_set_long(v[0], val);
    omlc_set_double(v[1], 1.0 / val);
    omlc_set_const_string(v[2], "foo");
    omlc_inject(oml_mps->sensor, v);

    val += 2;
    if (opts->loop) sleep(opts->delay);
  } while (opts->loop);
  */
}

int
main(
  int argc,
  const char *argv[]
) {
  omlc_init(argv[0], &argc, argv, NULL); 

  // parsing command line arguments
  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  int c;
  while ((c = poptGetNextOpt(optCon)) > 0) {}

  if (g_opts->interface == NULL) {
    fprintf(stderr, "Missing interface\n");
    return 1;
  }

  // Initialize measurment points
  oml_register_mps();  // defined in xxx_oml.h
  omlc_start();

  // Do some work
  run(g_opts, g_oml_mps);

  return(0);
}
