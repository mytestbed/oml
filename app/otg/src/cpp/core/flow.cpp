//#include "flow.h"
//#include <iostream>
//using namespace std;
/*
 * Default constructor of Flow Class
 * Constructor . The creation of a receiving flow.
 * @param flowid: If TCP the flowid is the fd for receiving stream
 */
 
/*
Flow::Flow(int flowid):flowid_(flowid)
{
  seqno_ = 0;
  prev_= NULL;
  next_= NULL;
  //addrId_ =  new Address(); 
  //packetcache_= new Packet(MAXBUFLENGTH);
  //sin_ = NULL;
}
*/
/**
 * Set address for the flow
 */
/*
void Flow::setAddress(Address* src)
{
   if ( src != NULL){
    addrId_->setHostname(src->getHostname());
    addrId_->setPort(src->getPort());
    addrId_->setHWAddr(src->getHWAddr());
   }
}
*/

/*
void Flow::inboundPacket()
{
 
  seqno_++;
  // first, set payload size as ( the default propeerty of packet)
  packetcache_->setPayloadSize(packetcache_->rxMeasure_->getReceivedLength() );
  // Then set all other rx,tx measurements
  packetcache_->txMeasure_->setSenderName(addrId_->getHostname());
  packetcache_->txMeasure_->setSenderMAC(addrId_->convertHWAddrToColonFormat());
  //  packetcache_->rxMeasure_->setRxTime((long)(flowclock_.getCurrentTime()*1e6) );
  packetcache_->txMeasure_->setSenderPort(addrId_->getPort());
  packetcache_->rxMeasure_->setFlowId(flowid_);
  packetcache_->rxMeasure_->setFlowSequenceNum(seqno_);

  sin_->handlePkt(packetcache_); 
}

*/



/**
 * Function to procses the packet, the most important thing is to recover the packet id and stream id f * from the payload and set them as propertoes of the packet
 */

