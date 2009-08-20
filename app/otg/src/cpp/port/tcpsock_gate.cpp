#include "tcpsock_gate.h"
#include "tcpsock_gate_helper.h"

#include "stream.h"
#include <iostream>
using namespace std;
#include <netdb.h>
#include <arpa/inet.h>
//#include "dummysink.h"

#define BACKLOG   10  ///< Maximum 10 connections for a TCP server



/** Init Funciton to initialize a TCP socket port
 *  default port number is set to 4500 for receiver, hostname is set to "localhost"
 *
 * if the myaddr_ is not set by command line ( both hostname and port has to be set simultaneously),
 * default address parameters will be given in init().
 * The port bind to its own address, generate a TCP socket.
 * This TCP socket will be used by sender to send packets, and for receiver to "listen".
 * Packet reception will use new socket file desciptor.
 *
 * Bind to local address is one important task in init() 
 * Here source address of node itself (myaddr_) does not really be used by bind function of port.
 * The program use INADDR_ANY as the address filled in address parameters of bind().
 * So, we need an empty hostname with the port number. 
 */

void TCPSockGate::init()
{

  SockGate::init(); 
  if ((sockfd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("socket");
    throw "Error while opening TCP socket";
  }
  Address *emptyAddr = new Address("", myaddr_.getPort());
  struct sockaddr* addr = setSockAddress(emptyAddr, &mySockAddress_);
  if (  bind(sockfd_, addr, sizeof(struct sockaddr_in)) < 0)
     throw " TCP Socket Bind error";
  cout << "listening TCP connections from port:" << myaddr_.getPort() << "......" << endl;  
  // TCP revceiver is actually a TCP server
  listen(sockfd_, BACKLOG);  

}


/**
 *  Function to start an endless loop for Listing TCP connections 
 *  This Marks the beginning of an independent receiving thread
 *  Each incomming connection will be assigned to a flow.
 */
void TCPSockGate::startReceive()
{
  int i,fdmax, newfd;
  fd_set readfds, master;
  FD_ZERO(&readfds);
  FD_ZERO(&master);
  FD_SET(sockfd_,&master);
  fdmax = sockfd_;
  while (1)
  {  
        readfds =  master;
        select(fdmax+1, &readfds, (fd_set *)0, (fd_set *)0, NULL);
        for ( i=0; i<=fdmax; i++)
	  if (FD_ISSET(i, &readfds))
	    { 
               if (i == sockfd_)
	       {
	            newfd =  acceptNewConnection();
                    FD_SET(newfd, &master);  // add to master set
                       if (newfd > fdmax)    // keep track of the maximum
                              fdmax = newfd;
		     
	       } 
	       else{
                 if (receivePacket(i)== false)
		               FD_CLR(i, &master); 
		 else
		    inboundPacket(); 
	       }
	    }
	          
   }   // end while   
}

/**
 * This will use rlcurr_ to receive a packet.
 *
 */
bool TCPSockGate::receivePacket(int fd)
{
  rlcurr_ = searchFlowbyFd(fd);
  //rlcuur_->getID is the flow id (socket fd) of this flow
  //pkt_ = rlcurr_->packetcache_;
  int len = (int)recv(rlcurr_->getID(), pkt_->getPayload(), pkt_->getBufferSize(),0); 
  if (len == -1) perror("recvfrom");
  if (len <= 0  )
    {
      close(rlcurr_->getID()); // Connection has been closed by the other end (==0) or error occurs
      return false;
    }
  pkt_->rxMeasure_->setReceivedLength(len);
  //pkt_->rxMeasure_->setRxTime((long)(gateclock_.getCurrentTime()*1e6) );
 
  return true;
}


/**
 * Function to accept new connections
 * After reaching maximum connections, the default sockfd_ should be closed to prohibit new connections
 * @return the new socket file descriptor
 */

int  TCPSockGate::acceptNewConnection()
{ 
  struct sockaddr_in tmpSockAddr; // connector's address information
  int sin_size =  sizeof(struct sockaddr_in);
  int newfd = accept(sockfd_, (struct sockaddr *)&tmpSockAddr, (socklen_t*)&sin_size);

  if ( newfd == -1){
    perror("accept");
    throw " accept new coonnection error.";
  }
  cout << "accepting a new connection" << endl;
  decodeSockAddress(&itsaddr_, &tmpSockAddr);
  addFlow(newfd, &itsaddr_);
  //decodeSockAddress(rltail_->getAddr(), &tmpSockAddr);  

 //add some code to prohibit new connecitons if there are already 10 connections. Fix ME!!!
 
  return newfd;
}


const struct poptOption* TCPSockGate::getOptions()

{
  //  return tcp_sock_gate_get_options(this, NULL);
  return NULL;
}

