#ifndef STREAM_H
#define STREAM_H

#include "otg2/unixtime.h"
#include "otg2/source.h"
#include "otg2/packet.h"
#include "otg2/sender.h"
#include <otg2/component.h>


class Stream : public Component

{
public:
  Stream(short id = 1);
  
  void setSource(ISource* source) { source_ = source; }
  void setSender(Sender* sender) { sender_ = sender; }
  void run();
  
  void startStream();
  void pauseStream();
  void resumeStream();
  void exitStream();
  
  void init() {}
  
  // should only be used internally
  void _run();
  
protected:
  inline const char*
  getNamespace() { return "flow"; }
  
  void
  defOpts();
  
  unsigned long seqno_; ///< number of packets sent or received  by this stream 
  ISource*       source_; ///< the generator to feed packets to this stream
  Sender*       sender_; /// the port for sending packets out.
  short         streamid_; /// stream identifier. 
  
  //double starttime_;  ///< starting time of a stream
  UnixTime streamclock_; ///<Contorl the timing of stream

  bool paused_; ///< indicate whether stream is paused
  
  int active_;  // set to true while stream is active
  pthread_t thread_;
};


  
#endif
