

#define USE_OPTS
#include "omf_trace_popt.h"

#define OML_FROM_MAIN
#include "omf_trace_oml.h"

void
run(
  opts_t* opts,
  oml_mps_t* oml_mps
) {
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

  // Initialize measurment points
  oml_register_mps();  // defined in xxx_oml.h
  omlc_start();

  // Do some work
  run(g_opts, g_oml_mps);

  return(0);
}
