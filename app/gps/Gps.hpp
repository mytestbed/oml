/*--------------------------------------------------------------- 
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
//#include "Settings.hpp"

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
    Gps( char* device );
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
    float GetLastSpeed();
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
    void ParseSpeed(std::string speedKnots);

    char* GpsDevice;
    int GpsDescriptor;
    bool GpsValid;
    float InitLatitude;
    float InitLongitude;
    float speedKps;
    int DateTime;
    bool Warning;
    float Latitude;
    float Longitude;
};

#endif /* GPS_H */
