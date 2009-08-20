
#include <stdlib.h>
#include <string.h>
#include <otg2/component.h>

/**
 * Constructor.
 */

Component::Component()

{
  opts_ = NULL;
}

const struct poptOption* 
Component::getOptions()

{
  if (opts_ == NULL) {
    opts_length_ = 20;
    opts_count_ = 0;
    opts_ = (struct poptOption*)calloc(opts_length_, sizeof(struct poptOption));
    this->defOpts();
  }
  return opts_;
}

const char*
Component::getNamespace()

{
  return NULL;
}

void
Component::defOpt( 
  const char * longName,
  int argInfo,
  void* arg,
  const char* descrip,
  const char * argDescrip
) {
  struct poptOption* o = &opts_[opts_count_++];
  const char* nameSpace = this->getNamespace();

  if (nameSpace != NULL && longName != NULL) {
    int len = strlen(nameSpace) + strlen(longName) + 2;
    char* s = (char*)malloc(len);
    sprintf(s, "%s:%s", nameSpace, longName);
    longName = s;
  }
  o->longName = longName;
  o->shortName = '\0';
  o->argInfo = argInfo | POPT_ARGFLAG_SHOW_DEFAULT;
  o->arg = arg;
  o->val = 0;
  o->descrip = descrip;
  o->argDescrip = argDescrip;
}
