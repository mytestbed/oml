#include "otg2/stream.h"
#include <iostream>
using namespace std;

#define MAXBUFLENGTH 10000

static void*
run_stream(void* ptr);


/**
 */
Stream::Stream(
  short id
)
{ 
  streamid_ =  id;
  seqno_ = 0;
//  packetcache_= new Packet();
  source_ = NULL;
  sender_ = NULL;
//  prev_ = NULL;
//  next_ = NULL;
//  app_  = app;
  paused_ = false;
  active_ = 1;
  streamclock_ = -1;
  
}

void
Stream::defOpts()

{
  defOpt("id", POPT_ARG_INT, (void*)&streamid_, "ID of flow");
  
}

void
Stream::run()

{
  if (source_ == NULL || sender_ == NULL) throw "Stream not fully defined";
  
  if (pthread_create(&thread_, NULL, run_stream, (void*)this)) {   
    throw "Create a Stream Thread Failed...";
  }
}
void 
Stream::pauseStream()

{
  paused_ = true;
  streamclock_.pauseClock();
}

void 
Stream::resumeStream()

{
  paused_ = false;
  streamclock_.resumeClock();
}

void 
Stream::exitStream()

{
  active_ = 0;
}


void 
Stream::_run()

{
  Packet* p = NULL;
  while (active_) {
    if (p == NULL) {
      p = new Packet(Packet::DEFAULT_PAYLOAD_SIZE, &streamclock_);
    }
    p->reset();
    if ((p = source_->nextPacket(p)) != NULL) {
      if (p->getFlowId() < 0) p->setFlowId(streamid_);
      if (p->getSequenceNum() == 0) p->setSequenceNum(++seqno_);
      
      if (p->getTxTime() > 0) { 
        streamclock_.waitAt(p->getTxTime());
      }

      if (p->getTimeStamp() <= 0) {
        p->setTimeStamp(streamclock_.getAbsoluteTime() * 1e3); 
      }
      p = sender_->sendPacket(p);
    }
  }
  if (p != NULL) {
    delete p;
  }
  sender_->closeSender();
//  app_->close(); //close OML reporting
  
}

/**
 * Function to run the stream in a new thread
 */
static void*
run_stream(void* ptr)

{
  Stream* s = (Stream *)ptr;
  s->_run();  
  return NULL;
}


