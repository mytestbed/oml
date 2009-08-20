
#ifndef OTG_NULL_OUT_PORT_H
#define OTG_NULL_OUT_PORT_H



#include "otg2/sender.h"
#include "otg2/packet.h"
#include "otg2/component.h"


class NullOutPort: public Sender, public Component

{
public:
  NullOutPort();
 
  void init();
  Packet* sendPacket(Packet *pkt);
  
  void
  closeSender() {}
  
  inline IComponent*
  getConfigurable() {return this;}
  
  
protected:
  inline const char*
  getNamespace() { return "null"; }
  
  void
  defOpts();
  
};

#endif
