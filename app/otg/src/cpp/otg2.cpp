#include <iostream>
using namespace std;

#include <otg2/otg2_app.h>

int main(
  int argc, 
  const char * argv[]
) {
  try {
    OTG* otg = new OTG(argc, argv);
    //otg->registerOutPortType("mp_udp", createMPOutPort);
    otg->run();
  } catch (const char *reason ) {
    cerr << "Exception: " << reason << endl;
    return -1;
  }  
  return 0;
}


