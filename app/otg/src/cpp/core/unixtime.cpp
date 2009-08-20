#include "otg2/unixtime.h"
#include <iostream>
using namespace std;

//#define HOUR_OFFSET 313171 
// 313171 hours passed at 3pm, Sep.22, 2005 since Jan.1st, 1970

/** Constructor
 *
 */
UnixTime::UnixTime(int externalcaliber)
{
  setOrigin();
  if (externalcaliber == -1 )
  {
      time_t seconds;
      seconds = time (NULL);
      int days= seconds/(3600*24);// since Jan,1, 1970    
      setAbsoluteOrigin(days*24);
  }
  else
     setAbsoluteOrigin(externalcaliber);

  paused_ =  false;
  //cout << abs_origin_ <<endl;
}

/**
 * Set the orginal Starting time of clock.
 * this is the zero point of a clock.
 */

void UnixTime::setOrigin()
{
  struct timeval tp;
  gettimeofday(&tp, NULL);
  origin_ = tp.tv_sec + tp.tv_usec/1e6;
}

/**
 *Function to get Absolute time value
 */
double UnixTime::getAbsoluteTime()
{  
  struct timeval tp;
  gettimeofday(&tp, NULL);
  //cout << tp.tv_sec + tp.tv_usec/1e6 - abs_origin_ <<endl;
  return (tp.tv_sec + tp.tv_usec/1e6 - abs_origin_);
}



/**
 * Function to Get Current relative time refer to the origin time.
 * @return clocktime.if the clock is paused, the pausing time instatnt is returned
 */

double UnixTime::getCurrentTime()
{ 
  if (paused_) return pauseinstant_;  
  //return (getAbsoluteTime() - origin_ );
  struct timeval tp;
  gettimeofday(&tp, NULL);
  return (tp.tv_sec + tp.tv_usec/1e6 - origin_);
  
}

/**
 * Function to pause the clock.
 * First,  record the time of now. 
 * Second, set flag as paused. 
 * The order of above two operations cannot be reversed.
 */

bool UnixTime::pauseClock()
{
  if (paused_ == true) return false;
  pauseinstant_ = getCurrentTime();
  paused_ = true;
  return true;
}
 
/**
 * Function to resume the clock.
 * As clock is paused for some interval, but the real-world (system) clock never paused, we need to
 * shift the orginal time to a later time instant.
 */


bool UnixTime::resumeClock()
{
  if (!paused_) return false;
  paused_ = false;
  shiftOrigin( getCurrentTime() - pauseinstant_ );
  return true; 
}


/**
 *Function to wait till a specified time
 *
 */
void UnixTime::waitAt(double timestamp)
{
  double x = getCurrentTime();	
  if (timestamp > x )
    {
      unsigned long y = (unsigned long)(1e6*(timestamp-x));
      //if (y >10000000)
      //       printf ("Warning: unusual long packet interval time! longer than 10 seconds!!!\n");
      // else
          usleep(y);
    }
}

