/*****************************************************************************
logging_to_thingSpeak.ino
Written By:  Sara Damiano (sdamiano@stroudcenter.org)
Development Environment: PlatformIO
Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
Software License: BSD-3.
  Copyright (c) 2017, Stroud Water Research Center (SWRC)
  and the EnviroDIY Development Team

This example sketch is written for ModularSensors library version 0.20.0

This sketch is an example of logging data to an SD card and sending the data to
ThingSpeak.

DISCLAIMER:
THIS CODE IS PROVIDED "AS IS" - NO WARRANTY IS GIVEN.
*****************************************************************************/

// ==========================================================================
//    Include the base required libraries
// ==========================================================================
#include <Arduino.h>  // The base Arduino library
#include <EnableInterrupt.h>  // for external and pin change interrupts


// ==========================================================================
//    Data Logger Settings
// ==========================================================================
// The library version this example was written for
const char *libraryVersion = "0.20.0";
// The name of this file
const char *sketchName = "logging_to_ThingSpeak.ino";
// Logger ID, also becomes the prefix for the name of the data file on SD card
const char *LoggerID = "XXXXX";
// How frequently (in minutes) to log data
const uint8_t loggingInterval = 5;
// Your logger's timezone.
const int8_t timeZone = -5;  // Eastern Standard Time
// NOTE:  Daylight savings time will not be applied!  Please use standard time!


// ==========================================================================
//    Primary Arduino-Based Board and Processor
// ==========================================================================
#include <sensors/ProcessorStats.h>

const long serialBaud = 115200;   // Baud rate for the primary serial port for debugging
const int8_t greenLED = 8;        // MCU pin for the green LED (-1 if not applicable)
const int8_t redLED = 9;          // MCU pin for the red LED (-1 if not applicable)
const int8_t buttonPin = 21;      // MCU pin for a button to use to enter debugging mode  (-1 if not applicable)
const int8_t wakePin = A7;        // MCU interrupt/alarm pin to wake from sleep
// Set the wake pin to -1 if you do not want the main processor to sleep.
// In a SAMD system where you are using the built-in rtc, set wakePin to 1
const int8_t sdCardPin = 12;      // MCU SD card chip select/slave select pin (must be given!)
const int8_t sensorPowerPin = 22; // MCU pin controlling main sensor power (-1 if not applicable)

// Create and return the main processor chip "sensor" - for general metadata
const char *mcuBoardVersion = "v0.5b";
ProcessorStats mcuBoard(mcuBoardVersion);


// ==========================================================================
//    Wifi/Cellular Modem Main Chip Selection
// ==========================================================================

// Select your modem chip - this determines the exact commands sent to it
#define TINY_GSM_MODEM_ESP8266  // Select for an ESP8266 using the DEFAULT AT COMMAND FIRMWARE


// ==========================================================================
//    Modem Pins
// ==========================================================================

const int8_t modemVccPin = -2;       // MCU pin controlling modem power (-1 if not applicable)
const int8_t modemSleepRqPin = 19;   // MCU pin used for wake from light sleep (-1 if not applicable)
const int8_t modemStatusPin = -1;    // MCU pin used to read modem status (-1 if not applicable)
const int8_t modemResetPin = A4;     // MCU pin connected to ESP8266's RSTB/GPIO16 pin (-1 if unconnected)


// ==========================================================================
//    TinyGSM Client
// ==========================================================================

// Include TinyGSM for the modem
// This include must be included below the define of the modem name!
#include <TinyGsmClient.h>

// Create a reference to the serial port for the modem
HardwareSerial &modemSerial = Serial1;  // Use hardware serial if possible

// Create a new TinyGSM modem to run on that serial port and return a pointer to it
TinyGsm *tinyModem = new TinyGsm(modemSerial);

// Create a new TCP client on that modem and return a pointer to it
TinyGsmClient *tinyClient = new TinyGsmClient(*tinyModem);


// ==========================================================================
//    Specific Modem On-Off Methods
// ==========================================================================

// This should work with most ESP8266 breakouts
// Describe the physical pin connection of your modem to your board
const long ModemBaud = 115200;       // Communication speed of the modem, 115200 is default for ESP8266
const bool modemStatusLevel = HIGH;  // The level of the status pin when the module is active (HIGH or LOW)
const int8_t espSleepRqPin = 13;     // ESP8266 GPIO pin used for wake from light sleep (-1 if not applicable)
const int8_t espStatusPin = -1;      // ESP8266 GPIO pin used to give modem status (-1 if not applicable)

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool modemSleepFxn(void)
{
    // Use this if you have GPIO16 connected to the reset pin to wake from deep sleep
    // but no other MCU pin connected to the reset pin.
    // NOTE:  This will NOT work nicely with "testingMode"
    /*if (loggingInterval > 1)
    {
        uint32_t sleepSeconds = (((uint32_t)loggingInterval) * 60 * 1000) - 75000L;
        String sleepCommand = String(sleepSeconds);
        tinyModem->sendAT(F("+GSLP="), sleepCommand);
        // Power down for 1 minute less than logging interval
        // Better:  Calculate length of loop and power down for logging interval - loop time
        return tinyModem->waitResponse() == 1;
    }*/
    // Use this if you have an MCU pin connected to the ESP's reset pin to wake from deep sleep
    if (modemResetPin >= 0)
    {
        digitalWrite(redLED, LOW);
        return tinyModem->poweroff();
    }
    // Use this if you don't have access to the ESP8266's reset pin for deep sleep but you
    // do have access to another GPIO pin for light sleep.  This also sets up another
    // pin to view the sleep status.
    else if (modemSleepRqPin >= 0 && modemStatusPin >= 0)
    {
        tinyModem->sendAT(F("+WAKEUPGPIO=1,"), String(espSleepRqPin), F(",0,"),
                          String(espStatusPin), ',', modemStatusLevel);
        bool success = tinyModem->waitResponse() == 1;
        tinyModem->sendAT(F("+SLEEP=1"));
        success &= tinyModem->waitResponse() == 1;
        digitalWrite(redLED, LOW);
        return success;
    }
    // Light sleep without the status pin
    else if (modemSleepRqPin >= 0 && modemStatusPin < 0)
    {
        tinyModem->sendAT(F("+WAKEUPGPIO=1,"), String(espSleepRqPin), F(",0"));
        bool success = tinyModem->waitResponse() == 1;
        tinyModem->sendAT(F("+SLEEP=1"));
        success &= tinyModem->waitResponse() == 1;
        digitalWrite(redLED, LOW);
        return success;
    }
    else return true;  // DON'T go to sleep if we can't wake up!
}
bool modemWakeFxn(void)
{
    if (modemVccPin >= 0)  // Turns on when power is applied
    {
        digitalWrite(redLED, HIGH);  // Because the ESP8266 doesn't have any lights
        return true;
    }
    else if (modemResetPin >= 0)
    {
        digitalWrite(modemResetPin, LOW);
        delay(1);
        digitalWrite(modemResetPin, HIGH);
        digitalWrite(redLED, HIGH);
        return true;
    }
    else if (modemSleepRqPin >= 0)
    {
        digitalWrite(modemSleepRqPin, LOW);
        delay(1);
        digitalWrite(modemSleepRqPin, HIGH);
        digitalWrite(redLED, HIGH);
        return true;
    }
    else return true;
}


// ==========================================================================
//    Network Information and LoggerModem Object
// ==========================================================================
#include <LoggerModem.h>

// Network connection information
const char *apn = "xxxxx";  // The APN for the gprs connection, unnecessary for WiFi
const char *wifiId = "xxxxx";  // The WiFi access point, unnecessary for gprs
const char *wifiPwd = "xxxxx";  // The password for connecting to WiFi, unnecessary for gprs

// Create the loggerModem instance
// A "loggerModem" is a combination of a TinyGSM Modem, a Client, and functions for wake and sleep
loggerModem modem(modemVccPin, modemStatusPin, modemStatusLevel, modemWakeFxn, modemSleepFxn, tinyModem, tinyClient, wifiId, wifiPwd);


// ==========================================================================
//    Maxim DS3231 RTC (Real Time Clock)
// ==========================================================================
#include <sensors/MaximDS3231.h>

// Create and return the DS3231 sensor object
MaximDS3231 ds3231(1);


// ==========================================================================
//    CAMPBELL OBS 3 / OBS 3+ Analog Turbidity Sensor
// ==========================================================================
#include <sensors/CampbellOBS3.h>

const int8_t OBS3Power = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const uint8_t OBS3numberReadings = 10;
const uint8_t ADSi2c_addr = 0x48;  // The I2C address of the ADS1115 ADC
// Campbell OBS 3+ Low Range calibration in Volts
const int8_t OBSLowADSChannel = 0;  // The ADS channel for the low range output
const float OBSLow_A = 0.000E+00;  // The "A" value (X^2) from the low range calibration
const float OBSLow_B = 1.000E+00;  // The "B" value (X) from the low range calibration
const float OBSLow_C = 0.000E+00;  // The "C" value from the low range calibration
// Create and return the Campbell OBS3+ LOW RANGE sensor object
CampbellOBS3 osb3low(OBS3Power, OBSLowADSChannel, OBSLow_A, OBSLow_B, OBSLow_C, ADSi2c_addr, OBS3numberReadings);


// Campbell OBS 3+ High Range calibration in Volts
const int8_t OBSHighADSChannel = 1;  // The ADS channel for the high range output
const float OBSHigh_A = 0.000E+00;  // The "A" value (X^2) from the high range calibration
const float OBSHigh_B = 1.000E+00;  // The "B" value (X) from the high range calibration
const float OBSHigh_C = 0.000E+00;  // The "C" value from the high range calibration
// Create and return the Campbell OBS3+ HIGH RANGE sensor object
CampbellOBS3 osb3high(OBS3Power, OBSHighADSChannel, OBSHigh_A, OBSHigh_B, OBSHigh_C, ADSi2c_addr, OBS3numberReadings);


// ==========================================================================
//    Decagon CTD Conductivity, Temperature, and Depth Sensor
// ==========================================================================
#include <sensors/DecagonCTD.h>

const char *CTDSDI12address = "1";  // The SDI-12 Address of the CTD
const uint8_t CTDnumberReadings = 6;  // The number of readings to average
const int8_t SDI12Power = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const int8_t SDI12Data = 7;  // The SDI12 data pin

// Create and return the Decagon CTD sensor object
DecagonCTD ctd(*CTDSDI12address, SDI12Power, SDI12Data, CTDnumberReadings);


// ==========================================================================
//    Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
#include <VariableArray.h>

// FORM1: Create pointers for all of the variables from the sensors,
// at the same time putting them into an array
Variable *variableList[] = {
    new DecagonCTD_Cond(&ctd, "12345678-abcd-1234-efgh-1234567890ab"),
    new DecagonCTD_Temp(&ctd, "12345678-abcd-1234-efgh-1234567890ab"),
    new DecagonCTD_Depth(&ctd, "12345678-abcd-1234-efgh-1234567890ab"),
    new CampbellOBS3_Turbidity(&osb3low, "12345678-abcd-1234-efgh-1234567890ab", "TurbLow"),
    new CampbellOBS3_Turbidity(&osb3high, "12345678-abcd-1234-efgh-1234567890ab", "TurbHigh"),
    new ProcessorStats_Batt(&mcuBoard, "12345678-abcd-1234-efgh-1234567890ab"),
    new MaximDS3231_Temp(&ds3231, "12345678-abcd-1234-efgh-1234567890ab"),
    new Modem_RSSI(&modem, "12345678-abcd-1234-efgh-1234567890ab")
};
// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);

// Create the VariableArray object
VariableArray varArray(variableCount, variableList);


// ==========================================================================
//     The Logger Object[s]
// ==========================================================================
#include <LoggerBase.h>

// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval, sdCardPin, wakePin, &varArray);


// ==========================================================================
//    ThingSpeak Data Publisher
// ==========================================================================
// Create a channel with fields on ThingSpeak in advance
// The fields will be sent in exactly the order they are in the variable array.
// Any custom name or identifier given to the field on ThingSpeak is irrelevant.
// No more than 8 fields of data can go to any one channel.  Any fields beyond the
// eighth in the array will be ignored.
const char *thingSpeakMQTTKey = "XXXXXXXXXXXXXXXX";  // Your MQTT API Key from Account > MyProfile.
const char *thingSpeakChannelID = "######";  // The numeric channel id for your channel
const char *thingSpeakChannelKey = "XXXXXXXXXXXXXXXX";  // The Write API Key for your channel

// Create a data publisher for ThingSpeak
#include <publishers/ThingSpeakPublisher.h>
ThingSpeakPublisher TsMqtt(dataLogger, thingSpeakMQTTKey, thingSpeakChannelID, thingSpeakChannelKey);


// ==========================================================================
//    Working Functions
// ==========================================================================

// Flashes the LED's on the primary board
void greenredflash(uint8_t numFlash = 4, uint8_t rate = 75)
{
  for (uint8_t i = 0; i < numFlash; i++) {
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, LOW);
    delay(rate);
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, HIGH);
    delay(rate);
  }
  digitalWrite(redLED, LOW);
}


// Read's the battery voltage
// NOTE: This will actually return the battery level from the previous update!
float getBatteryVoltage()
{
    if (mcuBoard.sensorValues[0] == -9999) mcuBoard.update();
    return mcuBoard.sensorValues[0];
}


// ==========================================================================
// Main setup function
// ==========================================================================
void setup()
{
    // Start the primary serial connection
    Serial.begin(serialBaud);

    // Print a start-up note to the first serial port
    Serial.print(F("Now running "));
    Serial.print(sketchName);
    Serial.print(F(" on Logger "));
    Serial.println(LoggerID);
    Serial.println();

    Serial.print(F("Using ModularSensors Library version "));
    Serial.println(MODULAR_SENSORS_VERSION);

    if (String(MODULAR_SENSORS_VERSION) !=  String(libraryVersion))
        Serial.println(F(
            "WARNING: THIS EXAMPLE WAS WRITTEN FOR A DIFFERENT VERSION OF MODULAR SENSORS!!"));

    // Start the serial connection with the modem
    modemSerial.begin(ModemBaud);

    // Set up pins for the LED's
    pinMode(greenLED, OUTPUT);
    digitalWrite(greenLED, LOW);
    pinMode(redLED, OUTPUT);
    digitalWrite(redLED, LOW);
    // Blink the LEDs to show the board is on and starting up
    greenredflash();

    // Set up some of the power pins so the board boots up with them off
    // NOTE:  This isn't necessary at all.  The logger begin() function
    // should leave all power pins off when it finishes.
    if (modemVccPin >= 0)
    {
        pinMode(modemVccPin, OUTPUT);
        digitalWrite(modemVccPin, LOW);
    }
    if (sensorPowerPin >= 0)
    {
        pinMode(sensorPowerPin, OUTPUT);
        digitalWrite(sensorPowerPin, LOW);
    }
    if (modemSleepRqPin >= 0)
    {
        pinMode(modemSleepRqPin, OUTPUT);
        digitalWrite(modemSleepRqPin, HIGH);
    }
    if (modemResetPin >= 0)
    {
        pinMode(modemResetPin, OUTPUT);
        digitalWrite(modemResetPin, HIGH);
    }

    // Set the timezone and offsets
    // Logging in the given time zone
    Logger::setTimeZone(timeZone);
    // Offset is the same as the time zone because the RTC is in UTC
    Logger::setTZOffset(timeZone);

    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modem);
    dataLogger.setAlertPin(greenLED);
    dataLogger.setTestingModePin(buttonPin);

    // Begin the logger
    // Note:  Please change these battery voltages to match your battery
    // Check that the battery is OK before powering the modem
    if (getBatteryVoltage() > 3.7) modem.modemPowerUp();
    // At lowest battery level, skip sensor set-up
    if (getBatteryVoltage() < 3.4) dataLogger.begin(true);
    else dataLogger.begin();  // set up file and sensors

    // At very good battery voltage, or with suspicious time stamp, sync the clock
    // Note:  Please change these battery voltages to match your battery
    if (getBatteryVoltage() > 3.8 ||
        dataLogger.getNowEpoch() < 1546300800 ||  /*Before 01/01/2019*/
        dataLogger.getNowEpoch() > 1735689600)  /*Before 1/1/2025*/
    {
        dataLogger.syncRTC();  // There's a sleepPowerDown at the end of this
}
    else modem.modemSleepPowerDown();

    // Call the processor sleep
    dataLogger.systemSleep();
}


// ==========================================================================
// Main loop function
// ==========================================================================

// Use this short loop for simple data logging and sending
void loop()
{
    // Note:  Please change these battery voltages to match your battery
    // At very low battery, just go back to sleep
    if (getBatteryVoltage() < 3.4) dataLogger.systemSleep();
    // At moderate voltage, log data but don't send it over the modem
    else if (getBatteryVoltage() < 3.7) dataLogger.logData();
    // If the battery is good, send the data to the world
    else dataLogger.logDataAndSend();
}
