# 8-channel logger with environmental sensors

- Records from 8 amplified analog inputs, same as the [8-channel
  logger](../8channel-logger).

- Water temperature is logged from a [DS18B20 1-wire digital
  thermometer](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf),
  air temperature, humidity, and pressure via [Bosch sensortec
  BME280](https://github.com/janscience/ESensors/blob/main/docs/chips/bme280.md),
  and light intensity via [AMS
  TSL2591](https://github.com/janscience/ESensors/blob/main/docs/chips/tsl2591.md)
  via [ESensors](https://github.com/janscience/ESensors) library.

Designed by Jan Benda in May 2022.


## Documentation

See the [documentation of the 8-channel logger](../8channel-logger).


## Dependencies and Installation

The sketch depends in addition on

- [ESensors](https://github.com/janscience/ESensors) library.


Install [ESensors library](https://github.com/janscience/ESensors)
from the Library Manager of the Arduino IDE. See also the
[installation
instructions](https://github.com/janscience/ESensors/blob/main/docs/install.md).


## Variables

`sensorsInterval` specifies at which intervals the environmental data
are written into a csv file.


## Configuration

The `Settings` section of the [`teegrid.cfg`](teegrid.cfg)
configuration file has an additional parameter `SensorInterval`, that
specifies the interval between readings of the environmental sensors.
The full configuration file looks like this:

```txt
# Configuration file for TeeGrid logger unit.

Settings:
  Path           : recordings  # path where to store data
  FileName       : grid1-SDATETIME  # may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM; the wav extension is added by the sketch.
  FileTime       : 10min       # s, min, or h
  InitialDelay   : 10s         # ms, s, or min
  SensorsInterval: 10s         # ms, s, or min

ADC:
  SamplingRate: 20kHz          # Hz, kHz, MHz
  Averaging   : 4
  Conversion  : high
  Sampling    : high
  Resolution  : 12bit
  Reference   : 3.3V
``` 


## Files

Analog input data are stored on the SD card as wave files in the
directory specified by `Path` with names `FileName`.wav .

Sensor data are stored in the same directory as an csv file with name
`FileName`-sensors.csv .  First column is a time stamp in ISO
date/time format. Further columns are trhe sensor readings.


## Temperature

Temperature is logged via an [DS18B20 1-wire digital
  thermometer](https://datasheets.maximintegrated.com/en/ds/DS18B20.pdf).

Here is how to connect the DS18B20 to the
[Teensy](https://www.pjrc.com/teensy/pinout.html#Teensy_3.5):

![ds18b20 teensy](images/ds18b20-teensy.png)

- black wire: GND (left most pin on the Teensy)
- red wire: 3.3V (between pin 12 and 14 on the Teensy)
- yellow wire: data on pin 10 (or any other digital input pin).

In addition you need to connect the data pin to 3.3V via a 4.7kΩ
pullup resistance.  See [here](
https://create.arduino.cc/projecthub/TheGadgetBoy/ds18b20-digital-temperature-sensor-and-arduino-9cc806)
for a circuit diagram.