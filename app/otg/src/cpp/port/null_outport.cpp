#include "null_outport.h"
#include <ocomm/o_log.h>

NullOutPort::NullOutPort()

{}

void
NullOutPort::defOpts()

{
}


void NullOutPort::init()
{
  // nothing to do
}

Packet* 
NullOutPort::sendPacket(Packet* pkt)
{  
   int pktLength = pkt->getPayloadSize();
   o_log(O_LOG_DEBUG2, "Consuming  packet of size '%d'\n", pktLength);
   return pkt;
}

