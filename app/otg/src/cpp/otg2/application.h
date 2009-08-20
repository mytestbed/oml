#ifndef APPLICATION_H_
#define APPLICATION_H_

#include <popt.h>

#include "otg2/stream.h"
#include "otg2/generator.h"
#include "otg2/port.h"

#define HELP_FLAG 99
#define USAGE_FLAG 98
#define VERSION_FLAG 97

class Application

{
public:
  
  Application(int argc, const char * argv[], const char* defLogFile = "-");
  virtual ~Application() {};
  
  virtual void
  run();
  
protected:
  
  virtual Sender*
  createSender(
    char* senderName
  ) = 0;

  virtual ISource*
  createSource(
    char* sourceName
  ) = 0;

  void 
  showHelp(poptContext optCon, char* component_name);
  
  virtual void
  printVersion();

  virtual const struct poptOption*
  getComponentOptions(
    char*       component_name
  ) = 0;
  
  void 
  parseOptionsPhase1();
  void
  parseOptionsPhase2(); 

  int 
  parseRuntimeOptions(
    char*   msg
  );
  
  void
  setSenderInfo(
    const char* longName,
    char        shortName,        /* may be ’\0’ */
    char*       descrip,        /* description for autohelp -- may be NULL */
    char*       argDescrip
  );

  void
  setSourceInfo(
    const char* longName,
    char        shortName,        /* may be ’\0’ */
    char*       descrip,        /* description for autohelp -- may be NULL */
    char*       argDescrip
  );

  struct poptOption* phase1_;
  struct poptOption* phase2_;  

  int argc_;
  const char** argv_;

  char* sender_name_;
  char* source_name_;
  int   clockref_; 
  char* component_name_;
  
  ISource*   source_; 
  Sender*    sender_;
  Stream*    stream_;
  
  int   log_level_;
  const char* logfile_name_;
  
  const char* app_name_;
  const char* copyright_;
  
};

#endif /*APPLICATION_H_*/
