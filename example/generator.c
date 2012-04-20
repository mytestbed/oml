#include <stdio.h>
#include <math.h>
#include <unistd.h>

#define USE_OPTS
#include "generator_popt.h"

#define OML_FROM_MAIN
#include "generator_oml.h"

#include "version.h"

void
run(
  opts_t* opts,
  oml_mps_t* oml_mps
) {
  double angle = 0;
  double delta = opts->frequency * opts->sample_interval * 2 * M_PI;
  unsigned long sleep = (unsigned long)(opts->sample_interval * 1E6);

  printf("%f, %f, %f\n", M_PI,  delta, sleep);

  int i = opts->samples;
  unsigned int count = 1;
  for (; i != 0; i--, count++) {
    char label[64];
    sprintf(label, "sample-%d", count);

    oml_inject_d_lin(oml_mps->d_lin, label, count);

    double value = opts->amplitude * sin(angle);
    oml_inject_d_sin(oml_mps->d_sin, label, angle, value);

    printf("%s %d | %f %f\n", label, count, angle, value);

    angle = fmodf(angle + delta, 2 * M_PI);
    usleep(sleep);
  }
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
