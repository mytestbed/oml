 
#ifndef EXPO_GENERATOR_H
#define EXPO_GENERATOR_H

#include <popt.h>
#include "otg2/packet.h"
#include "otg2/generator.h"
#include "randomvariable.h"

/**Exponential GENERATOR Module
 *  
 * Exponential: Exponential BIT RATE Traffic
 * Specify mean burst time
 * Specify mean idle time
 * Rate
 * Fixed packet size
 *
 */
class Expo_Generator: public Generator, public Component
{     
public:
  Expo_Generator(int size=512, double rate=4096.0, double ontime=1.0, double offtime=1.0);

  void init();
  Packet* nextPacket(Packet* p);  
  
  inline void 
  closeSource() {}
  
  inline IComponent*
  getConfigurable() {return this;}  

protected:
  inline const char*
  getNamespace() { return "exp"; }
  
  void
  defOpts();
  
private:
  int pktSize_;   ///< packet size variable, constant.
  double rate_;  ///< send rate during on time (bps) bits per second 
  double ontime_ ; ///<average length of the burst (sec), burst time
  double offtime_ ; ///< Exp random var: average length of idle time (sec)
  ExponentialRandomVariable offtimeVar_ ; ///< Exp random var: average length of idle time (sec)
  double pktInterval_;  ///< interarrival time defined as the offset between two beginnings of consecutive packets during burst
  double lastPktStamp_; ///< used to record last packet generation time.
  ExponentialRandomVariable burstLength_; ///< Exp Random variable: Average number of packets during burst period
  unsigned int rem_;  ///<number of remaining packets in a burst, tracking the deduction of burstLength
};


#endif
