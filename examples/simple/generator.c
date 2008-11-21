
/*
 * This programs implements a simple sin-wave generator with
 * the output measured by OML.
 */

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <popt.h>
#include <oml2/omlc.h>


static float amplitude = 1.0;
static float frequency = 0.1;
static float sample_interval = 1;
static float samples = -1;

struct poptOption options[] = {
  POPT_AUTOHELP
  { "amplitude", 'b', POPT_ARG_FLOAT, &amplitude, 0, 
        "Amplitude of produce signal"},
  { "frequency", 'd', POPT_ARG_FLOAT, &frequency, 0, 
        "Frequency of wave generated [Hz]"  },
  { "samples", 'n', POPT_ARG_INT, &samples, 0, 
        "Number of samples to take. -1 ... forever"},
  { "sample-interval", 's', POPT_ARG_FLOAT, &sample_interval, 0, 
        "Time between consecutive measurements [sec]"},
  { NULL, 0, 0, NULL, 0 }
};

static OmlMPDef d_lin[] = {
    {"label", OML_STRING_PTR_VALUE},
    {"seq_no", OML_LONG_VALUE},
    {NULL, 0}
};
static OmlMP* m_lin;

static OmlMPDef d_sin[] = {
    {"label", OML_STRING_PTR_VALUE},
    {"phase", OML_DOUBLE_VALUE},
    {"value", OML_DOUBLE_VALUE},
    {NULL, 0}
};
static OmlMP* m_sin;

void
run()

{
  float angle = 0;
  float delta = frequency * sample_interval * 2 * M_PI;
  unsigned long sleep = (unsigned long)(sample_interval * 1E6);

  // this loop should never end if samples = -1
  int i = samples;
  int count = 1;
  for (; i != 0; i--, count++) {
    char label[64];
    sprintf(label, "sample-%d", count);
    {
      // "lin" measurement point
      OmlValueU v[2];
      v[0].stringPtrValue = label;
      v[1].longValue = count;
      omlc_process(m_lin, v);
    }

    float value = amplitude * sin(angle);
    {
      // "sin" measurement point
      OmlValueU v[3];
      v[0].stringPtrValue = label;
      v[1].doubleValue = angle;
      v[2].doubleValue = value;
      omlc_process(m_sin, v);
    }

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
  char c;

  // registering OML measurement points
  omlc_init("generator", &argc, argv, NULL); 
  m_lin = omlc_add_mp("lin", d_lin);
  m_sin = omlc_add_mp("sin", d_sin);
  omlc_start();

  // parsing command line arguments
  poptContext optCon = poptGetContext(NULL, argc, argv, options, 0);
  while ((c = poptGetNextOpt(optCon)) >= 0);

  run();
  return(0);
}
