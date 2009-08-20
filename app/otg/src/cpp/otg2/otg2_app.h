#ifndef OTG2_APP_H_
#define OTG2_APP_H_

#include <otg2/application.h>

/**
*  Main funciton of the OTG
*
* The major purpose of this function are:
* - handle command-line inputs, before starting the program and at run-time. 
* - arrange wait, send, and stdin reading operations.
*
* It is designed as: First, parse all options in two phases.
* After parsing the second level options, check the readiness of Port and Generator (if this is sender)
* then initialize the port and set-up the scenario, and ready to start.

* Here. we are going to have multiple streams in one OTG.  
* using multiple thread. the main thread is handling commands.
* each stream is a seperate thread, by calling th stream_init functon, the stream is independently sending packets 
* 
*/
class OTG : public Application

{
public:
  OTG(
    int argc, 
    const char* argv[],
    char* senderName = NULL,
    char* sourceName = NULL,
    const char* appName = NULL,
    const char* copyright = NULL
  );

protected:

  const struct poptOption*
  getComponentOptions(
    char*       component_name
  );
  
  Sender*
  createSender(
    char* senderName
  );

  ISource*
  createSource(
    char* sourceName
  );
  
};

#endif /*OTG2_APP_H_*/
