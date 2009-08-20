
#include <iostream>
using namespace std;

#include <string.h>
#include <stdlib.h>

#include "cbr_generator.h"
//#include "cbr_generator_helper.h"


/**
 * Constructor
 */
CBR_Generator::CBR_Generator()

{
  pktSize_ = 512;
  pktInterval_ = 1;
  pktRate_ = 8 * pktSize_; // 1 packet/sec
  
}

void
CBR_Generator::defOpts()

{
  defOpt("size", POPT_ARG_INT, &pktSize_, "Size of packet", "bytes");
  defOpt("interval", POPT_ARG_FLOAT, &pktInterval_, "Internval between consecutive packets", "msec");
  defOpt("rate", POPT_ARG_FLOAT, &pktRate_, "Data rate of the flow", "kbps");
}


/**
 * Initialize generator. Should be called after all initial properties are set.
 */
void CBR_Generator::init()
{
  lastPktStamp_ = 0;
  if (pktRate_ == 0) throw "Rate cannot be set to zero!";
  pktInterval_ = (8.0 * pktSize_) / pktRate_;
}

/**
 * key function of the traffic generator
 * Determine the parameters of next packet, set size and timestamp.
 * @param p: the packet structure pointer to carry the calclulated parameter values
 */
Packet* 
CBR_Generator::nextPacket(Packet* p)

{
  p->setPayloadSize(pktSize_);
  lastPktStamp_ += pktInterval_;
  p->setTxTime(lastPktStamp_);
  return p;
}
