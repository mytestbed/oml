#ifndef OSOCKET_H_
#define OSOCKET_H_

//#include "unixtime.h"
#include "otg2/component.h"
#include "otg2/address.h"
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>

#ifndef INADDR_NONE
#  define INADDR_NONE (-1)
#endif


class Socket: public Component 

{
public:
  Socket();
  
  void init();
  void close();

protected:
  void
  defOpts();

  struct sockaddr* 
  setSockAddress(Address *addr, struct sockaddr_in *address);
  struct sockaddr*
  setSockAddress(char* hostname, int   port, struct sockaddr_in *address);
  
  void 
  decodeSockAddress(Address *addr, struct sockaddr_in *address);
  
  virtual void
  initSocket() = 0;
  
  int buffersize_; ///<sender or receiver buffer sockopt... IMPLEMENT ME LATER     
  int sockfd_; ///<socket file descriptor
  
  int nblockflag_;
  
  char* localhost_;
  int   localport_;
  char* dsthost_;
  int   dstport_;


};


#endif
