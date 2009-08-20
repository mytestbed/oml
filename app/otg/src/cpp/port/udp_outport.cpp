#include "udp_outport.h"
//#include "udpsock_port_helper.h"

//#include "stream.h"
#include <iostream>
using namespace std;
#include <netdb.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <ocomm/o_log.h>
#include <sys/time.h>
#include <time.h>

#ifdef WITH_OML
extern "C" {
#include <oml2/omlc.h>
}
static OmlMPDef oml_def[] = {
   {"ts", OML_DOUBLE_VALUE},
   {"flow_id", OML_LONG_VALUE},
   {"seq_no", OML_LONG_VALUE},
   {"pkt_length", OML_LONG_VALUE},
   {"dst_host",OML_STRING_VALUE},
   {"dst_port", OML_LONG_VALUE},
   {NULL, (OmlValueT)0},
};
static OmlMP* oml_mp = NULL;
#endif


UDPOutPort::UDPOutPort()

{
  
  //cout << "creation of udp_out"<< endl;
#ifdef WITH_OML
  //cout << "creation MP"<< endl;
  
  oml_mp = omlc_add_mp("udp_out",  oml_def);
#endif
}

void
UDPOutPort::defOpts()

{
  Socket::defOpts();
  
  defOpt("broadcast", POPT_ARG_INT, (void*)&bcastflag_, "Use UDP broadcast", "on|off");
  defOpt("nonblock", POPT_ARG_STRING, NULL, "Use Non-blocking UDP", "on|off");
}

/** Init Funciton to initialize a socket port.
 *  Init will do
 *  - create socket
 *  - bind socket
 * 
 *  Notes:
 * Bind to local address is one important task in init() 
 * Here source address of node itself (myaddr_) does not really be used by bind function of port.
 * The program use INADDR_ANY as the address filled in address parameters of bind().
 * So, we need an empty hostname with the port number.
 * 
 * Usually, port state should changed from uninitialized to running
 * but in case of port being paused from beginning, we keep it paused.
 *  
 * 
 */
void UDPOutPort::init()
{
    
  struct timeval time;
  int i = gettimeofday(&time, NULL);
  timestamp = time.tv_sec;

  
  if (sockfd_ != 0) return; // already initialized
  
  if (dsthost_ == NULL || strcmp(dsthost_, "") == 0) {
    throw "Missing destination host";
  }
  if (dstport_ <= 0) {
    throw "Missing dest_host port";
  }
  
  Socket::init();
  
  if (bcastflag_ == 1) { 
    if (setsockopt(sockfd_, SOL_SOCKET, SO_BROADCAST, &bcastflag_, sizeof(bcastflag_)) == -1) {
      perror("setsockopt");
      throw "Set broadcast option failed.";
    }
  }
  setSockAddress(dsthost_, dstport_, &dstSockAddress_);
}

void
UDPOutPort::initSocket()

{
  if ((sockfd_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
    throw "Error while opening UDP socket";
  }
}

/** The main send function of UDP Socket Port.
 * 
 * Stamp a stream id and sequence number. 
 * Then, use sendto()
 */
Packet* 
UDPOutPort::sendPacket(Packet* pkt)
{  
  struct timeval tv;
  gettimeofday(&tv, NULL);
  double now = tv.tv_sec - timestamp + 0.000001 * tv.tv_usec;
  
 // cout << timestamp << endl;
  pkt->stampPacket(0x01);
  pkt->stampShortVal(pkt->getFlowId());
  pkt->stampLongVal(pkt->getSequenceNum());
  
  struct sockaddr* dest = (struct sockaddr*)&dstSockAddress_;
  int  destLength = sizeof(struct sockaddr_in); 
  int pktLength = pkt->getPayloadSize();
   
  o_log(O_LOG_DEBUG2, "Sending UDP packet of size '%d' to '%s:%d'\n", pktLength, dsthost_, dstport_);
  int len = sendto(sockfd_, pkt->getPayload(), pktLength, 0, dest, destLength); 
  if (len == -1) { 
    perror("send");
    throw "Sending Error.";
  }
  
#ifdef WITH_OML
  OmlValueU v[6];
  v[0].doubleValue = now;
  v[1].longValue = pkt->getFlowId();
  v[2].longValue = pkt->getSequenceNum();
  v[3].longValue = pktLength;
  omlc_set_string(v[4], dsthost_);
  v[5].longValue = dstport_;
  omlc_inject(oml_mp, v);   
#endif   
  return pkt;
}

void
UDPOutPort::closeSender()

{
  Socket::close();
}
