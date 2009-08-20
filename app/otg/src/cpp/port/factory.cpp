

//#include "tcpsock_port.h"
#include "udp_outport.h"
#include "udp_inport.h"
#include "null_outport.h"

Sender*
Port::createOutPort(
  char* name
) {
  if (strcmp(name, "udp") == 0) {
    return new UDPOutPort();
  } else if (strcmp(name, "null") == 0) {
    return new NullOutPort();
  }
  return NULL;
}

char*
Port::listOutPorts()

{
  return "udp|null";
}

char*
Port::getDefOutPortName()

{
  return "udp";
}


ISource*
Port::createInPort(
  char* name
) {
  if (strcmp(name, "udp") == 0) {
    return new UDPInPort();
  }
  return NULL;
}

char*
Port::listInPorts()

{
  return "udp";
}

char*
Port::getDefInPortName()

{
  return "udp";
}
