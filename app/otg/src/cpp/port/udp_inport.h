
#ifndef OTG_UDP_INPORT_H
#define OTG_UDP_INPORT_H



#include "otg2/source.h"
#include "otg2/packet.h"
#include "socket.h"

class UDPInPort: public ISource, public Socket

{
public:
  UDPInPort();
 
  void init();

  /**
  * Returns the next packet from the source.
  * The (optionally) packet 'p' can be used to 
  * minimize packet creation. If not used, it
  * should be freed by this generator.
  */            
  Packet*
  nextPacket(Packet* p);
  
  void
  closeSource();

  inline IComponent*
  getConfigurable() {return this;}
  
  time_t timestamp;
  
protected:
  inline const char*
  getNamespace() { return "udp"; }
  
  void
  defOpts();
  
  void
  initSocket();

  int maxPktLength_;

};

#endif
