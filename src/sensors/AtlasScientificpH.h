/*
 * AtlasScientificpH.h
 * This file is part of the EnviroDIY modular sensors library for Arduino
 *
 * Initial developement for Atlas Sensors was done by Adam Gold
 * Files were edited by Sara Damiano
 *
 * The output from the Atlas Scientifc pH is the temperature in degrees C.
 *     Accuracy is ± 0.002
 *     Range is 0.001 − 14.000
 *     Resolution is 0.001
 */

// Header Guards
#ifndef AtlasScientificpH_h
#define AtlasScientificpH_h

// Debugging Statement
// #define DEBUGGING_SERIAL_OUTPUT Serial

// Included Dependencies
#include "VariableBase.h"
#include "sensors/AtlasParent.h"

// I2C address
#define ATLAS_PH_I2C_ADDR 0x63  // 99

// Sensor Specific Defines
#define ATLAS_PH_NUM_VARIABLES 1

#define ATLAS_PH_WARM_UP_TIME_MS 0
#define ATLAS_PH_STABILIZATION_TIME_MS 0
#define ATLAS_PH_MEASUREMENT_TIME_MS 900

#define ATLAS_PH_RESOLUTION 3
#define ATLAS_PH_VAR_NUM 0

// The main class for the Atlas Scientific pH temperature sensor
class AtlasScientificpH : public AtlasParent
{
public:
    AtlasScientificpH(int8_t powerPin, uint8_t i2cAddressHex = ATLAS_PH_I2C_ADDR,
                      uint8_t measurementsToAverage = 1)
     : AtlasParent(powerPin, i2cAddressHex, measurementsToAverage,
                    "AtlasScientificpH", ATLAS_PH_NUM_VARIABLES,
                    ATLAS_PH_WARM_UP_TIME_MS, ATLAS_PH_STABILIZATION_TIME_MS,
                    ATLAS_PH_MEASUREMENT_TIME_MS)
    {}
    ~AtlasScientificpH(){}
};

// The class for the pH variable
class AtlasScientificpH_pH : public Variable
{
public:
    AtlasScientificpH_pH(Sensor *parentSense,
                        const char *UUID = "", const char *customVarCode = "")
      : Variable(parentSense, ATLAS_PH_VAR_NUM,
                 "pH", "pH",
                 ATLAS_PH_RESOLUTION,
                 "AtlaspH", UUID, customVarCode)
    {}
    ~AtlasScientificpH_pH(){}
};

#endif  // Header Guard
