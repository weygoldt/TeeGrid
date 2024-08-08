# R4.x sensors logger

Logger for 2 to 16 channels based on [Teensy_Amp R4.1](https://github.com/janscience/Teensy_Amp/tree/main/R4.1) or [R4.2](https://github.com/janscience/Teensy_Amp/tree/main/R4.2)
  connected to a [Teensy 4.1](https://www.pjrc.com/store/teensy41.html) with additional sensors.

Designed by Jan Benda in September 2023.


## Installation

See the [installation instructions](../../doc/install.md) for the
TeeGrid library.


## Setup

The behavior of the logger sketch can be modfied in various
ways. Either by editing some variables directly in the sketch, as
described here, or by a configuration file provided on the SD card, as
described in the next section.

*WARNING* the settings of the config file overwrite the settings made
 in the sketch!

For editing the sketch, open the
[`R4-sensors-logger.ino`](R4-sensors-logger.ino) sketch in the Arduino
IDE (`File` - `Open`, `Ctrl-O`) and edit it appropriately.  In the top
section marked as `Default settings` you may adapt some settings
according to your needs as described in the next sections.

Once you modified the sketch to your needs, compile and upload it to
the Teensy (`Ctrl-U`).


### Data acquisition

The first section is about the data acquisition:

- `NCHANNELS`: the number of channels you want to record. If you use a
  single [Teensy_Amp R4.1](https://github.com/janscience/Teensy_Amp/tree/main/R4.1)
  or [R4.2](https://github.com/janscience/Teensy_Amp/tree/main/R4.2) then set this to 8.
  If you use [both of them stacked
  together](https://github.com/janscience/Teensy_Amp/tree/main/R4.1-R4.2),
  then set it to 16.
- `SAMPLING_RATE`: the sampling rate you want to use (48000 or 96000 Hertz).
- `PREGAIN`: the gain of the preamplifier, either 10 (default) or 1
  (for recording electric eels).
- `GAIN`: additional gain of the ADC in dB. 0: x1, 20: x10, 40: x100.

For wavefish use an amplifier with a pregain of 10 and set the gain
to 20dB. For electric eels, use an amplifier with a pregain of 1 and
set the gain to 0dB.


### File naming and size

The second section is about the files that store the data on the SD
card.

- `PATH`: name of the directory, in which the files are stored.
- `FILENAME`: name of the files that store the recorded data. See
  below for special strings to insert data and time.
- `FILE_SAVE_TIME`: each file will store that many seconds of the
  continuous recording.
- `INITIAL_DELAY`: after startup, wait for the specified time in
  seconds before writing data to files.

 The following special strings in the file name are replaced by
the current date, time, or a number:

- `DATE`: the current date as ISO string (YYYY-MM-DD)
- `SDATE`: "short date" - the current date as YYYYMMDD
- `TIME`: the current time as ISO string but with dashes instead of colons (HH-MM-SS)
- `STIME`: "short time" - the current time as HHMMSS
- `DATETIME`: the current date and time as YYYY-MM-DDTHH-MM-SS
- `SDATETIME`: "short data and time" - the current date and time as YYYYMMDDTHHMMSS
- `ANUM`: a two character string numbering the files: 'aa', 'ab', 'ac', ..., 'ba', 'bb', ...
- `NUM`: a two character string numbering the files: '01', '02', '03', ..., '10', '11', ...
- `NUM3`: a three character string numbering the files: '001', '002', '003', ..., '010', '011', ..., '100', '101', ...
- `NUM4`: a four character string numbering the files: '0001', '0002', '0003', ..., '0010', '0011', ..., '0100', '0101', ..., '1000', '1001', ...


### Pins and Sensors

- `LED_PIN`: the Teensy pin to which the LED on the amplifier is connected to.
- `TEMP_PIN`: the Teensy pin to which the DATA wire of the [Dallas
  DS18x20](https://github.com/janscience/ESensors/blob/main/docs/chips/ds18x20.md)
  temperature sensor is connected to.




## Configuration

Most of the settings described above can be configured via a
configuration file. Simply place a configuration file named
[`logger.cfg`](logger.cfg) into the root folder of the SD card. If
present, this file is read once on startup.

The content of the configuration file should look like this:

```txt
# Configuration file for TeeGrid logger unit.

Settings:
  Path           : recordings  # path where to store data
  FileName       : logger1-SDATETIME.wav  # may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
  FileTime       : 10min       # s, min, or h
  InitialDelay   : 10s         # ms, s, or min

ADC:
  SamplingRate: 48kHz          # Hz, kHz, MHz
  NChannels: 16
  Gain: 20                     #dB
``` 

Everything behind '#' is a comment. All lines without a colon are
ignored.  Unknown keys are ignored but reported. Times and frequencies
understand various units as indicated in the comments. Check the
serial monitor of the Arduino IDE (`Ctrl+Shif+M`) to confirm the right
settings.


## Real-time clock

For proper naming of files, the real-time clock needs to be set to the
right time. The easiest way to achieve this, is to compile and upload
the [`R4-sensors-logger.ino`](R4-sensors-logger.ino) sketch from the
Arduino IDE.

Alternatively, you may copy a file named `settime.cfg` into the root
folder of the SD card. This file contains a single line with a date
and a time in the following format:
``` txt
YYYY-MM-DDTHH:MM:SS
```

In a shell you might generate this file via
``` sh
date +%FT%T > settime.cfg
```
and then edit this file to some time in the near future.

Insert the SD card into the Teensy. Start the Teensy by connecting it
to power. On start up the [`R4-sensors-logger.ino`](R4-sensors-logger.ino)
sketch reads in the `settime.cfg` file, sets the real-time clock to
this time, and then deletes the file from the SD card to avoid
resetting the time at the next start up.


## Logging

1. *Format the SD card.*
2. Optional: Copy your logger.cfg file onto the SD card.
3. Insert the SD card into the Teensy.
4. Connect the Teensy to a battery/power bank and let it record the data.

The SD card needs to be *reformatted* once in a while. The logger runs
until the battery is drained and therefore cannot properly close the
last files. This leaves the file system in a corrupted state, which
apparently results in very long delays when writing to the SD card
again.

You may use the sketch provided by the SdFat library for formatting
the SD card directly on the Teensy: In the Arduino IDE menu select
File - Examples, browse to the last section ("Examples from custom
libraries"), select "SdFat" and then "SdFormatter". Upload the script
on Teensy and open the serial monitor. Follow the instructions on the
serial monitor.


### LED

The on-board LED of the Teensy and the LED of the [Teensy_Amp
R4.1](https://github.com/janscience/Teensy_Amp/tree/main/R4.1) or
[R4.2](https://github.com/janscience/Teensy_Amp/tree/main/R4.2)
indicate the following events:

- On startup the LED is switched on during the initialization of the
  data acqusition and SD card. This can last for up to 2 seconds
  (timeout for establishing a serial connection with a computer).

- For the time of the initial delay (nothing is recorded to SD card)
  a double-blink every 2s is emitted.

- Normal operation, i.e. data acquisition is running and data are
  written to SD card: the LED briefly flashes every 5 seconds.

- Whenever a file is closed and the next one opened, the LED lights for
  1 second. Then it continues with flashes every 5 seconds.

- The LED is switched off if no data can be written on the SD card (no
  SD card present or SD card full) or data acquisition is not working.
  In this case, connect the Teensy to a computer and open the serial
  monitor of the Arduino IDE (`Ctrl+Shif+M`). On startup the settings
  for the data acquisition are reported and in case of problems an
  error message is displayed.


## Files

Analog input data are stored on the SD card as wave files in the
directory specified by `PATH` with names `FILENAME`.wav . The metadata
in these files indicate the time when the file was created and the
overall gain of the hardware.

The sensor data are stored as a csv file with the very same name as
the first data file, but with `-sensors.csv` added to it.


## Improvements

- Add sensor for illumination and water conductivity. Make a PCB for this!
