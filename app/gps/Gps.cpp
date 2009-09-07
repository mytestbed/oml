/*--------------------------------------------------------------- 
 *
 * Gps.cpp
 * by Olivier Mehani <olivier.mehani@inria.fr>
 * -------------------------------------------------------------------
 * Interfaces with a serial GPS to get universal time and position
 * information.
 * ------------------------------------------------------------------- */

#include <iostream>
#include <sys/stat.h>
#include <fcntl.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "ocomm/o_log.h"
#include <iostream>
using namespace std;
#include "Gps.hpp"
//#include "Locale.hpp"
extern "C" {
#include <oml2/omlc.h>
}
static OmlMPDef oml_def[] = { 
{"longitute", OML_DOUBLE_VALUE},
{"latitude", OML_DOUBLE_VALUE}, 
{"Xposition", OML_DOUBLE_VALUE},
{"Yposition", OML_DOUBLE_VALUE},
{"DistanceFromOrigin", OML_DOUBLE_VALUE},
{"Speed", OML_DOUBLE_VALUE},
{"Time", OML_DOUBLE_VALUE},
{NULL, (OmlValueT)0},
};
static OmlMP* oml_mp = NULL;


/* public members */

Gps::Gps( char* device ) {
    
    int len = strlen(device);
    GpsDevice = new char[len + 1];
    strncpy(GpsDevice, device, len);

    GpsDescriptor = -1;
    OpenDevice(GpsDevice);

    DateTime = 0;
    Warning = true;
    Latitude = 0.;
    Longitude = 0;
    
    Initialized = false;
    InitLatitude = 0;
    InitLongitude = 0;
    
    oml_mp = omlc_add_mp("gps", oml_def);
    
    
}

Gps::~Gps( void ) {
    CloseDevice();
}

const char* Gps::GetDevice() {
    return GpsDevice;
}

bool Gps::IsValid() {
    return GpsDescriptor != -1;
}

void Gps::Update() {
    std::string line;
    bool newdata = false;

    if (IsValid()) {
	do {
	    line = ReadLine();
        //cout<< "current GPS device information " << line << endl;
	    newdata |= ParseNMEA(line);
	} while(line.length() > 0);
	if (!Initialized && newdata) {
	    SetBaseCoordinates(Latitude, Longitude);
	}
    }
}

void Gps::SetBaseCoordinates(float latitude, float longitude) {
    fprintf(stderr, "setting base coordinates to (%2.4f, %3.4f)\n", latitude, longitude);
    InitLatitude = latitude;
    InitLongitude = longitude;
    Initialized = true;
}

void Gps::AcquireBaseCoordinates() {
    while (!Initialized) {
	Update();
    }
}

    float Gps::GetInitLatitude() {
	if(Initialized)
	    return InitLatitude;
	return 0;
    }

    float Gps::GetInitLongitude() {
	if(Initialized)
	    return InitLongitude;
	return 0;
    }

float Gps::GetLastLatitude() {
	return Latitude;
}

float Gps::GetLastLongitude() {
	return Longitude;
}

float Gps::GetLastXCoordinate() {
    return EARTH_RADIUS * DEG2RAD * (Longitude - InitLongitude) * cos(InitLatitude * DEG2RAD);
}

float Gps::GetLastYCoordinate() {
    return EARTH_RADIUS * DEG2RAD * (Latitude - InitLatitude);
}

float Gps::GetDistanceFromBase() {
    float X=GetLastXCoordinate(), Y=GetLastYCoordinate();
    return sqrt(X*X+Y*Y);
}
float Gps::GetLastSpeed(){
    return speedKps;
}

int Gps::GetLastTime() {
    return DateTime;
}

bool Gps::GetWarning() {
    return Warning;
}

/* private members */

void Gps::OpenDevice(char* device) {
    if (!IsValid()) {
	Initialized = false;
	GpsDescriptor = open(device, O_ASYNC|O_NONBLOCK|O_RDONLY);
    }
}

    void Gps::CloseDevice() {
	if(IsValid())
	    close(GpsDescriptor);
    }

std::string Gps::ReadLine() {
    char buf;
    std::string str;
    int len;
    if (IsValid()) {
	do {
	    len = read(GpsDescriptor, &buf, 1);
	    if (len > 0) {
		if (buf == '$')
		    str.erase();
		else if (buf == '\n' && ! str.empty())
		    return str;
		str.append(1, buf);
	    }
	} while (len > 0);
    }
    return "";
}

/*
 * This function only parse the needed subset (RMC)of NMEA frames.
 */
bool Gps::ParseNMEA(std::string line) {
    const char *c_line;
    ssize_t beg, end;
    std::string aTime, aDate, aLatitude, aLatitudeDirection, aLongitude, aLongitudeDirection, speedKnots;
    if (line.empty())
	return false;
    c_line = line.c_str();

    if (strncmp(c_line, "$GPRMC", 6))
	return false;

    /* Time */
    beg = line.find_first_of(',') + 1;
    end = line.find_first_of(',',beg);
    aTime = line.substr(beg, end-beg);

    /* Warning */
    beg = end + 1;
    end = line.find_first_of(',',beg);
    /* We directly set the object's property as it may be modified by the
     * subsequent Parse* methods */
    Warning = (line.substr(beg, end-beg).c_str()[0]=='V')?true:false;

    /* Latitude */
    beg = end + 1;
    end = line.find_first_of(',',beg);
    aLatitude = line.substr(beg, end-beg);
    beg = end + 1;
    end = line.find_first_of(',',beg);
    aLatitudeDirection = line.substr(beg, end-beg);

    /* Longitude */
    beg = end + 1;
    end = line.find_first_of(',',beg);
    aLongitude = line.substr(beg, end-beg);
    beg = end + 1;
    end = line.find_first_of(',',beg);
    aLongitudeDirection = line.substr(beg, end-beg);

    /* Skip two fields... */
    beg = end + 1;
    end = line.find_first_of(',',beg);
    speedKnots = line.substr(beg, end-beg);
    beg = end + 1;
    end = line.find_first_of(',',beg);

    /* Date */
    beg = end + 1;
    end = line.find_first_of(',',beg);
    aDate = line.substr(beg, end-beg);

    ParseDateTime(aTime, aDate);
    ParseLatitude(aLatitude, aLatitudeDirection);
    ParseLongitude(aLongitude, aLongitudeDirection);
    ParseSpeed(speedKnots);

    return true;
}

void Gps::ParseDateTime(std::string Date, std::string Time) {
    std::string aDateTime;
    struct tm tm;
    char buf[16]; /* Should still hold the seconds for a long time */

    if(Date.length() < 6 || Time.length() < 6) {
	fprintf(stderr, "bad date or time format\n");
	Warning = true;
	return;
    }
    DateTime = 0;
    aDateTime.append(Time.substr(0,Time.find_first_of('.')));
    aDateTime.append(Date.substr(0,Date.find_first_of('.')));

    strptime(aDateTime.c_str(), "%d%m%y%H%M%S", &tm);
    strftime(buf, sizeof(buf), "%s", &tm);
    DateTime = atoi(buf);
}

void Gps::ParseLatitude(std::string latitude, std::string dir) {
    int degrees;
    float minutes;

    if(latitude.length() < 7 || dir.length() != 1) {
	fprintf(stderr, "missing latitude or direction\n");
	Warning = true;
	return;
    }

    /* FIXME: will this be working with all GPS's frames ? */
    degrees = atoi(latitude.substr(0,2).c_str());
    minutes = atof(latitude.substr(2).c_str());

    Latitude = degrees + minutes / 60.;

    if(dir.c_str()[0] != 'N')
	Latitude = -Latitude;
}

void Gps::ParseLongitude(std::string longitude, std::string dir) {
    int degrees;
    float minutes;

    if(longitude.length() < 8 || dir.length() != 1) {
	fprintf(stderr, "missing longitude or direction\n");
	Warning = true;
	return;
    }

    /* FIXME: will this be working with all GPS's frames ? */
    degrees = atoi(longitude.substr(0,3).c_str());
    minutes = atof(longitude.substr(3).c_str());

    Longitude = degrees + minutes / 60.;

    if(dir.c_str()[0] != 'E')
	Longitude = -Longitude;
}
void Gps::ParseSpeed(std::string speedKnots){
    speedKps = atof( speedKnots.c_str())*1.852;
    
}

int main( int argc, char **argv ) {
    int* argc_;
    const char** argv_;
    omlc_init(argv[0], &argc, (const char**)argv, o_log);
    

    
    Gps      *theGPS      = NULL;

    
    theGPS = new Gps(argv[1]);
    
    cout << "argv " <<argv[1] <<endl;
    
    omlc_start();
    while(1){
        theGPS->Update();
        OmlValueU v[7];
        if(theGPS->GetLastLatitude() == 0.0 && theGPS->GetLastLongitude() == 0.0)
            ;
        else {
            v[0].doubleValue = theGPS->GetLastLatitude();
            v[1].doubleValue = theGPS->GetLastLongitude();
            v[2].doubleValue = theGPS->GetLastXCoordinate();
            v[3].doubleValue = theGPS->GetLastYCoordinate();
            v[4].doubleValue = theGPS->GetDistanceFromBase();
            v[5].doubleValue = theGPS->GetLastSpeed();
            v[6].doubleValue = theGPS->GetLastTime();
            omlc_process(oml_mp, v);
        }
        sleep(1);
    }
        

    return 0;
} // end main
