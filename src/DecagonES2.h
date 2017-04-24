/*
 *DecagonES2.h
 *This file is part of the EnviroDIY modular sensors library for Arduino
 *
 *Initial library developement done by Sara Damiano (sdamiano@stroudcenter.org).
 *
 *This file is for the Decagon Devices ES-2 Electrical Conductivity Sensor
 *It is dependent on the EnviroDIY SDI-12 library and the DecagonSDI12 super class.
 *
 *Documentation fo the SDI-12 Protocol commands and responses
 *for the Decagon 5TM can be found at:
 * http://manuals.decagon.com/Integration%20Guides/ES-2%20Integrators%20Guide.pdf
 *
 * For Specific Conductance:
 *  Resolution is 0.001 mS/cm = 1 µS/cm
 *  Accuracy is ±0.01mS/cm or ±10% (whichever is greater)
 *  Range is 0 – 120 mS/cm (bulk)
 *
 * For Temperature:
 *  Resolution is 0.1°C
 *  Accuracy is ±1°C
 *  Range is -40°C to +50°C
*/

#ifndef DecagonES2_h
#define DecagonES2_h

#include "DecagonSDI12.h"

#define ES2_NUM_MEASUREMENTS 2

#define ES2_COND_RESOLUTION 0
#define ES2_COND_VAR_NUM 0

#define ES2_TEMP_RESOLUTION 1
#define ES2_TEMP_VAR_NUM 0

// Forward declare classes
class DecagonES2_Cond;
class DecagonES2_Temp;

// The main class for the Decagon ES-2
class DecagonES2 : public virtual DecagonSDI12
{
public:
    // Constructors with overloads
    DecagonES2(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
     : Sensor(dataPin, powerPin, F("DecagonES2"), ES2_NUM_MEASUREMENTS),
       DecagonSDI12(ES2_NUM_MEASUREMENTS, SDI12address, powerPin, dataPin, numReadings)
    {}
    DecagonES2(char *SDI12address, int powerPin, int dataPin, int numReadings = 1)
     : Sensor(dataPin, powerPin, F("DecagonES2"), ES2_NUM_MEASUREMENTS),
     DecagonSDI12(ES2_NUM_MEASUREMENTS, SDI12address, powerPin, dataPin, numReadings)
    {}
    DecagonES2(int SDI12address, int powerPin, int dataPin, int numReadings = 1)
     : Sensor(dataPin, powerPin, F("DecagonES2"), ES2_NUM_MEASUREMENTS),
       DecagonSDI12(ES2_NUM_MEASUREMENTS, SDI12address, powerPin, dataPin, numReadings)
    {}
};


// Defines the "Ea/Matric Potential Sensor"
class DecagonES2_Cond : public virtual Variable
{
public:
    DecagonES2_Cond(Sensor *parentSense)
     : Variable(parentSense, ES2_COND_VAR_NUM,
                F("specificConductance"), F("microsiemenPerCentimeter"),
                ES2_COND_RESOLUTION, F("ES2Cond"))
    {}
};

// Defines the "Temperature Sensor"
class DecagonES2_Temp : public virtual Variable
{
public:
    DecagonES2_Temp(Sensor *parentSense)
     : Variable(parentSense, ES2_TEMP_VAR_NUM,
                F("temperature"), F("degreeCelsius"),
                ES2_TEMP_RESOLUTION, F("ES2temp"))
    {}
};

#endif
