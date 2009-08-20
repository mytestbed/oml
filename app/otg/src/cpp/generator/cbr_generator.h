 
#ifndef CBR_GENERATOR_H
#define CBR_GENERATOR_H

#include <popt.h>
#include "otg2/packet.h"
#include "otg2/generator.h"
#include "otg2/component.h"


/**CBR GENERATOR Module
 *  
 * CBR: CONSTANT BIT RATE Traffic
 * Fix packet interval
 * Fix packet size
 *
 */

class CBR_Generator: public Generator, public Component
{     
public:
  CBR_Generator();

  void 
  init(); 

  Packet*
  nextPacket(Packet* p);
  
  inline void 
  closeSource() {}

  inline IComponent*
  getConfigurable() {return this;}
  

private:
  inline const char*
  getNamespace() { return "cbr"; }
  
  void
  defOpts();
  

  int pktSize_;   ///< packet size variable, constant.
  float pktInterval_;  ///< interarrival time defined as the offset between two beginnings of consecutive packets
  float pktRate_;

  double lastPktStamp_; ///< used to record last packet generation time.
};


#endif
