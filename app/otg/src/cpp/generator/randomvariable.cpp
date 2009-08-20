#include <cstdlib>
#include <ctime>
#include <iostream>
#include "randomvariable.h"
using std::cout;
using std::endl;


double UniformRandomVariable::getSample()
{ 
    double  x;
    // Set evil seed (initial seed)
    srand( (unsigned)time( NULL ) );
    x = (double) rand()/RAND_MAX;
    //    cout << x << endl;   
    return x;
}

void ExponentialRandomVariable::setMean(double mean)
{
   mean_ = mean;
}

double ExponentialRandomVariable::getSample()
{
   UniformRandomVariable x; 
   //Fix Me! Need some code to calculate based on mean_
   return (-mean_*log(x.getSample()));
   //assume the pdf is 1/sigma^2*exp{-x/sigma^2} mean= sigma^2
}
