/*--------------------------------------------------------------- 
 * Copyright (c) 1999,2000,2001,2002,2003                              
 * The Board of Trustees of the University of Illinois            
 * All Rights Reserved.                                           
 *--------------------------------------------------------------- 
 * Permission is hereby granted, free of charge, to any person    
 * obtaining a copy of this software (Iperf) and associated       
 * documentation files (the "Software"), to deal in the Software  
 * without restriction, including without limitation the          
 * rights to use, copy, modify, merge, publish, distribute,        
 * sublicense, and/or sell copies of the Software, and to permit     
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions: 
 *
 *     
 * Redistributions of source code must retain the above 
 * copyright notice, this list of conditions and 
 * the following disclaimers. 
 *
 *     
 * Redistributions in binary form must reproduce the above 
 * copyright notice, this list of conditions and the following 
 * disclaimers in the documentation and/or other materials 
 * provided with the distribution. 
 * 
 *     
 * Neither the names of the University of Illinois, NCSA, 
 * nor the names of its contributors may be used to endorse 
 * or promote products derived from this Software without
 * specific prior written permission. 
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES 
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT 
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE. 
 * ________________________________________________________________
 * National Laboratory for Applied Network Research 
 * National Center for Supercomputing Applications 
 * University of Illinois at Urbana-Champaign 
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________ 
 *
 * Gps.hpp
 * by Olivier Mehani <olivier.mehani@inria.fr>
 * -------------------------------------------------------------------
 * Interfaces with a serial GPS to get universal time and position
 * information.
 * ------------------------------------------------------------------- */

#ifndef GPS_H
#define GPS_H

#include <iostream>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "Settings.hpp"

/* 
 * Computed as an average between the Earth's equatorial and polar radii, in
 * meters as given in Wikipedia as of 2007-01-17
 * http://en.wikipedia.org/wiki/Earth_radius
 * */
#define EARTH_RADIUS	((double)6367514)

/*
 * 2*pi/360.
 */
#define DEG2RAD		((float).0174532925)

class Gps {
public:
    Gps( ext_Settings *inSettings );
    ~Gps( void );
    
    const char* GetDevice();
    bool IsValid();
    void Update();
    void SetBaseCoordinates(float latitude, float longitude);
    void AcquireBaseCoordinates();
    float GetInitLatitude();
    float GetInitLongitude();
    float GetLastLatitude();
    float GetLastLongitude();
    float GetLastXCoordinate();
    float GetLastYCoordinate();
    float GetDistanceFromBase();
    int GetLastTime();
    bool GetWarning();
    
    bool Initialized;

private:
    void OpenDevice(char* device);
    void CloseDevice();
    std::string ReadLine();
    bool ParseNMEA(std::string line);
    void ParseDateTime(std::string Date, std::string Time);
    void ParseLatitude(std::string latitude, std::string dir);
    void ParseLongitude(std::string longitude, std::string dir);

    char* GpsDevice;
    int GpsDescriptor;
    bool GpsValid;
    float InitLatitude;
    float InitLongitude;
    int DateTime;
    bool Warning;
    float Latitude;
    float Longitude;
};

#endif /* GPS_H */
