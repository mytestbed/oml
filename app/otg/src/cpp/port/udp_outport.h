
#ifndef OTG_UDP_SOCK_PORT_H
#define OTG_UDP_SOCK_PORT_H



#include "otg2/port.h"
#include "otg2/packet.h"
#include "socket.h"


class UDPOutPort: public Sender, public Socket

{
public:
  UDPOutPort();
 
  void init();
  Packet* sendPacket(Packet *pkt);
  
  void
  closeSender();
  
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

  int bcastflag_;
  struct sockaddr_in dstSockAddress_; ///< for every outgoing packet of a connection, the dstaddress.
  
  
};

#endif
