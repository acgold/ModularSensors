/*****************************************************************************
menu_a_la_carte.ino
Written By:  Sara Damiano (sdamiano@stroudcenter.org)
Development Environment: PlatformIO
Hardware Platform: EnviroDIY Mayfly Arduino Datalogger
Software License: BSD-3.
  Copyright (c) 2017, Stroud Water Research Center (SWRC)
  and the EnviroDIY Development Team

This example sketch is written for ModularSensors library version 0.21.2

This shows most of the standard functions of the library at once.

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
const char *libraryVersion = "0.21.2";
// The name of this file
const char *sketchName = "menu_a_la_carte.ino";
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
const int8_t sdCardPwrPin = -1;     // MCU SD card power pin (-1 if not applicable)
const int8_t sdCardSSPin = 12;      // MCU SD card chip select/slave select pin (must be given!)
const int8_t sensorPowerPin = 22;  // MCU pin controlling main sensor power (-1 if not applicable)

// Create the main processor chip "sensor" - for general metadata
const char *mcuBoardVersion = "v0.5b";
ProcessorStats mcuBoard(mcuBoardVersion);

// Create sample number, battery voltage, and free RAM variable pointers for the processor
// Variable *mcuBoardBatt = new ProcessorStats_Batt(&mcuBoard, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *mcuBoardAvailableRAM = new ProcessorStats_FreeRam(&mcuBoard, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *mcuBoardSampNo = new ProcessorStats_SampleNumber(&mcuBoard, "12345678-abcd-1234-efgh-1234567890ab");


// ==========================================================================
//    Settings for Additional Serial Ports
// ==========================================================================

// The modem and a number of sensors communicate over UART/TTL - often called "serial".
// "Hardware" serial ports (automatically controlled by the MCU) are generally
// the most accurate and should be configured and used for as many peripherals
// as possible.  In some cases (ie, modbus communication) many sensors can share
// the same serial port.

#if not defined ARDUINO_ARCH_SAMD && not defined ATMEGA2560  // For AVR boards
// Unfortunately, most AVR boards have only one or two hardware serial ports,
// so we'll set up three types of extra software serial ports to use

// AltSoftSerial by Paul Stoffregen (https://github.com/PaulStoffregen/AltSoftSerial)
// is the most accurate software serial port for AVR boards.
// AltSoftSerial can only be used on one set of pins on each board so only one
// AltSoftSerial port can be used.
// Not all AVR boards are supported by AltSoftSerial.
#include <AltSoftSerial.h>
AltSoftSerial altSoftSerial;

// NeoSWSerial (https://github.com/SRGDamia1/NeoSWSerial) is the best software
// serial that can be used on any pin supporting interrupts.
// You can use as many instances of NeoSWSerial as you want.
// Not all AVR boards are supported by NeoSWSerial.
#include <NeoSWSerial.h>  // for the stream communication
const int8_t neoSSerial1Rx = 11;     // data in pin
const int8_t neoSSerial1Tx = -1;     // data out pin
NeoSWSerial neoSSerial1(neoSSerial1Rx, neoSSerial1Tx);
// To use NeoSWSerial in this library, we define a function to receive data
// This is just a short-cut for later
void neoSSerial1ISR()
{
    NeoSWSerial::rxISR(*portInputRegister(digitalPinToPort(neoSSerial1Rx)));
}

// The "standard" software serial library uses interrupts that conflict
// with several other libraries used within this program, we must use a
// version of software serial that has been stripped of interrupts.
// NOTE:  Only use if necessary.  This is not a very accurate serial port!
const int8_t softSerialRx = A3;     // data in pin
const int8_t softSerialTx = A4;     // data out pin

#include <SoftwareSerial_ExtInts.h>  // for the stream communication
SoftwareSerial_ExtInts softSerial1(softSerialRx, softSerialTx);
#endif  // End software serial for avr boards


// The SAMD21 has 6 "SERCOM" ports, any of which can be used for UART communication.
// The "core" code for most boards defines one or more UART (Serial) ports with
// the SERCOMs and uses others for I2C and SPI.  We can create new UART ports on
// any available SERCOM.  The table below shows definitions for select boards.

// Board =>   Arduino Zero       Adafruit Feather    Sodaq Boards
// -------    ---------------    ----------------    ----------------
// SERCOM0    Serial1 (D0/D1)    Serial1 (D0/D1)     Serial (D0/D1)
// SERCOM1    Available          Available           Serial3 (D12/D13)
// SERCOM2    Available          Available           I2C (A4/A5)
// SERCOM3    I2C (D20/D21)      I2C (D20/D21)       SPI (D11/12/13)
// SERCOM4    SPI (D21/22/23)    SPI (D21/22/23)     SPI1/Serial2
// SERCOM5    EDBG/Serial        Available           Serial1

// If using a Sodaq board, do not define the new sercoms, instead:
// #define ENABLE_SERIAL2
// #define ENABLE_SERIAL3


#if defined ARDUINO_ARCH_SAMD
#include <wiring_private.h> // Needed for SAMD pinPeripheral() function

#ifndef ENABLE_SERIAL2
// Set up a 'new' UART using SERCOM1
// The Rx will be on digital pin 11, which is SERCOM1's Pad #0
// The Tx will be on digital pin 10, which is SERCOM1's Pad #2
// NOTE:  SERCOM1 is undefinied on a "standard" Arduino Zero and many clones,
//        but not all!  Please check the variant.cpp file for you individual board!
//        Sodaq Autonomo's and Sodaq One's do NOT follow the 'standard' SERCOM definitions!
Uart Serial2(&sercom1, 11, 10, SERCOM_RX_PAD_0, UART_TX_PAD_2);
// Hand over the interrupts to the sercom port
void SERCOM1_Handler()
{
    Serial2.IrqHandler();
}
#endif

#ifndef ENABLE_SERIAL3
// Set up a 'new' UART using SERCOM2
// The Rx will be on digital pin 5, which is SERCOM2's Pad #3
// The Tx will be on digital pin 2, which is SERCOM2's Pad #2
// NOTE:  SERCOM2 is undefinied on a "standard" Arduino Zero and many clones,
//        but not all!  Please check the variant.cpp file for you individual board!
//        Sodaq Autonomo's and Sodaq One's do NOT follow the 'standard' SERCOM definitions!
Uart Serial3(&sercom2, 5, 2, SERCOM_RX_PAD_3, UART_TX_PAD_2);
// Hand over the interrupts to the sercom port
void SERCOM2_Handler()
{
    Serial3.IrqHandler();
}
#endif

#endif  // End hardware serial on SAMD21 boards


// ==========================================================================
//    Wifi/Cellular Modem Main Chip Selection
// ==========================================================================

// Select your modem chip - this determines the exact commands sent to it
// #define TINY_GSM_MODEM_SIM800  // Select for a SIMCOM SIM800, SIM900, or variant thereof
// #define SIM800_GPRSBEE_R6  // Select with SIM800 for - these use atypical sleep and wake fxns
// #define TINY_GSM_MODEM_SIM808  // Select for a SIMCOM SIM808 or SIM868, or variant thereof
// #define TINY_GSM_MODEM_UBLOX  // Select for most u-blox cellular modems
// #define USE_UBLOX_R410M  // Select with UBLOX for a non-XBee SARA R4 or N4 model
// #define USE_XBEE_BYPASS  // Select with UBLOX for a Digi 3G or LTE-M XBee in bypass mode
// #define TINY_GSM_MODEM_ESP8266  // Select for an ESP8266 using the DEFAULT AT COMMAND FIRMWARE
#define TINY_GSM_MODEM_XBEE  // Select for Digi brand WiFi or Cellular XBee's
// #define USE_XBEE_WIFI  // Select with XBEE for an S6B wifi XBee
// #define TINY_GSM_MODEM_M590  // Select for a Neoway M590
// #define TINY_GSM_MODEM_A6  // Select for an AI-Thinker A6, A6C, A7, A20
// #define TINY_GSM_MODEM_M95  // Select for a Quectel M95
// #define TINY_GSM_MODEM_BG96  // Select for a Quectel BG96
// #define TINY_GSM_MODEM_MC60  // Select for a Quectel MC60 or MC60E


// ==========================================================================
//    Modem Pins
// ==========================================================================

const int8_t modemVccPin = -2;      // MCU pin controlling modem power (-1 if not applicable)
const int8_t modemSleepRqPin = 23;  // MCU pin used for modem sleep/wake request (-1 if not applicable)
const int8_t modemStatusPin = 19;   // MCU pin used to read modem status (-1 if not applicable)
const int8_t modemResetPin = A4;    // MCU pin connected to modem reset pin (-1 if unconnected)


// ==========================================================================
//    TinyGSM Client
// ==========================================================================

// #define TINY_GSM_DEBUG Serial  // If you want debugging on the main debug port

#define TINY_GSM_YIELD() { delay(2); }  // Can help with slow (9600) baud rates

// Include TinyGSM for the modem
// This include must be included below the define of the modem name!
#include <TinyGsmClient.h>

// Create a reference to the serial port for the modem
// Extra hardware and software serial ports are created in the "Settings for Additional Serial Ports" section
HardwareSerial &modemSerial = Serial1;  // Use hardware serial if possible
// AltSoftSerial &modemSerial = altSoftSerial;  // For software serial if needed
// NeoSWSerial &modemSerial = neoSSerial1;  // For software serial if needed

// Create a new TinyGSM modem to run on that serial port and return a pointer to it
#if defined TINY_GSM_MODEM_XBEE
TinyGsm *tinyModem = new TinyGsm(modemSerial, modemResetPin);
#else
TinyGsm *tinyModem = new TinyGsm(modemSerial);
#endif

// Use this to create a modem if you want to spy on modem communication through
// a secondary Arduino stream.  Make sure you install the StreamDebugger library!
// https://github.com/vshymanskyy/StreamDebugger
// Also make sure you comment out the modem creation above to use this.
// #include <StreamDebugger.h>
// StreamDebugger modemDebugger(modemSerial, Serial);
// #if defined TINY_GSM_MODEM_XBEE
// TinyGsm *tinyModem = new TinyGsm(modemDebugger, modemResetPin);
// #else
// TinyGsm *tinyModem = new TinyGsm(modemDebugger);
// #endif

// Create a new TCP client on that modem and return a pointer to it
TinyGsmClient *tinyClient = new TinyGsmClient(*tinyModem);
// The u-blox SARA R410 is very slow to open and close clients, so we can
// iterate through mutiple data senders much more quickly if we have multiple
// clients.  The u-blox SARA R410 is the only modem where there's any advantage
// to this.
#if defined USE_UBLOX_R410M
TinyGsmClient *tinyClient2 = new TinyGsmClient(*tinyModem);
TinyGsmClient *tinyClient3 = new TinyGsmClient(*tinyModem);
#endif


// ==========================================================================
//    Specific Modem On-Off Methods
// ==========================================================================

// This should apply to all Digi brand XBee modules.
#if defined TINY_GSM_MODEM_XBEE || defined USE_XBEE_BYPASS
// Describe the physical pin connection of your modem to your board
const long modemBaud = 9600;        // Communication speed of the modem, 9600 is default for XBee
const bool modemStatusLevel = LOW;  // The level of the status pin when the module is active (HIGH or LOW)

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
// After enabling pin sleep, the sleep request pin is held LOW to keep the XBee on
// Enable pin sleep in the setup function or using XCTU prior to connecting the XBee
bool modemSleepFxn(void)
{
    if (modemSleepRqPin >= 0)  // Don't go to sleep if there's not a wake pin!
    {
        digitalWrite(modemSleepRqPin, HIGH);
        digitalWrite(redLED, LOW);
        return true;
    }
    else
    {
        return true;
    }
}
bool modemWakeFxn(void)
{
    if (modemVccPin >= 0)  // Turns on when power is applied
        return true;
    else if (modemSleepRqPin >= 0)
    {
        digitalWrite(modemSleepRqPin, LOW);
        digitalWrite(redLED, HIGH);  // Because the XBee doesn't have any lights
        return true;
    }
    else
    {
        return true;
    }
}
// An extra function to set up pin sleep and other preferences on the XBee
// NOTE:  This will only succeed if the modem is turned on and awake!
#if defined TINY_GSM_MODEM_XBEE
void extraModemSetup(void)
{
    tinyModem->init();  // initialize
    if (tinyModem->commandMode())
    {
        tinyModem->sendAT(F("D8"),1);  // Set DIO8 to be used for sleep requests
        // NOTE:  Only pin 9/DIO8/DTR can be used for this function
        tinyModem->waitResponse();
        tinyModem->sendAT(F("D9"),1);  // Turn on status indication pin
        // NOTE:  Only pin 13/DIO9 can be used for this function
        tinyModem->waitResponse();
        tinyModem->sendAT(F("D7"),1);  // Turn on CTS pin - as proxy for status indication
        // NOTE:  Only pin 12/DIO7/CTS can be used for this function
        tinyModem->waitResponse();
        tinyModem->sendAT(F("SM"),1);  // Pin sleep
        tinyModem->waitResponse();
        tinyModem->sendAT(F("DO"),0);  // Disable remote manager, USB Direct, and LTE PSM
        // NOTE:  LTE-M's PSM (Power Save Mode) sounds good, but there's no
        // easy way on the LTE-M Bee to wake the cell chip itself from PSM,
        // so we'll use the Digi pin sleep instead.
        tinyModem->waitResponse();
        #if defined USE_XBEE_WIFI
        tinyModem->sendAT(F("SO"),200);  // For WiFi - Disassociate from AP for Deep Sleep
        tinyModem->waitResponse();
        #else
        tinyModem->sendAT(F("SO"),0);  // For Cellular - disconnected sleep
        tinyModem->waitResponse();
        tinyModem->sendAT(F("P0"),0);  // Make sure USB direct won't be pin enabled
        tinyModem->waitResponse();
        tinyModem->sendAT(F("P1"),0);  // Make sure pins 7&8 are not set for USB direct
        tinyModem->waitResponse();
        tinyModem->sendAT(F("N#"),2);  // Cellular network technology - LTE-M Only
        // LTE-M XBee connects much faster on AT&T/Hologram when set to LTE-M only (instead of LTE-M/NB IoT)
        tinyModem->waitResponse();
        #endif
        tinyModem->writeChanges();
        tinyModem->exitCommand();
    }
}
#elif defined USE_XBEE_BYPASS
void extraModemSetup(void)
{
    delay(1000);  // Guard time for command mode
    tinyModem->streamWrite(GF("+++"));  // enter command mode
    tinyModem->waitResponse(2000, F("OK\r"));
    tinyModem->sendAT(F("D8"),1);  // Set DIO8 to be used for sleep requests
    // NOTE:  Only pin 9/DIO8/DTR can be used for this function
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("D9"),1);  // Turn on status indication pin
    // NOTE:  Only pin 13/DIO9 can be used for this function
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("D7"),1);  // Turn on CTS pin - as proxy for status indication
    // NOTE:  Only pin 12/DIO7/CTS can be used for this function
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("SM"),1);  // Pin sleep
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("SO"),0);  // For Cellular - disconnected sleep
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("DO"),0);  // Disable remote manager, USB Direct, and LTE PSM
    // NOTE:  LTE-M's PSM (Power Save Mode) sounds good, but there's no
    // easy way on the LTE-M Bee to wake the cell chip itself from PSM,
    // so we'll use the Digi pin sleep instead.
    tinyModem->waitResponse(F("OK\r"));
    #if defined USE_UBLOX_R410M
    tinyModem->sendAT(F("P0"),0);  // Make sure USB direct won't be pin enabled
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("P1"),0);  // Make sure pins 7&8 are not set for USB direct
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("N#"),2);  // Cellular network technology - LTE-M Only
    // LTE-M XBee connects much faster on AT&T/Hologram when set to LTE-M only (instead of LTE-M/NB IoT)
    tinyModem->waitResponse(F("OK\r"));
    #endif
    tinyModem->sendAT(F("AP5"));  // Turn on bypass mode
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("WR"));  // Write changes to flash
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("AC"));  // Apply changes
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->sendAT(F("FR"));  // Force reset to enter bypass mode
    tinyModem->waitResponse(F("OK\r"));
    tinyModem->init();  // initialize
}
#endif


// This should work with most ESP8266 breakouts
#elif defined TINY_GSM_MODEM_ESP8266
// Describe the physical pin connection of your modem to your board
const long modemBaud = 115200;       // Communication speed of the modem, 115200 is default for ESP8266
const bool modemStatusLevel = HIGH;  // The level of the status pin when the module is active (HIGH or LOW)
const int8_t espSleepRqPin = 13;     // ESP8266 GPIO pin used for wake from light sleep (-1 if not applicable)
const int8_t espStatusPin = -1;      // ESP8266 GPIO pin used to give modem status (-1 if not applicable)

// A helper function to wait for the esp to boot and immediately change some settings
// We'll use this in the wake function
bool ESPwaitForBoot(void)
{
    // Wait for boot - finished when characters start coming
    // NOTE: After every "hard" reset (either power off or via RST-B), the ESP
    // sends out a boot log from the ROM on UART1 at 74880 baud.  We're not
    // going to worry about the odd baud rate since we're simply throwing the
    // characters away.
    delay(200);  // It will take at least this long
    uint32_t start = millis();
    bool success = false;
    while (!modemSerial.available() && millis() - start < 1000) {}
    if (modemSerial.available())
    {
        success = true;
        // Read the boot log to empty it from the serial buffer
        while (modemSerial.available())
        {
            modemSerial.read();
            delay(2);
        }
        // Have to make sure echo is off or all AT commands will be confused
        tinyModem->sendAT(F("E0"));
        success &= tinyModem->waitResponse() == 1;
        // re-run init to set mux and station mode
        success &= tinyModem->init();
    }
    return success;
}

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
    else  // DON'T go to sleep if we can't wake up!
    {
        return true;
    }
}
bool modemWakeFxn(void)
{
    bool success = true;
    if (modemVccPin >= 0)  // Turns on when power is applied
    {
        digitalWrite(redLED, HIGH);  // Because the ESP8266 doesn't have any lights
        success &= ESPwaitForBoot();
        if (!success)
        {
            digitalWrite(redLED, LOW);  // Turn off light if the boot failed
        }
        return success;
    }
    else if (modemResetPin >= 0)
    {
        digitalWrite(redLED, HIGH);
        digitalWrite(modemResetPin, LOW);
        delay(1);
        digitalWrite(modemResetPin, HIGH);
        success &= ESPwaitForBoot();
        if (!success)
        {
            digitalWrite(redLED, LOW);  // Turn off light if the boot failed
        }
        return success;
    }
    else if (modemSleepRqPin >= 0)
    {
        digitalWrite(modemSleepRqPin, LOW);
        delay(1);
        digitalWrite(modemSleepRqPin, HIGH);
        digitalWrite(redLED, HIGH);
        // Don't have to wait for a boot if using light sleep
        return true;
    }
    else
    {
        return true;
    }
}
// Set up the light-sleep status pin, if applicable
void extraModemSetup(void)
{
    // Slow down the baud rate for slow processors - and save the change to
    // the ESP's non-volatile memory so we don't have to do it every time
    #if F_CPU == 8000000L
    if (modemBaud > 57600)
    {
        modemSerial.begin(modemBaud);
        tinyModem->sendAT(F("+UART_DEF=9600,8,1,0,0"));
        tinyModem->waitResponse();
        modemSerial.end();
        modemSerial.begin(9600);
    }
    #endif
    if (modemVccPin < 0 && modemResetPin < 0 && modemSleepRqPin >= 0 && modemStatusPin >= 0)
    {
        tinyModem->sendAT(F("+WAKEUPGPIO=1,"), String(espSleepRqPin), F(",0,"),
                          String(espStatusPin), ',', modemStatusLevel);
        tinyModem->waitResponse();
    }
}


// This should work for many u-blox breakouts, but check the timing in wake/sleep functions
#elif defined TINY_GSM_MODEM_UBLOX
// Describe the physical pin connection of your modem to your board
#if defined USE_UBLOX_R410M
const long modemBaud = 115200;       // Communication speed of the modem, R4/N4 use 115200
#else
const long modemBaud = 9600;         // Communication speed of the modem, most u-blox use 9600
#endif
const bool modemStatusLevel = HIGH;  // The level of the status pin when the module is active (HIGH or LOW)

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool modemSleepFxn(void)
{
    #if defined USE_UBLOX_R410M
    if (modemSleepRqPin >= 0)  // R410 must have access to PWR_ON pin to sleep
    #else
    if (modemVccPin >= 0 || modemSleepRqPin >= 0)  // others will go on with power on
    #endif
    {
        // Easiest to just go to sleep with the AT command rather than using pins
        return tinyModem->poweroff();
    }
    else  // DON'T go to sleep if we can't wake up!
    {
        return true;
    }
}
bool modemWakeFxn(void)
{
    // SARA/LISA U2/G2 and SARA G3 series turn on when power is applied
    // SARA R4/N4 series must power on and then pulse on
    #ifndef USE_UBLOX_R410M
    if (modemVccPin >= 0)
        return true;
    #endif
    if (modemSleepRqPin >= 0)
    {
        digitalWrite(modemSleepRqPin, LOW);
        digitalWrite(redLED, HIGH);
        #if defined USE_UBLOX_R410M
        delay(200);  // 0.15-3.2s pulse for wake on SARA R4/N4
        #else
        // delay(6);  // >5ms pulse for wake on SARA G3
        delayMicroseconds(65);  // 50-80µs pulse for wake on SARA/LISA U2/G2
        #endif
        digitalWrite(modemSleepRqPin, HIGH);
        digitalWrite(redLED, LOW);
        // Need to slow down R4/N4's default 115200 baud rate for slow processors
        #if F_CPU == 8000000L && defined USE_UBLOX_R410M
        delay(4600);  // Must wait for UART port to become active
        modemSerial.begin(115200);
        tinyModem->setBaud(9600);
        modemSerial.end();
        modemSerial.begin(9600);
        #endif
        return true;
    }
    else
    {
        return true;
    }
}
#if defined USE_UBLOX_R410M
// Set band to only use LTE-M (not NB IoT) - this can speed up connection
void extraModemSetup(void)
{
    tinyModem->sendAT(F("+URAT=7"));
    tinyModem->waitResponse();
}
#else
void extraModemSetup(void){}
#endif


// THIS ONLY APPLIES TO A SODAQ GPRSBEE R6!!!
#elif defined TINY_GSM_MODEM_SIM800 && defined SIM800_GPRSBEE_R6
// Describe the physical pin connection of your modem to your board
const long modemBaud = 9600;         // Communication speed of the modem
const bool modemStatusLevel = HIGH;  // The level of the status pin when the module is active (HIGH or LOW)

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool modemWakeFxn(void)
{
    digitalWrite(modemSleepRqPin, HIGH);
    digitalWrite(redLED, HIGH);  // A light just for show
    return true;
}
bool modemSleepFxn(void)
{
    digitalWrite(modemSleepRqPin, LOW);
    digitalWrite(redLED, LOW);
    return true;
}
void extraModemSetup(void){}


// Most cellular chips/breakouts respond to a low pulse of some length to power up and down
#else
// Describe the physical pin connection of your modem to your board
const long modemBaud = 9600;         // Communication speed of the modem
const bool modemStatusLevel = HIGH;  // The level of the status pin when the module is active (HIGH or LOW)

// Create the wake and sleep methods for the modem
// These can be functions of any type and must return a boolean
bool modemWakeFxn(void)
{
    digitalWrite(modemSleepRqPin, LOW);
    // delay(1100);  // >1s for SIM800, SIM900, Quectel M95, Quectel MC60
    // delay(510);  // >300ms (>500ms recommended) for Neoway M590
    delay(110);  // >100ms for Quectel BG96
    digitalWrite(modemSleepRqPin, HIGH);
    return true;
}
bool modemSleepFxn(void)
{
    digitalWrite(modemSleepRqPin, LOW);
    // delay(1100);  // 1sec > t > 33sec for SIM800 and clones
    // delay(600);  // 0.5sec > pull down > 1sec for SIM900
    // delay(510);  // >300ms (>500ms recommended) for Neoway M590
    delay(700);  // >650ms for Quectel BG96, 0.6sec > pull down > 1sec for  Quectel M95, Quectel MC60
    digitalWrite(modemSleepRqPin, HIGH);
    return true;
}
void extraModemSetup(void){}
#endif


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
#if defined TINY_GSM_MODEM_ESP8266 || defined USE_XBEE_WIFI
loggerModem modem(modemVccPin, modemStatusPin, modemStatusLevel, modemWakeFxn, modemSleepFxn, tinyModem, tinyClient, wifiId, wifiPwd);
// ^^ Use this for WiFi
#else
loggerModem modem(modemVccPin, modemStatusPin, modemStatusLevel, modemWakeFxn, modemSleepFxn, tinyModem, tinyClient, apn);
// ^^ Use this for cellular
#endif

// Create RSSI and signal strength variable pointers for the modem
// Variable *modemRSSI = new Modem_RSSI(&modem, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *modemSignalPct = new Modem_SignalPercent(&modem, "12345678-abcd-1234-efgh-1234567890ab");


// ==========================================================================
//    Maxim DS3231 RTC (Real Time Clock)
// ==========================================================================
#include <sensors/MaximDS3231.h>

// Create a DS3231 sensor object
MaximDS3231 ds3231(1);

// Create a temperature variable pointer for the DS3231
// Variable *ds3231Temp = new MaximDS3231_Temp(&ds3231, "12345678-abcd-1234-efgh-1234567890ab");


// ==========================================================================
//    Bosch BME280 Environmental Sensor (Temperature, Humidity, Pressure)
// ==========================================================================
#include <sensors/BoschBME280.h>

const int8_t I2CPower = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
uint8_t BMEi2c_addr = 0x76;
// The BME280 can be addressed either as 0x77 (Adafruit default) or 0x76 (Grove default)
// Either can be physically mofidied for the other address

// Create a Bosch BME280 sensor object
BoschBME280 bme280(I2CPower, BMEi2c_addr);

// Create four variable pointers for the BME280
// Variable *bme280Humid = new BoschBME280_Humidity(&bme280, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *bme280Temp = new BoschBME280_Temp(&bme280, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *bme280Press = new BoschBME280_Pressure(&bme280, "12345678-abcd-1234-efgh-1234567890ab");
// Variable *bme280Alt = new BoschBME280_Altitude(&bme280, "12345678-abcd-1234-efgh-1234567890ab");


// ==========================================================================
//    Maxim DS18 One Wire Temperature Sensor
// ==========================================================================
#include <sensors/MaximDS18.h>

// OneWire Address [array of 8 hex characters]
// If only using a single sensor on the OneWire bus, you may omit the address
// DeviceAddress OneWireAddress1 = {0x28, 0xFF, 0xBD, 0xBA, 0x81, 0x16, 0x03, 0x0C};
const int8_t OneWirePower = sensorPowerPin;  // Pin to switch power on and off (-1 if unconnected)
const int8_t OneWireBus = 6;  // Pin attached to the OneWire Bus (-1 if unconnected) (D24 = A0)

// Create a Maxim DS18 sensor objects (use this form for a known address)
// MaximDS18 ds18(OneWireAddress1, OneWirePower, OneWireBus);

// Create a Maxim DS18 sensor object (use this form for a single sensor on bus with an unknown address)
MaximDS18 ds18(OneWirePower, OneWireBus);

// Create a temperature variable pointer for the DS18
// Variable *ds18Temp = new MaximDS18_Temp(&ds18, "12345678-abcd-1234-efgh-1234567890ab");


// ==========================================================================
//    Creating the Variable Array[s] and Filling with Variable Objects
// ==========================================================================
#include <VariableArray.h>

// FORM1: Create pointers for all of the variables from the sensors,
// at the same time putting them into an array
// NOTE:  Forms one and two can be mixed
Variable *variableList[] = {
    new ProcessorStats_SampleNumber(&mcuBoard, "12345678-abcd-1234-efgh-1234567890ab"),
    new BoschBME280_Temp(&bme280, "12345678-abcd-1234-efgh-1234567890ab"),
    new BoschBME280_Humidity(&bme280, "12345678-abcd-1234-efgh-1234567890ab"),
    new BoschBME280_Pressure(&bme280, "12345678-abcd-1234-efgh-1234567890ab"),
    new BoschBME280_Altitude(&bme280, "12345678-abcd-1234-efgh-1234567890ab"),
    new MaximDS18_Temp(&ds18, "12345678-abcd-1234-efgh-1234567890ab"),
    new MaximDS3231_Temp(&ds3231, "12345678-abcd-1234-efgh-1234567890ab"),
    new Modem_RSSI(&modem, "12345678-abcd-1234-efgh-1234567890ab"),
    new Modem_SignalPercent(&modem, "12345678-abcd-1234-efgh-1234567890ab"),
};

/*
// FORM2: Fill array with already created and named variable pointers
// NOTE:  Forms one and two can be mixed
Variable *variableList[] = {
    mcuBoardBatt,
    mcuBoardAvailableRAM,
    mcuBoardSampNo,
    modemRSSI,
    modemSignalPct,
    // etc, etc, etc,
    calculatedVar
}
*/


// Count up the number of pointers in the array
int variableCount = sizeof(variableList) / sizeof(variableList[0]);

// Create the VariableArray object
VariableArray varArray(variableCount, variableList);


// ==========================================================================
//     The Logger Object[s]
// ==========================================================================
#include <LoggerBase.h>

// Create a new logger instance
Logger dataLogger(LoggerID, loggingInterval, &varArray);


// ==========================================================================
//    A Publisher to Monitor My Watershed / EnviroDIY Data Sharing Portal
// ==========================================================================
// Device registration and sampling feature information can be obtained after
// registration at https://monitormywatershed.org or https://data.envirodiy.org
const char *registrationToken = "12345678-abcd-1234-efgh-1234567890ab";   // Device registration token
const char *samplingFeature = "12345678-abcd-1234-efgh-1234567890ab";     // Sampling feature UUID

// Create a data publisher for the EnviroDIY/WikiWatershed POST endpoint
#include <publishers/EnviroDIYPublisher.h>
EnviroDIYPublisher EnviroDIYPOST(dataLogger, registrationToken, samplingFeature);


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
    // Wait for USB connection to be established by PC
    // NOTE:  Only use this when debugging - if not connected to a PC, this
    // could prevent the script from starting
    #if defined SERIAL_PORT_USBVIRTUAL
      while (!SERIAL_PORT_USBVIRTUAL && (millis() < 10000)){}
    #endif

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

    // Allow interrupts for software serial
    #if defined SoftwareSerial_ExtInts_h
        enableInterrupt(softSerialRx, SoftwareSerial_ExtInts::handle_interrupt, CHANGE);
    #endif
    #if defined NeoSWSerial_h
        enableInterrupt(neoSSerial1Rx, neoSSerial1ISR, CHANGE);
    #endif

    // Start the serial connection with the modem
    modemSerial.begin(modemBaud);

    // // Start the stream for the modbus sensors; all currently supported modbus sensors use 9600 baud
    // modbusSerial.begin(9600);
    //
    // // Start the SoftwareSerial stream for the sonar; it will always be at 9600 baud
    // sonarSerial.begin(9600);

    // Assign pins SERCOM functionality for SAMD boards
    // NOTE:  This must happen *after* the various serial.begin statements
    #if defined ARDUINO_ARCH_SAMD
    #ifndef ENABLE_SERIAL2
    pinPeripheral(10, PIO_SERCOM);  // Serial2 Tx/Dout = SERCOM1 Pad #2
    pinPeripheral(11, PIO_SERCOM);  // Serial2 Rx/Din = SERCOM1 Pad #0
    #endif
    #ifndef ENABLE_SERIAL3
    pinPeripheral(2, PIO_SERCOM);  // Serial3 Tx/Dout = SERCOM2 Pad #2
    pinPeripheral(5, PIO_SERCOM);  // Serial3 Rx/Din = SERCOM2 Pad #3
    #endif
    #endif

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

    // Set up the sleep/wake pin for the modem and put its inital value as "off"
    #if defined TINY_GSM_MODEM_SIM800 && defined SIM800_GPRSBEE_R6  // ONLY FOR GPRSBee R6!!!!
        if (modemSleepRqPin >= 0)
        {
            pinMode(modemSleepRqPin, OUTPUT);
            digitalWrite(modemSleepRqPin, LOW);
        }
    #else
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
    #endif

    // Set the timezone and offsets
    // Logging in the given time zone
    Logger::setTimeZone(timeZone);
    // Offset is the same as the time zone because the RTC is in UTC
    Logger::setTZOffset(timeZone);

    // Attach the modem and information pins to the logger
    dataLogger.attachModem(modem);
    dataLogger.setLoggerPins(wakePin, sdCardSSPin, sensorPowerPin, buttonPin, greenLED);

    // Begin the logger
    dataLogger.begin();

    // Note:  Please change these battery voltages to match your battery
    // Check that the battery is OK before powering the modem
    if (getBatteryVoltage() > 3.7)
    {
        modem.modemPowerUp();
        modem.wake();

        // Run any extra pre-set-up for the modem
        Serial.println(F("Running extra modem pre-setup"));
        extraModemSetup();
        modem.setup();

        // At very good battery voltage, or with suspicious time stamp, sync the clock
        // Note:  Please change these battery voltages to match your battery
        if (getBatteryVoltage() > 3.8 ||
            dataLogger.getNowEpoch() < 1546300800 ||  /*Before 01/01/2019*/
            dataLogger.getNowEpoch() > 1735689600)  /*Before 1/1/2025*/
        {
            // Synchronize the RTC with NIST
            Serial.println(F("Attempting to synchronize RTC with NIST"));
            if (modem.connectInternet(120000L))
            {
                dataLogger.setRTClock(modem.getNISTTime());
            }
        }
    }

    // Set up the sensors, except at lowest battery level
    if (getBatteryVoltage() > 3.4)
    {
        Serial.println(F("Setting up sensors..."));
        varArray.setupSensors();
    }

    // Power down the modem
    modem.modemSleepPowerDown();

    // Create the log file, adding the default header to it
    // Do this last so we have the best chance of getting the time correct and
    // all sensor names correct
    // Writing to the SD card can be power intensive, so if we're skipping
    // the sensor setup we'll skip this too.
    if (getBatteryVoltage() > 3.4)
    {
        dataLogger.turnOnSDcard(true);  // true = wait for card to settle after power up
        dataLogger.createLogFile(true);  // true = write a new header
        dataLogger.turnOffSDcard(true);  // true = wait for internal housekeeping after write
    }

    // Call the processor sleep
    Serial.println(F("Putting processor to sleep"));
    dataLogger.systemSleep();
}


// ==========================================================================
// Main loop function
// ==========================================================================

// Use this short loop for simple data logging and sending
// /*
void loop()
{
    // Note:  Please change these battery voltages to match your battery
    // At very low battery, just go back to sleep
    if (getBatteryVoltage() < 3.4)
    {
        dataLogger.systemSleep();
    }
    // At moderate voltage, log data but don't send it over the modem
    else if (getBatteryVoltage() < 3.7)
    {
        dataLogger.logData();
    }
    // If the battery is good, send the data to the world
    else
    {
        dataLogger.logDataAndSend();
    }
}
// */


// Use this long loop when you want to do something special
// Because of the way alarms work on the RTC, it will wake the processor and
// start the loop every minute exactly on the minute.
// The processor may also be woken up by another interrupt or level change on a
// pin - from a button or some other input.
// The "if" statements in the loop determine what will happen - whether the
// sensors update, testing mode starts, or it goes back to sleep.
/*
void loop()
{
    // Assuming we were woken up by the clock, check if the current time is an
    // even interval of the logging interval
    // We're only doing anything at all if the battery is above 3.4V
    if (dataLogger.checkInterval() && getBatteryVoltage() > 3.4)
    {
        // Flag to notify that we're in already awake and logging a point
        Logger::isLoggingNow = true;

        // Print a line to show new reading
        Serial.println(F("------------------------------------------"));
        // Turn on the LED to show we're taking a reading
        dataLogger.alertOn();
        // Power up the SD Card, but skip any waits after power up
        dataLogger.turnOnSDcard(false);

        // Turn on the modem to let it start searching for the network
        // Only turn the modem on if the battery at the last interval was high enough
        // NOTE:  if the modemPowerUp function is not run before the completeUpdate
        // function is run, the modem will not be powered and will not return
        // a signal strength readign.
        if (getBatteryVoltage() > 3.7)
            modem.modemPowerUp();

        // Do a complete update on the variable array.
        // This this includes powering all of the sensors, getting updated
        // values, and turing them back off.
        // NOTE:  The wake function for each sensor should force sensor setup
        // to run if the sensor was not previously set up.
        varArray.completeUpdate();

        // Create a csv data record and save it to the log file
        dataLogger.logToSD();

        // Connect to the network
        // Again, we're only doing this if the battery is doing well
        if (getBatteryVoltage() > 3.7)
        {
            if (modem.connectInternet())
            {
                // Publish data to remotes
                dataLogger.sendDataToRemotes();

                // Sync the clock at midnight
                if (Logger::markedEpochTime != 0 && Logger::markedEpochTime % 86400 == 0)
                {
                    Serial.println(F("Running a daily clock sync..."));
                    dataLogger.setRTClock(modem.getNISTTime());
                }

                // Disconnect from the network
                modem.disconnectInternet();
            }
            // Turn the modem off
            modem.modemSleepPowerDown();
        }

        // Cut power from the SD card - without additional housekeeping wait
        dataLogger.turnOffSDcard(false);
        // Turn off the LED
        dataLogger.alertOff();
        // Print a line to show reading ended
        Serial.println(F("------------------------------------------\n"));

        // Unset flag
        Logger::isLoggingNow = false;
    }

    // Check if it was instead the testing interrupt that woke us up
    if (Logger::startTesting) dataLogger.testingMode();

    // Call the processor sleep
    dataLogger.systemSleep();
}
*/
