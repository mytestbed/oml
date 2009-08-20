
#ifdef WITH_OML
extern "C" {
#  include <oml2/omlc.h>
}
#endif

#include <stdlib.h>
#include <fstream>
#include <iostream>
using namespace std;

#include <sys/time.h>
#include <iostream>
using namespace std;
#include "otg2/application.h"

# include <ocomm/o_log.h>

#include "../version.h"


#define STDIN 0
#define MAX_INPUT_SIZE 256

static struct poptOption 
phase1[] = {
  { "help", 'h', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, NULL, HELP_FLAG, "Show help", "[component]"},
  { "usage", '\0', POPT_ARG_NONE, NULL, USAGE_FLAG, "Display brief use message"},     
  { "sender", '\0', POPT_ARG_STRING, NULL, 0},
  { "source", '\0', POPT_ARG_STRING, NULL, 0},
  { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0},  
  { "debug-level", 'd', POPT_ARG_INT, NULL, 0, "Debug level - error:-2 .. debug:1-3"},
  { "logfile", 'l', POPT_ARG_STRING, NULL, 0, "File to log to"},
  { "version", 'v', 0, 0, VERSION_FLAG, "Print version information and exit" },
  { NULL, 0, 0, NULL, 0 }
};

static struct poptOption 
phase2[] = {
    { "help", 'h', POPT_ARG_STRING | POPT_ARGFLAG_OPTIONAL, NULL, HELP_FLAG, "Show help", "[component]"},
    { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0},
    { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0},
    { "sender", '\0', POPT_ARG_STRING, NULL, 0},
    { "source", '\0', POPT_ARG_STRING, NULL, 0},
    { NULL, '\0', POPT_ARG_INCLUDE_TABLE, NULL, 0},
    { "debug-level", 'd', POPT_ARG_INT, NULL, 0, "Debug level - error:-2 .. debug: 1-3"},
    { "logfile", 'l', POPT_ARG_STRING, NULL, 0, "File to log to"},
    { "version", 'v', 0, 0, 'v', "Print version information and exit" },
    { "exit", '\0', POPT_ARG_NONE, NULL, 1, "Stop the generator and exit" },
    { "pause", '\0', POPT_ARG_NONE, NULL, 2, "pause the generator and exit" },
    { "resume", '\0', POPT_ARG_NONE, NULL, 3, "resume the generator" },
    { NULL, 0, 0, NULL, 0 }
};


/**
*  Genral structure of an OTx application
*
* The major purpose of this function are:
* - handle command-line inputs, before starting the program and at run-time. 
* - arrange wait, send, and stdin reading operations.
*
* It is designed as: First, parse all options in two phases.
* After parsing the second level options, check the readiness of Source and Sender (if this is sender)
* then initialize the port and set-up the scenario, and ready to start.

* Here. we are going to have multiple streams in one OTG.  
* using multiple thread. the main thread is handling commands.
* each stream is a seperate thread, by calling th stream_init functon, the stream is independently sending packets 
* 
*/


Application::Application(int argc, const char * argv[], const char* defLogFile)

{
#ifdef WITH_OML
 /* const char* name = argv[0];
  const char* p = name + strlen(argv[0]);
  while (! (p == name || *p == '/')) p--;
  if (*p == '/') p++;
  name = p;*/
  
  omlc_init(argv[0], &argc, argv, o_log); 
#endif

  argc_ = argc;
  argv_ = argv;
  //cout << "initialisation of the application \n" <<endl;
  clockref_ = -1;
  
  component_name_ = NULL;
  
  log_level_ = O_LOG_INFO;
  logfile_name_ = defLogFile;
  
  stream_ = new Stream();
  //cout << "initialisation of the application after Stream creation \n " <<endl;
  
  phase1_ = phase1;
  phase1_[0].arg = &component_name_;
  phase1_[2].arg = &sender_name_;
  phase1_[3].arg = &source_name_;
  phase1_[4].arg = (void*)stream_->getOptions();
  phase1_[5].arg = &log_level_;
  phase1_[6].arg = &logfile_name_;
  //cout << "initialisation of the application end of phase 1 \n " <<endl;
  phase2_ = phase2;
  phase2_[0].arg = &component_name_;
  phase2_[4].argDescrip = "FIXED";
  phase2_[5].arg = (void*)stream_->getOptions();
  phase2_[6].argDescrip = "FIXED";
  phase2_[7].argDescrip = "FIXED";  
  phase2_[8].argDescrip = "FIXED";
  //cout << "initialisation of the application end of phase 2 \n " <<endl;
  
}


/**
*  Parse options for phase 1: Protocol and Generator Type
*/
void 
Application::parseOptionsPhase1() 

{
  int rc;
  poptContext optCon = poptGetContext(NULL, argc_, argv_, phase1_, 0);
  while ((rc = poptGetNextOpt(optCon)) != -1) {
    switch (rc) {
      case HELP_FLAG:
        showHelp(optCon, component_name_);
        exit(0);
      case USAGE_FLAG:
        poptPrintUsage(optCon, stdout, 0);
        exit(0);
      case VERSION_FLAG:
        printVersion();
        exit(0);
      case POPT_ERROR_BADOPT:
        // ignore here
        break;
      default:
        cout << "Unknown flag operation '" << rc << "'." << endl;
        exit(-1);
    }
  }
  poptFreeContext(optCon);
  o_set_log_file((char*)logfile_name_);
  o_set_log_level(log_level_);
  
  o_log(O_LOG_INFO, "%s V%s\n", app_name_, OTG2_VERSION);
  o_log(O_LOG_INFO, "%s\n", copyright_);
  
}

void
Application::showHelp(
  poptContext optCon,
  char*       component_name
) {
  if (component_name == NULL) {
    // common help
    poptPrintHelp(optCon, stdout, 0);
  } else {
    // help about a component. Let's find it.
    const struct poptOption* opts= getComponentOptions(component_name);
    if (opts == NULL) {
      cout << "Unknown component '" << component_name << "'." << endl <<endl;
    } else {
      poptContext ctxt =
          poptGetContext(NULL, argc_, argv_, opts, POPT_CONTEXT_NO_EXEC);
      poptPrintHelp(ctxt, stdout, 0);
    }
  }
}

/**
*  Parse options in second Phase.
*  Check all port and generator parameters except "type" which has already been parsed.
*  in Phase I.
*/
void
Application::parseOptionsPhase2() 

{
  int rc;
  poptContext optCon = poptGetContext(NULL, argc_, argv_, phase2_, 0);
  while ((rc = poptGetNextOpt(optCon)) >= 0);
  if (rc < -1) {
    cerr << "ERROR: " << poptBadOption(optCon, POPT_BADOPTION_NOALIAS)
            << " (" << poptStrerror(rc) << ")" << endl;
    poptPrintUsage(optCon, stderr, 0);
    exit(-1);
  }

//  const char** leftover = poptGetArgs(optCon);
//  if (leftover != NULL) {
//    cerr << "ERROR: Additional argument '" << leftover[0] << "' found." << endl;
//    goto err;
//  }
  poptFreeContext(optCon);
  return;
}

int 
Application::parseRuntimeOptions(
  char * msg
) {
  int argc;
  const char** argv;
  char strin[MAX_INPUT_SIZE];

  if (*msg == '\0') return -1;
  if (*msg != '-') {
    // allow for commands without leading --
    strcpy(strin + 2, msg);
    strin[0] = strin[1] = '-';
    msg = strin;
  }
  poptParseArgvString(msg, &argc, &argv);
//  char* component_name;
//  phase2_[0].arg = &component_name;
  poptContext optCon = poptGetContext(NULL, argc, argv, phase2_, POPT_CONTEXT_KEEP_FIRST);
 
  int rc;
  while ((rc = poptGetNextOpt(optCon)) > 0) {
    switch(rc) {
      case 1: // Stop
        stream_->exitStream();
        exit(0);  //exit terminate process and all its threads
      case 2:
        stream_->pauseStream();  
        break;
      case 3:
        stream_->resumeStream();
        break;
      case HELP_FLAG:
        showHelp(optCon, component_name_);
        break;
      case VERSION_FLAG:
        printVersion();
        break;
    }
  }
  if (rc < -1) {
    cerr << "ERROR: " << poptBadOption(optCon, POPT_BADOPTION_NOALIAS)
            << " (" << poptStrerror(rc) << ")" << endl;
  }
  poptFreeContext(optCon);
  return rc;  
}





void
Application::run() 

{
  
  parseOptionsPhase1();
  
  source_ = createSource(source_name_);

  if (source_ == NULL) {
    cerr << "Error: Unknown source '" << source_name_ << "'." << endl;
    exit(-1);
  }
  
  sender_ = createSender(sender_name_);

  if (sender_ == NULL) {
    cerr << "Error: Unknown sender '" << sender_name_ << "'." << endl;
    exit(-1);
  }
  //cout << "Sender/Source created  " <<endl;
  
  phase2_[1].arg = (void*)sender_->getConfigurable()->getOptions();
  phase2_[2].arg = (void*)source_->getConfigurable()->getOptions();
  parseOptionsPhase2();
  //cout << "Parsing phase 2 finished  " <<endl;
  
  source_->getConfigurable()->init();
  sender_->getConfigurable()->init();
  stream_->setSource(source_);
  stream_->setSender(sender_);
  
  //cout << "Stream configured   " <<endl;
  
  
#ifdef WITH_OML
  omlc_start();
#endif
  
  stream_->run();
  
  int rc;
  char msg[MAX_INPUT_SIZE];
  while (1) {
    cin.getline(msg, MAX_INPUT_SIZE );
    rc = parseRuntimeOptions(msg);    
  }
}

void
Application::printVersion()

{  
  cout << app_name_ << " V" << OTG2_VERSION << endl;
  cout << copyright_ << endl;
}

void
Application::setSenderInfo(
  const char* longName,
  char        shortName,        /* may be ’\0’ */
  char*       descrip,        /* description for autohelp -- may be NULL */
  char*       argDescrip   
) {
  struct poptOption& p1 = phase1_[2];
  struct poptOption& p2 = phase2_[3];  
  
  p1.longName = p2.longName = longName;
  p1.shortName = p2.shortName = shortName;
  p1.descrip = p2.descrip = descrip;
  p1.argDescrip = p2.argDescrip = argDescrip;
}

void
Application::setSourceInfo(
  const char* longName,
  char        shortName,        /* may be ’\0’ */
  char*       descrip,        /* description for autohelp -- may be NULL */
  char*       argDescrip   
) {
  struct poptOption& p1 = phase1_[3];
  struct poptOption& p2 = phase2_[4];  
  
  p1.longName = p2.longName = longName;
  p1.shortName = p2.shortName = shortName;
  p1.descrip = p2.descrip = descrip;
  p1.argDescrip = p2.argDescrip = argDescrip;
}

