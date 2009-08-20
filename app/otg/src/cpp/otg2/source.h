#ifndef SOURCE_H_
#define SOURCE_H_

#include "otg2/packet.h"
#include "otg2/component.h"

class ISource

{
public:
  virtual ~ISource() {}
  
  /**
  * Returns the next packet from the source.
  * The (optionally) packet 'p' can be used to 
  * minimize packet creation. If not used, it
  * should be freed by this generator.
  */            
  virtual Packet*
  nextPacket(Packet* p) = 0;
 
  /**
  * Function to close whatever is behind this source. Any further
  * calls to +nextPacket+ is illegal.
  */
  virtual void 
  closeSource() = 0;
  
  virtual IComponent*
  getConfigurable() = 0;
  
};
#endif /*SOURCE_H_*/
