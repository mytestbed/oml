#include "otg2/gate.h"
#include <string.h>

#include <iostream>
using namespace std;

/**
 * Constructor.
 * 
 */

Gate::Gate():gateclock_(-1)
{
  rlhead_ = rltail_ = rlcurr_ = NULL;
  flownum_ = 0;
  pkt_= new Packet(MAXBUFLENGTH);
  sin_ = NULL; // A  sink will created in application class...
  //sin_ =  new DummySink(app_);

}

void Gate::configGate(OrbitApp* app, Sink* sin, int clockref)
{
  app_ = app;
  sin_ = sin;
  gateclock_.setAbsoluteOrigin(clockref);
}

/**
 * Adding a default new flow to the receiver gate.
 * Always add flow to tail of the list
 * And if the "rlcurr_" pointer is NULL, set rlcurr_ to this newly added stream
 * @param recvfd the socket file descriptor binded to this flow
 * 
 */

Flow* Gate::createFlow(int recvfd)
{
 
  Flow* s =  new Flow(recvfd);
  if (!rltail_) {
    rlhead_= rltail_= s;
  } else {
    s->prev_ = rltail_;
    rltail_->next_= s;
    rltail_= s;
  }
  rltail_->next_= NULL;
  if (rlcurr_ == NULL ) rlcurr_ = rltail_;   
  return s;
}

/*
Flow* Gate::createFlow(int recvfd, Sink *sin)
{
  
  Flow* s =  createFlow(recvfd);
  s->bindSink(sin);  
  return s;
}
*/


/**
 *Add a flow at tail 
 */

Flow* Gate::addFlow(int flowid, Address *src)
{
  Flow *s=createFlow(flowid);
  s->setAddress(src);
  if (app_!= NULL )app_->omlDemultiplexReport(flowid, src);
  return s;  
}



/**
 *  A function to delete the stream from stream list
 *  Coding needs to consider three situations:
 *  - list head
 *  - list tail
 *  - other
 */

bool Gate::deleteFlow(Flow* strm)
{
   if (!strm) return false;
   if ( strm->prev_ !=NULL) 
     strm->prev_->next_ = strm->next_;
   if ( strm->next_ !=NULL) 
   	 strm->next_->prev_ = strm->prev_;
   return true;  
}


Flow *Gate::searchFlowbyFd (int fd)
{
  Flow *s = rlhead_;
  while ( s !=NULL)
  	{
  	  if  (s->flowid_ == fd ) return s;
  	  s = s->next_;
  	}
  return NULL;
}

Flow *Gate::searchFlowbyAddress (Address *addr)
{
  Flow *s = rlhead_;
  while ( s !=NULL)
    {  //search 
       if ( strcmp(s->getAddr()->getHostname(),"UseMACAddr")== 0)
        {
          if (s->getAddr()->isSameMACAddr(addr)== true) return s;   
        }
        else{
         //Matching IP address and Port number
  	  if  (strcmp(s->getAddr()->getHostname(), addr->getHostname() )== 0
               && s->getAddr()->getPort() == addr->getPort() )
                return s;
  	} 
        s = s->next_;
    }
  return NULL;
}



/** get the number of incoming flows in the list
 *  count from head to tail
 *
 */
int Gate::getFlowNum()
{
	int cnt = 0;
	Flow *s = rlhead_;
	while ( s !=NULL )
	{
		cnt++;
		s = s->next_;
	}
	return cnt;
}

/**
 * Post-processing based on flow information
 */
void Gate::inboundPacket()
{
  rlcurr_->incSeq();
  // first, set payload size as ( the default property of packet)
  pkt_->setPayloadSize(pkt_->rxMeasure_->getReceivedLength() );
  // second, set rxtimestamp in milliseconds
  pkt_->rxMeasure_->setRxTime(gateclock_.getAbsoluteTime()*1e3);//change to milliseconds
  // Then set all other rx,tx measurements
  pkt_->txMeasure_->setSenderName(rlcurr_->getAddr()->getHostname());
  pkt_->txMeasure_->setSenderMAC(rlcurr_->getAddr()->convertHWAddrToColonFormat());
  pkt_->txMeasure_->setSenderPort(rlcurr_->getAddr()->getPort());
  pkt_->rxMeasure_->setFlowId(rlcurr_->getID());
  pkt_->rxMeasure_->setFlowSequenceNum(rlcurr_->getSeqno());
  // oml report only occurs when an OML Application is bind to this gate
  if(app_ != NULL ) app_->omlReceiverReport(pkt_);
  // if sink is connected, pass packet to sink
  if(sin_ != NULL)  sin_->handlePkt(pkt_); 
}


 /** Fucntuon to de-multiplexing the receiving packets accroding to their sender address
   * This function will be used for UDP and RAW socket gates 
   *
   */

 Flow*  Gate::demultiplex(Address* addr)
 {
    
    Flow* s  = searchFlowbyAddress(addr);
    if ( s == NULL) //not found any mataching flow
    {          
      s = addFlow(flownum_,addr);    
      flownum_++;
    }   
    return s;
 }

