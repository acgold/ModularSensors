/*
 * AtlasScientificCO2.h
 * This file is part of the EnviroDIY modular sensors library for Arduino
 *
 * Initial developement for Atlas Sensors was done by Adam Gold
 * Files were edited by Sara Damiano
 *
 * The output from the Atlas Scientifc CO2 is the temperature in degrees C.
 *     Accuracy is ± 3% or ± 30 ppm
 *     Range is 0 − 10000 ppm
 *     Resolution is 1 ppm
 */

// Header Guards
#ifndef AtlasScientificCO2_h
#define AtlasScientificCO2_h

// Debugging Statement
// #define DEBUGGING_SERIAL_OUTPUT Serial

// Included Dependencies
#include "VariableBase.h"
#include "sensors/AtlasParent.h"

// I2C address
#define ATLAS_CO2_I2C_ADDR 0x69  // 105

// Sensor Specific Defines
#define ATLAS_CO2_NUM_VARIABLES 2
// NOTE:  This has a long warm up!
#define ATLAS_CO2_WARM_UP_TIME_MS 0
#define ATLAS_CO2_STABILIZATION_TIME_MS 10000
#define ATLAS_CO2_MEASUREMENT_TIME_MS 900

#define ATLAS_CO2_RESOLUTION 1
#define ATLAS_CO2_VAR_NUM 0

#define ATLAS_CO2TEMP_RESOLUTION 0
#define ATLAS_CO2TEMP_VAR_NUM 1

// The main class for the Atlas Scientific CO2 temperature sensor
class AtlasScientificCO2 : public AtlasParent
{
public:
    AtlasScientificCO2(int8_t powerPin, uint8_t i2cAddressHex = ATLAS_CO2_I2C_ADDR,
                       uint8_t measurementsToAverage = 1);
    ~AtlasScientificCO2();

    virtual bool setup(void) override;
};

// The class for the CO2 Concentration Variable
class AtlasScientificCO2_CO2 : public Variable
{
public:
    AtlasScientificCO2_CO2(Sensor *parentSense,
                        const char *UUID = "", const char *customVarCode = "")
      : Variable(parentSense, ATLAS_CO2_VAR_NUM,
                 "carbonDioxide", "partPerMillion",
                 ATLAS_CO2_RESOLUTION,
                 "AtlasCO2ppm", UUID, customVarCode)
    {}
    ~AtlasScientificCO2_CO2(){}
};

// The class for the Temp Variable
class AtlasScientificCO2_Temp : public Variable
{
public:
    AtlasScientificCO2_Temp(Sensor *parentSense,
                        const char *UUID = "", const char *customVarCode = "")
      : Variable(parentSense, ATLAS_CO2TEMP_VAR_NUM,
                 "temperature", "degreeCelsius",
                 ATLAS_CO2TEMP_RESOLUTION,
                 "AtlasCO2Temp", UUID, customVarCode)
    {}
    ~AtlasScientificCO2_Temp(){}
};

#endif  // Header Guard
