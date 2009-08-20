
#define APP_NAME "OTG2 Traffic Generator"
#define COPYRIGHT "Copyright (c) 2005-07 WINLAB, 2007-08 NICTA"

#include <fstream>
#include <iostream>
using namespace std;

#include <sys/time.h>
#include <iostream>
using namespace std;

#include <otg2/port.h>
#include <otg2/otg2_app.h>

#define STDIN 0
#define MAX_INPUT_SIZE 256

OTG::OTG(
  int argc, 
  const char* argv[],
  char*       senderName,
  char*       sourceName,
  const char* appName,
  const char* copyright
): Application(argc, argv)

{
  sender_name_ = senderName == NULL ? Port::getDefOutPortName() : senderName;
  source_name_ = sourceName == NULL ? Generator::getDefGeneratorName() : sourceName;
  
  app_name_ = appName == NULL ? APP_NAME : appName;
  copyright_ = copyright == NULL ? COPYRIGHT : copyright;

  setSenderInfo("protocol", 'p', "Protocol to use to send packet", Port::listOutPorts());
  setSourceInfo("generator", 'g', "Generator producing packets", Generator::listGenerators());

}


const struct poptOption*
OTG::getComponentOptions(
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
OTG::createSender(
  char* name
) {
  return Port::createOutPort(name);
}

ISource*
OTG::createSource(
  char* name
) {
  return Generator::create(name);
}


