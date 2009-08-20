#include <iostream>
using namespace std;

#include <otg2/otr2_app.h>

int main(
  int argc, 
  const char * argv[]
) {
  try {
    OTR* otr = new OTR(argc, argv);
    otr->run();
  } catch (const char *reason ) {
    cerr << "Exception: " << reason << endl;
    return -1;
  }  
  return 0;
}
