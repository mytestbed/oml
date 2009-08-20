#ifndef RAMD_H
#define RAND_H

#include <math.h>
#include <stdlib.h>



class RandomVariable
{

 public:
  virtual  ~RandomVariable(){}
   virtual double getSample()=0;
 protected:
  double mean_;
  double variance_;
};

/**
 * Generate unifomr randome variable in [0,1)
 */
class UniformRandomVariable : public RandomVariable
{
  // mean_ is 0.5
 public:
  double getSample();
};


/**
 *
 */
class ExponentialRandomVariable: public RandomVariable
{
 public:
  void setMean(double mean);
  double getSample();
};


#endif

