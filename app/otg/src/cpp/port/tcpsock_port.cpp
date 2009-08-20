#include "tcpsock_port.h"
#include "tcpsock_port_helper.h"
//#include "stream.h"
#include <iostream>
using namespace std;
#include <netdb.h>
#include <arpa/inet.h>

#define BACKLOG   10  ///< Maximum 10 connections for a TCP server

TCPSockPort::TCPSockPort()

{
}

/** Init Funciton to initialize a TCP socket port
 *

 * The port bind to its own address, generate a TCP socket.
 * This TCP socket will be used by sender to send packets, and for receiver to "listen".
 * Packet reception will use new socket file desciptor.
 *
 * Bind to local address is one important task in init() 
 * Here source address of node itself (myaddr_) does not really be used by bind function of port.
 * The program use INADDR_ANY as the address filled in address parameters of bind().
 * So, we need an empty hostname with the port number.
 * 
 * Another important thing init() does is: for TCP sender port, the init is going to connect to the other end.
 *
 * Usually, port state should changed from uninitialized to running
 * but in case of port being paused from beginning, we keep it paused. 
 */

void TCPSockPort::init()

{
  if (sockfd_ != 0) return; // already initialized

  InternetPort::init();

  struct sockaddr_in addr;
  setSockAddress(desthost_, destport_, &addr);
  if (connect(sockfd_, (struct sockaddr*)&addr, sizeof(struct sockaddr_in)) < 0) {
    perror("connect");
    throw "Connect to TCP server failed";
  }
  cout << "succeed to connect a TCP server..." << endl;
}

void
TCPSockPort::initSocket()

{
  if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    throw "Error while opening TCP socket";
  }
}



/** The main send function of TCP Socket Port.
 * Be sure the sockfd_ is already connected to a remote TCP server
 * First, check if the port is still in "running" state.
 * Then, simply call send() to send packet 
 */
void TCPSockPort::sendPacket(Packet *pkt)
{  
  //if (state_ == paused) return;
  int len = send(sockfd_, pkt->getPayload(), pkt->getPayloadSize(), 0);                    
  if (len == -1) perror("send");
  //pkt_->txMeasure_->setTxTime((long)(portclock_.getCurrentTime()*1e6) );
  //otgMeasureReport();
}



//const struct poptOption* 
//TCPSockPort::getOptions()
//
//{
//  char* prefix;
//  struct poptOption* opts;
//  opts = (struct poptOption*)calloc(6, sizeof(struct poptOption));
//  struct poptOption* p = opts;
//
//  popt_set(p++, prefix, "hostname", '\0', POPT_ARG_STRING, (void*)localhost, 0, 
//	   "Name of local host", "[name]");
//  popt_set(p++, prefix, "port", '\0', POPT_ARG_INT, (void*)&localport, 0,
//	   "Local port to bind to", "[int]");
//  popt_set(p++, prefix, "dsthostname", '\0', POPT_ARG_STRING, (void*)dsthost, 0, 
//	   "Name of destination host", "[name]");
//  popt_set(p++, prefix, "dstport", '\0', POPT_ARG_INT, (void*)&dstport, 0,
//	   "Destination port to send to", "[int]");
//  popt_set(p);
//
//  return opts;
//}
