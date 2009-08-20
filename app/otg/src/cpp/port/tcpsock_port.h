
#ifndef OTG_TCP_SOCK_PORT_H
#define OTG_TCP_SOCK_PORT_H

#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <netinet/in.h>

#include "otg2/port.h"
#include "sockport.h"


class TCPSockPort: public InternetPort
{
public:
  TCPSockPort();
  
  void init();
  Packet* sendPacket(Packet* pkt);

protected:
  inline const char*
  getNamespace() { return "tcp"; }
  
  void
  initSocket();
};

#endif
