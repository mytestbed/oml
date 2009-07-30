 

/**
 * ORBIT OML Client API Header file.  This is an automatically generated file
 * and has been customised for your application. Please DO-NOT edit this file.
 **/

#ifndef CLIENTAPI_H
#define CLIENTAPI_H

#include <oml2/omlc.h>

extern OmlMP* iperf_measure;

static OmlMPDef oml_receiverport[] = {
  {"flow_no", OML_LONG_VALUE},
  {"throughput", OML_DOUBLE_VALUE},
  {"jitter", OML_DOUBLE_VALUE},
  {"packet_loss", OML_DOUBLE_VALUE},
  {NULL, (OmlValueT)0}
};

static void
oml_register_mps()
{
    iperf_measure = omlc_add_mp("iperf_recived", oml_receiverport);
    
}


#endif
