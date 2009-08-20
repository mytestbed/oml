
#define APP_NAME "OTG2 Traffic Sink"
#define COPYRIGHT "Copyright (c) 2005-07 WINLAB, 2007-08 NICTA"


#include <fstream>
#include <sys/time.h>
#include <iostream>
using namespace std;

#include "otg2/otr2_app.h"


#define STDIN 0
#define MAX_INPUT_SIZE 256



OTR::OTR(
  int argc, 
  const char* argv[],
  char*       senderName,
  char*       sourceName,
  const char* appName,
  const char* copyright
): Application(argc, argv)
{
  sender_name_ = senderName == NULL ? (char*)"null" : senderName;
  source_name_ = sourceName == NULL ? Port::getDefInPortName() : sourceName;
  
  app_name_ = appName == NULL ? APP_NAME : appName;
  copyright_ = copyright == NULL ? COPYRIGHT : copyright;
  
  setSenderInfo("protocol", 'p', "Protocol to use to send packet", Port::listInPorts());
  setSourceInfo("sink", 'g', "What to do with received packets", Port::listOutPorts());
}


const struct poptOption*
OTR::getComponentOptions(
  char*       component_name
) {
  const struct poptOption* opts= NULL;
  ISource* gen;
  Sender*      port;
  
  if ((gen = createSource(component_name)) != NULL) {
    opts = gen->getConfigurable()->getOptions();
    cout << "Options for generator '" << component_name << "'." << endl
        <<endl;
  } else if ((port = createSender(component_name)) != NULL) {
    opts = port->getConfigurable()->getOptions();
    cout << "Options for port '" << component_name << "'." << endl <<endl;
  }
  return opts;
}


Sender*
OTR::createSender(
  char* name
) {
  return Port::createOutPort(name);
}

ISource*
OTR::createSource(
  char* name
) {
  return Port::createInPort(name);
}


