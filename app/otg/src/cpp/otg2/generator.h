
#ifndef GENERATOR_H
#define GENERATOR_H

#include "otg2/source.h"

/** 
 * Generator is an interface for any componet which can generate 
 * a packet. It is usually called from a stream.
 */
class Generator: public ISource

{     
public:
  static Generator* create(char* name);  
  static char* getDefGeneratorName();
  static char* listGenerators();
  
};



#endif
