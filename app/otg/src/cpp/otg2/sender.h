#ifndef SENDER_H_
#define SENDER_H_

#include <otg2/packet.h>
#include <otg2/component.h>

/** 
 * Sender is an interface for any componet which can send 
 * a packet fed through a stream.
  */
class Sender
{
public:
  virtual ~Sender() {};  

  /**
   * Function to send a packet.
   * The default socket file descriptor will always be used for send()
   * No matter how many streams will be served by this port, only a sockfd used by the port.
   * 
   * The function optionally returns a packet which can be reused.
   */
  virtual Packet*
  sendPacket(Packet *pkt) = 0;
  
  /**
   * Function to close whatever is behind this source. Any further
   * calls to +sendPacket+ is illegal.
   */
  virtual void 
  closeSender() = 0;
  
  virtual IComponent*
  getConfigurable() = 0;
  
  
};

#endif /*SENDER_H_*/
