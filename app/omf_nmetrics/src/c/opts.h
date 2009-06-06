
#ifndef NMETRICS_OPTS_H

typedef struct {
  char* if_name;
  int   report_cpu; 
  int   report_memory; 
  int   sample_interval;
} opts_t;

#ifndef USE_OPTS

opts_t* g_opts;

#else

static opts_t g_opts_storage = {'\0', 0, 0, 1};
opts_t* g_opts = &g_opts_storage;

// only the file containing the main() function should come through here

#include <popt.h>

struct poptOption options[] = {
  POPT_AUTOHELP
  { "cpu", 'c', POPT_ARG_NONE, &g_opts_storage.report_cpu, 0, 
    "Report memory usage"},
  { "interface", 'i', POPT_ARG_STRING, &g_opts_storage.if_name, 'i', 
    "Report usage of specified network device (can be used multiple times)",
    "ifName" },
  { "memory", 'm', POPT_ARG_NONE, &g_opts_storage.report_memory, 0, 
    "Report memory usage"},
  { "sample-interval", 's', POPT_ARG_INT, &g_opts_storage.sample_interval, 0, 
    "Time between consecutive measurements [sec]", "seconds"},
  { NULL, 0, 0, NULL, 0 }
};

#endif

#define NMETRICS_OPTS_H
#endif
