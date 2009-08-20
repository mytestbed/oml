#ifndef OTG2_PORT_H
#define OTG2_PORT_H

#include "otg2/component.h"
#include "otg2/sender.h"
#include "otg2/packet.h"
#include "otg2/source.h"

//#define MAX_STREAM_NUM_PER_PORT 16  // but we only allow 1

#define DEFAULT_SEND_PORT 3000
#define DEFAULT_RECV_PORT 4000



/** 
 * Port is an abstract class for the interface to send and receive packets.
 * Ports which receive (and therefore 'produce') packet, implement the
 * ISource interface. Ports which send packets implement the Sender 
 * interface.
 */

class Port

{
public:
  static Sender* createOutPort(char* name);
  static char* getDefOutPortName();
  static char* listOutPorts();
  
  static ISource* createInPort(char* name);  
  static char* getDefInPortName();
  static char* listInPorts();
};

#endif


