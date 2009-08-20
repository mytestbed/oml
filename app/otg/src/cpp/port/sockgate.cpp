#include "sockgate.h"

#include "stream.h"
#include <iostream>
using namespace std;
#include <netdb.h>
#include <arpa/inet.h>


SockGate::SockGate(): Gate(), sockfd_(0)
{ 
   setHostname("localhost");
   setPort(DEFAULT_RECV_PORT);
}
/**
 * Constructor.
 * @param hostname the ip address or hostname of the Address (layer 3).
 * @param port     the port number of UDP or TCP socket (layer 4).
 */
/*
SockGate::SockGate(char *hostname, short port): Gate(), sockfd_(0)
{
  setHostname(hostname);
  setPort(port);
}
*/


/** Init Funciton to initialize a socket port
 *  default port number is set to 4000, hostname is set to "localhost"
 *
 * if the myaddr_ is not set by command line ( both hostname and port has to be set simultaneously),
 * default address parameters will be given in Init().
 */
void SockGate::init()
{
  
  if (sockfd_ != 0) {
    return;
  }
  if ( myaddr_.isSet() == false) {
    setHostname("localhost");
    setPort(DEFAULT_RECV_PORT);
  }
}


/**
 * Function to check if the IP address of receiving interface is set in myaddr_
 * Giving an IP address is the strongest argument to specify a receiving interface,
 * instead of using hostname
 * or "localhost". This is necessary when LIBMAC functions are desired.
 */

bool SockGate::isIPAddrSet()
{
  if (inet_addr(myaddr_.getHostname()) == INADDR_NONE ) return false;
  return true;
}


 /**
  * Fill sockaddr_in 'address' structure with information taken from
  * 'addr' and return it cast to a 'struct sockaddr'.
  * It handles following situations:
  * - if hostname is given as empty "", then INADDR_ANY is used in return
  * - if an IP address is given, then address could be set directly
  * - if a hostname is given, call gethostbyname() to find the ip address of the hostname from DNS
  */
struct sockaddr * SockGate::setSockAddress(Address *addr, struct sockaddr_in *address)
{
  char *hostname;
  int port;
  unsigned int tmp;
  struct hostent *hp;

  hostname = addr->getHostname();
  port = addr->getPort();

  address->sin_family = AF_INET;
  address->sin_port   = htons((short)port);
      
  if (strcmp(hostname, "") == 0) {
    address->sin_addr.s_addr = htonl(INADDR_ANY);  
  } 
  else {
    tmp = inet_addr(hostname);  // If an IP address is given
    if(tmp != (unsigned long) INADDR_NONE){
     
      address->sin_addr.s_addr = tmp;  
    } 
    else{  // if a hostname is passed, call DNS
      if ((hp = gethostbyname(hostname)) == NULL) {
	throw "Error in Resolving hostname!" ;                    
      }
      memcpy((char *)&address->sin_addr, (char *)hp->h_addr, hp->h_length);
    }
  }
  return (sockaddr*)address;
}

/**
 * Function to interpreate hostname and port from the SocketAddress
 *
 */

void SockGate::decodeSockAddress(Address *addr, struct sockaddr_in *address)
{
  addr->setHostname(inet_ntoa(address->sin_addr));
  addr->setPort(ntohs(address->sin_port));  
}


