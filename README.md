# ModularSensors

A "library" of sensors to give all sensors a common interface of functions.  This library was written primarily for the [EnviroDIY Mayfly](https://envirodiy.org/mayfly/) but should be applicable to other Arduino based boards as well.  To use the full functionality of this library, you do need a "large" amount of RAM (in Arduino terms).  The processor on an Arduino UNO or similar board is unlikely to be able to handle all of the logger functionality, though it will be able to access individual sensors.  An Arduino Mega, Sodaq Autonomo, Sodaq One, or other similar boards should be able to use the full library.  To date, however, the EnviroyDIY Mayfly is the only board that has been tested.

Each sensor is implemented as a subclass of the "SensorBase" class.  Within each sensor, there are subclasses for each variable that the sensor can return.  At this time, all sensors return values as floats.

To use a sensor in your sketch, you must include SensorBase.h in your script AND separately include xxx.h for each sensor you intend to use.  While this may force you to write many more include statements, it makes the library lighter weight by not requiring you to install the functions and dependencies for every sensor when only one is needed.

#### Contents:
- [Basic Senor and Variable Functions](#Basic)
    - [Individual Sensors Code Examples](#individuals)
- [Grouped Sensor Functions](#Grouped)
    - [SensorArray Code Examples](#ArrayExamples)
- [Logger Functions](#Logger)
    - [Logger Code Examples](#LoggerExamples)
- Available Sensors
    - [EnviroDIY Mayfly Onboard Sensors](#MayflyOnboard)
    - [MaxBotix MaxSonar](#MaxBotix)
    - [Campbell Scientific OBS-3+](#OBS3)
    - [Decagon Devices 5TM](#5TM)
    - [Decagon Devices CTD-10](#CTD)
    - [Decagon Devices ES-2](#ES2)


## <a name="Basic"></a>Basic Senor and Variable Functions

### Functions Available for Each Sensor
- **setup()** - This "sets up" the sensor - setting up serial ports, etc required for the given sensor.  This must always be called for each sensor within the "setup" loop of your Arduino program.
- **getStatus()** - This returns the current status of the sensor, if the sensor has some way of giving it to you.  (Most do not.)
- **sleep()** - This puts the sensor to sleep, often by stopping the power.  Returns true.
- **wake()** - This wakes the sensor up and sends it power.  Returns true.
- **getSensorName()** - This gets the name of the sensor and returns it as a string.
- **getSensorLocation()** - This returns the data pin or other sensor installation information as a string.  This is the location where the sensor is connected to the data logger, NOT the position of the sensor in the environment.
- **update()** - This updates the sensor values and returns true when finished.  For digital sensors with a single infomation return, this only needs to be called once for each sensor, even if there are multiple variable subclasses for the sensor.

### Functions for Each Variable Returned by a Sensor
- **getVarName()** - This returns the variable's name using http://vocabulary.odm2.org/variablename/ as a string
- **getVarUnit()** - This returns the variable's unit using http://vocabulary.odm2.org/units/ as a string
- **getValue()** - This returns the current value of the variable as a float.  You should call the update function before calling getValue.  As a backup, if the getValue function sees that the update function has not been called within the last 60 seconds, it will re-call it.

### <a name="individuals"></a>Examples Using Individual Sensor and Variable Functions
To access and get values from a sensor, you must create an instance of the variable subclass you are interested in.  Each variable has different parameters that you must specify when creating the variable instance.  You must create a new instance for each _variable_, not just each sensor.  When using multple variables from the same sensor, you can save time by only calling the setup() and update() functions on a single variable and then calling the getValue() function on all of the variables.  A very simple program which creates instances of the variables and get data from all the parameters measured by a Decagon CTD you might be something like:

```cpp
cond DecagonCTD_Cond(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
temp DecagonCTD_Temp(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
depth DecagonCTD_Depth(char SDI12address, int powerPin, int dataPin, int numReadings = 1)

setup()
{
    cond.setup();
}

loop()
{
    cond.update();
    Serial.println(cond.getValue());
    Serial.println(temp.getValue());
    Serial.println(depth.getValue());
    delay(60000);
}
```


## <a name="Grouped"></a>Grouped Sensor Functions
Having a unified set of functions to access many sensors allows us to quickly poll through a list of sensors to get all results quickly.  Within sensor base, there is a class "SensorArray" that adds functions to use on an array of pointers to sensor objects.

### Functions Available for a SensorArray Object:

- **setupSensors()** - This sets up all of the sensors in the list by running each sensor object's setup() function.  If a sensor doesn't respond to its setup command, the command is called 5 times in attempt to make a connection.  To save time, if two variables from the same sensor are in sequence in the pointer array, the setup function will only be called for the first variable in the sequence.  If all sensors are set up sucessfully, returns true.
- **sensorsSleep()** - This puts all sensors to sleep (ie, cuts power).  Returns true.
- **sensorsWake()** - This wakes all sensors (ie, gives power).  Returns true.
- **updateAllSensors()** - This updates all sensor values.  To save time, if two variables from the same sensor are in sequence in the pointer array, the update function will only be called for the first variable in the sequence.  Returns true.
- **printSensorData(Stream *stream)** - This prints curent sensor values along with metadata to a stream.  By default, it will print to the first Serial port.
- **generateSensorDataCSV()** - This returns an Arduino String containing comma separated list of sensor values.  This string does _NOT_ contain a timestamp of any kind.

### <a name="ArrayExamples"></a>SensorArray Examples:

To use the SensorArray module, you must first create the array of pointers.  This must be done outside of the setup() or loop() functions.  Remember that you must create a new instance for each _variable_, not just each sensor.  You should order your list so all the variables from a single sensor come one after the other.  All functions will be called on the sensors in the order they appear in the list.

```cpp
SensorBase *SENSOR_LIST[] = {
    new Sensor1_Variable1(param1, param2, ..., paramX),
    new Sensor1_Variable2(param1, param2, ..., paramX),
    new Sensor2_Variable1(param1, param2, ..., paramX),
    ...
    new SensorX_VariableX(param1, param2, ..., paramX)
};
int sensorCount = sizeof(SENSOR_LIST) / sizeof(SENSOR_LIST[0]);
```

Once you have created the array of pointers, you can initialize the SensorArray module and setup all of the sensors at once in the setup function:

```cpp
// Initialize the sensor array;
sensors.init(sensorCount, SENSOR_LIST);
// Set up all the sensors
sensors.setupSensors();
```

You can then get values or variable names for all of the sensors within the loop with calls like:

```cpp
// Update the sensor value(s)
sensors.updateAllSensors();
// Print the data to the screen
sensors.printSensorData();
```


## <a name="Logger"></a>Logger Functions
Our main reason to unify the output from many sensors is to easily log the data to an SD card and to send it to the EnviroDIY data page.  There are two modules available to use with the sensors to log data:  LoggerBase and LoggerEnviroDIY.  Both of these are sub-classes of SensorArray and contain all of the functions available to a SenorArray as described above.  These both will add the abilities to communicate with a DS3231 real time clock and set up the Arduino as a logger which goes to deep sleep between readings to conserver power.  LoggerBase only adds the functionality to write the data from the sensors to a csv file on a connected SD card.  LoggerEnviroDIY adds the ability to send data to the EnviroDIY data portal.  Both logger modules depend on the [Sodaq](https://github.com/SodaqMoja/Sodaq_DS3231) or [EnviroDIY DS-3231](https://github.com/EnviroDIY/Sodaq_DS3231) (for clock control), the [Sodaq RTCTimer library](https://github.com/SodaqMoja/RTCTimer) (for timing functions), the [EnviroDIY modified version of Sodaq's pin change interrupt library](https://github.com/EnviroDIY/PcInt_PCINT0) (for waking the processor from clock alarms), the AVR sleep library (for low power sleeping), and the [SdFat library](https://github.com/greiman/SdFat) for communicating with the SD card.  The LoggerEnviroDIY has the additional dependency of the [EnviroDIY version of Sodaq's GPRSBee library](https://github.com/EnviroDIY/GPRSbee) for GPRS communications.

### Functions Available for a LoggerBase Object:
Setup and initialization functions:
- **init(int timeZone, int SDCardPin, int interruptPin, int sensorCount, SensorBase *SENSOR_LIST[], float loggingIntervalMinutes, const char *loggerID = 0, const char *samplingFeature = 0, const char *UUIDs[] = 0)** - Initializes the logger object.  Must happen within the setup function.
- **setAlertPin(int ledPin)** - Sets a pin to put out an alert that a measurement is being logged.  This should be a pin with a LED on it.

Functions to access the clock in proper format and time zone:
- **getNow()** - Returns the current unix time stamp in the current time zone.
- **getDateTime_ISO8601()** - Returns the current date/time as an ISO8601 formatted Arduino String.


Functions for the timer and sleep modes:
- **setupTimer()** - Sets up the timer for repeated logging events.  This should be called in the setup function.
- **checkInterval()** - This returns true if the current time is an even iterval of the logging interval, otherwise false.
- **setupSleep()** - Sets up the sleep mode and interrupts to wake it back up.  This should be called in the setup function.
- **systemSleep()** - Puts the system into deep sleep mode.  This should be called at the very end of the loop function.

Functions for logging data:
- **setupLogFile()** - This creates a file on the SD card and writes a header to it.
- **generateSensorDataCSV()** - This returns an Arduino String containing the time and a comma separated list of sensor values.
- **logToSD(String rec)** - Writes a line the the SD card.

Convience functions to do it all:
- **begin()** - Starts the logger.  Must be in the setup function.
- **log()** - Logs data, must be the entire content of the loop function.

### Additional Functions Available for a LoggerEnviroDIY Object:
- **setToken(const char *registrationToken)** - Sets the retistration token to access the EnviroDIY streaming data loader API.
- **setupBee(xbee beeType, Stream *beeStream, int beeCTSPin, int beeDTRPin, const char *APN)** - Sets up the internet communcation, with either GPRSv4, GPRSv6, or WIFI
- **generateSensorDataJSON()** - Generates a properly formatted JSON string to go to the EnviroDIY streaming data loader API.
- **postDataWiFi()** - Creates proper headers and sends data to the EnviroDIY data portal via WiFi.  You must have set up a WIFI bee for this to work.
- **postDataGPRS()** - Creates proper headers and sends data to the EnviroDIY data portal via GPRS.  You must have set up a GPRS bee for this to work.
- **printPostResult(int result)** - Interprets HTML response code.

### <a name="LoggerExamples"></a>Logger Examples:

To set up logging, you must first include the appropriate logging module and create a new logger instance.  This must happen outside of the setup and loop functions:

```cpp
// Import Logger Module
#include <LoggerBase.h>
// Create a new logger instance
LoggerBase Logger;
```

--OR--

```cpp
// Import Logger Module
#include <LoggerEnviroDIY.h>
// Create a new logger instance
LoggerEnviroDIY EnviroDIYLogger;
```

_Within the setup function_, you must then initialize the logger and then run the logger setup.  For the EnviroDIY logger, you must also set up the communication.  (Please note that these are show with default values.):

```cpp
// Initialize the logger;
Logger.init(int timeZone, int SDCardPin, int interruptPin,
            int sensorCount,
            SensorBase *SENSOR_LIST[],
            float loggingIntervalMinutes,
            const char *loggerID = 0,
            const char *samplingFeature = 0,
            const char *UUIDs[] = 0);
// OPTIONAL - specify a pin to give an alert when a measurement is taken
// This should generally be a pin with an LED
setAlertPin(int ledPin);
// Begin the logger;
Logger.begin();
```

--OR--

```cpp
// Initialize the logger;
EnviroDIYLogger.init(int timeZone, int SDCardPin, int interruptPin,
                     int sensorCount,
                     SensorBase *SENSOR_LIST[],
                     float loggingIntervalMinutes,
                     const char *loggerID = 0,
                     const char *samplingFeature = 0,
                     const char *UUIDs[] = 0);
// OPTIONAL - specify a pin to give an alert when a measurement is taken
// This should generally be a pin with an LED
setAlertPin(int ledPin);
// Set up the communication with EnviroDIY
EnviroDIYLogger.setToken(const char *registrationToken);
EnviroDIYLogger.setupBee(xbee beeType,
                         Stream *beeStream,
                         int beeCTSPin,
                         int beeDTRPin,
                         const char *APN);
// Run the logger setup;
EnviroDIYLogger.begin();
```

_Within the main loop function_, all logging and sending of data can be done using a single program line.  Because the built-in log functions already handle sleeping and waking the board processor, there cannot be nothing else within the loop function.  If you would like to do other things within the loop function, you should access the component logging functions individually instead of using these short-cut functions.
```cpp
void loop()
{
    Logger.log();
}
```

--OR--

```cpp
void loop()
{
    EnviroDIYLogger.log();
}
```


## Available sensors

#### <a name="MayflyOnboard"></a>Mayfly Onboard Sensors

The version of the Mayfly is required as input (ie, "v0.3" or "v0.4" or "v0.5")  You must have the [Sodaq](https://github.com/SodaqMoja/Sodaq_DS3231) or [EnviroDIY DS-3231](https://github.com/EnviroDIY/Sodaq_DS3231) library installed to use this sensor.  Because the sensors operate independently, you must call the update function for each one before calling getValue.
- MayflyOnboardTemp(char const *version)
- MayflyOnboardBatt(char const *version)
- MayflyFreeRam()

#### <a name="MaxBotix"></a>[MaxBotix MaxSonar](http://www.maxbotix.com/Ultrasonic_Sensors/High_Accuracy_Sensors.htm) - HRXL MaxSonar WR or WRS Series with TTL Outputs

The power/excite pin and digital data pin are needed as input.  The power pin must provide smoothed digital power.  You must have the [EnviroDIY modified version of SoftwareSerial](https://github.com/EnviroDIY/SoftwareSerial_PCINT12/) installed to use this sensor.  This modified version is needed so there are no pin change interrupt conflicts with the SDI-12 library or the software pin change interrupt library used to wake the clock.  Because of this, the MaxBotix must be installed on a digital pin that depends on pin change interrupt vector 1 or 2.  On the Mayfly, the empty pins in this range are pins D10, D11, and D18.  (Changing the solder jumper options on the back of the board may eliminate D18 as a possibility.)
- MaxBotixSonar_Depth(int powerPin, int dataPin)

#### <a name="OBS3"></a>[Campbell Scientific OBS-3+](https://www.campbellsci.com/obs-3plus)

The power pin, analog data pin, and calibration values for Ax^2 + Bx + C are required as inputs and the sensor must be attached to a TI ADS1115 ADD converter (such as on the first four analog pins of the Mayfly).  You must have the [Adafruit ADS1015 library](https://github.com/Adafruit/Adafruit_ADS1X15/) installed to use this sensor.

Note that to access both the high and low range returns, two instances must be created, one at the low range return pin and one at the high pin.
- CampbellOBS3_Turbidity(int powerPin, int dataPin, float A, float B, float C)

#### <a name="5TM"></a>[Decagon Devices 5TM](https://www.decagon.com/en/soils/volumetric-water-content-sensors/5tm-vwc-temp/) Soil Moisture and Temperature Sensor

The SDI-12 address of the sensor, the power pin, the data pin, and a number of distinct readings to average are required as inputs.  You must have the [EnviroDIY Arduino SDI-12 library](https://github.com/EnviroDIY/Arduino-SDI-12/tree/mf_archive) that has been modified to only use PCInt3 installed to use this sensor.  This modified version is needed so there are no pin change interrupt conflicts with the SoftwareSerial library or the software pin change interrupt library used to wake the clock.  Because of this, the 5TM (and all SDI-12 based sensors) must be installed on on of the digital pins that depends on pin change interrupt vector 3.  On the Mayfly, the empty pins in this range are pins D4, D5, D6, and D7.

Calling the update function for any one of the three variables will update all three.

- Decagon5TM_Ea(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
- Decagon5TM_Temp(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
- Decagon5TM_VWC(char SDI12address, int powerPin, int dataPin, int numReadings = 1)

#### <a name="CTD"></a>[Decagon Devices CTD-10](https://www.decagon.com/en/hydrology/water-level-temperature-electrical-conductivity/ctd-10-sensor-electrical-conductivity-temperature-depth/) Electrical Conductivity, Temperature, and Depth Sensor

The SDI-12 address of the sensor, the power pin, the data pin, and a number of distinct readings to average are required as inputs.  You must have the [EnviroDIY Arduino SDI-12 library](https://github.com/EnviroDIY/Arduino-SDI-12/tree/mf_archive) that has been modified to only use PCInt3 installed to use this sensor.  This modified version is needed so there are no pin change interrupt conflicts with the SoftwareSerial library or the software pin change interrupt library used to wake the clock.  Because of this, the 5TM (and all SDI-12 based sensors) must be installed on on of the digital pins that depends on pin change interrupt vector 3.  On the Mayfly, the empty pins in this range are pins D4, D5, D6, and D7.

Calling the update function for any one of the three variables will update all three.

- DecagonCTD_Cond(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
- DecagonCTD_Temp(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
- DecagonCTD_Depth(char SDI12address, int powerPin, int dataPin, int numReadings = 1)

#### <a name="ES2"></a>[Decagon Devices ES-2](http://www.decagon.com/en/hydrology/water-level-temperature-electrical-conductivity/es-2-electrical-conductivity-temperature/) Electrical Conductivity Sensor

The SDI-12 address of the sensor, the power pin, the data pin, and a number of distinct readings to average are required as inputs.  You must have the [EnviroDIY Arduino SDI-12 library](https://github.com/EnviroDIY/Arduino-SDI-12/tree/mf_archive) that has been modified to only use PCInt3 installed to use this sensor.  This modified version is needed so there are no pin change interrupt conflicts with the SoftwareSerial library or the software pin change interrupt library used to wake the clock.  Because of this, the 5TM (and all SDI-12 based sensors) must be installed on on of the digital pins that depends on pin change interrupt vector 3.  On the Mayfly, the empty pins in this range are pins D4, D5, D6, and D7.

Calling the update function for either one of the variables will both.

- DecagonES2_Cond(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
- DecagonES2_Temp(char SDI12address, int powerPin, int dataPin, int numReadings = 1)
