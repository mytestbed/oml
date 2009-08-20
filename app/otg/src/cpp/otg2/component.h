/**
 * This defines the parent class for all configurable 
 * objects in OTG. It primarily provides support
 * for POPT to configure an object from command
 * line options.
 */

#ifndef OTG2_COMPONENT_H_
#define OTG2_COMPONENT_H_

#include <popt.h>
#include <string>

class IComponent {

public:
  virtual ~IComponent() {};
  
  /**
    * Function to initialize the component.
    * Can only be called once and should be
    * called after options have been set.
    */
  virtual void 
  init() = 0;
  
  /**
   * Function to get Command line Options
   *
   */
  virtual const struct poptOption* 
  getOptions() = 0;
  
};


class Component : public IComponent {

public:
  Component();
  
  /**
   * Function to get Command line Options
   *
   */
  const struct poptOption* 
  getOptions();
  
protected:
  virtual const char*
  getNamespace();
  
  inline virtual void
  defOpts() {}
  
  void 
  defOpt( 
    const char * longName = NULL,
    int argInfo = 0,
    void* arg = 0,
    const char* descrip = NULL,
    const char * argDescrip = NULL); 

  char* namespace_;

private:
  struct poptOption* opts_;
  int opts_length_;
  int opts_count_;

};
#endif /* OTG2_COMPONENT_H_*/
