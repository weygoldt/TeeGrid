# Usage

## Dependencies

The [`8channel-logger.ino`](8channel-logger.ino) sketch is based on
the following libraries:

- [Arduino Time Library](https://github.com/PaulStoffregen/Time)
- [ADC](https://github.com/pedvide/ADC)
- [SdFat version2](https://github.com/greiman/SdFat)
- [TeeRec](https://github.com/janscience/TeeRec) library.


## Installation

1. Download and install Teensyduino and the Arduino IDE, following the
   [installation instructions](https://www.pjrc.com/teensy/td_download.html).

2. Make sure that you also install the [Arduino Time
   Library](https://github.com/PaulStoffregen/Time),
   [ADC](https://github.com/pedvide/ADC), [SdFat
   version2](https://github.com/greiman/SdFat), and
   [Bounce2](https://github.com/thomasfredericks/Bounce2) libraries
   during the installation process of
   [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html).

3. For [TeeRec](https://github.com/janscience/TeeRec) clone the
   repository into `Arduino/libraries/`:
   ```sh
   cd Arduino/libraries
   git clone https://github.com/janscience/TeeRec.git
   ```

    Alternatively, download the whole repository as a zip archive (open
    https://github.com/janscience/TeeRec in your browser and click on the
    green `Code` button). Unpack the zip file:
    ```sh
    cd Arduino/libraries
    unzip ~/Downloads/TeeRec-main.zip
    ```

4. Close the Arduino IDE and open it again. Then the Arduino IDE knows
   about the newly installed libraries.

5. Clone TeeGrid into `Arduino/`
   ```sh
   cd Arduino/
   git clone https://github.com/janscience/TeeGrid.git
   ```

   As with TeeRec, alternatively, you can download the whole
   repository as a zip archive (open
   https://github.com/janscience/TeeGrid in your browser and click on
   the green `Code` button). Then unpack the zip file:
   ```sh
   cd Arduino/
   unzip ~/Downloads/TeeGrid-main.zip
   ```

6. Load `Arduino/TeeGrid/8channel-logger/8channel-logger.ino` into the
   Arduino IDE (`File` - `Open`, `Ctrl-O`).

7. Select the right Teensy board: in the menu of the Arduino IDE go to
   `Tools` - `Board` - `Teensyduino` and select your Teensy board.

8. Connect the Teensy to the USB. Compile and upload the
   `8channel-logger.ino` sketch by pressing `Ctrl-U`.


## Setup

The behavior of the logger sketch can be modfied in various
ways. Either by editing some variables directly in the sketch, as
described here, or by a configuration file provided on the SD card, as
described in the next section.

*WARNING* the settings of the config file overwrite the settings made
 in the sketch!

For editing the sketch, open the
[`8channel-logger.ino`](8channel-logger.ino) sketch in the Arduino IDE
(`File` - `Open`, `Ctrl-O`) and edit it appropriately as described in
the next paragraphs.

### Data acquisition

In the top section marked as `Settings` you may adapt some settings
according to your needs. The first section is about the data
acquisition. You can set the sampling rate, bit resolution, number of
averages per sample, conversion and sampling speeds, and input pins
(channels). If you change these settings, check the output on the
serial monitor and the performance before using the logger! See
[TeeRec](https://github.com/janscience/TeeRec#testing-data-acquisition)
for various sketches and tools that help you to select the best
settings for the data acquisition.

### Environmental sensors

In the second section environmental sensors are configured. Here you
can specify on which pin the temperature sensor is connected to, and
at which intervals the environmental data are written into a csv file.

### File size and naming

The third section is about the files that store the data on the SD
card.  The files are stored in a directory whose name is specified by
the `path` variable. The file names in this directory are specified by
the `fileName` variable. The '.wav' extension is added by the
sketch. The following special strings in the file name are replaced by
the current date, time, or a number:

- `DATE`: the current date as ISO string (YYYY-MM-DD)
- `SDATE`: "short date" - the current date as YYYYMMDD
- `TIME`: the current time as ISO string but with dashes instead of colons (HH-MM-SS)
- `STIME`: "short time" - the current time as HHMMSS
- `DATETIME`: the current date and time as YYYY-MM-DDTHH-MM-SS
- `SDATETIME`: "short data and time" - the current date and time as YYYYMMDDTHHMMSS
- `ANUM`: a two character string numbering the files: 'aa', 'ab', 'ac', ..., 'ba', 'bb', ...
- `NUM`: a two character string numbering the files: '01', '02', '03', ..., '10', '11', ...

`fileSaveTime` specifies for how many seconds data should be saved in
each file. The default is 10min.

`initialDelay` specifies an initial delay right after start up the
sketch waits before starting to store data on SD card.

Once you modified the sketch to your needs, compile and upload it to
the Teensy (`Ctrl-U`).


## Configuration

Most of the settings described above can be configured via a
configuration file. Simply place a configuration file named
[`teegrid.cfg`](teegrid.cfg) into the root folder of the SD card. If
present, this file is read once on startup.

The content of the configuration file should look like this:

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

Everything behind '#' is a comment. All lines without a colon are
ignored.  Unknown keys are ignored but reported. Times and frequencies
understand various units as indicated in the comments. Check the
serial monitor of the Arduino IDE (`Ctrl+Shif+M`) to confirm the right
settings.


## Real-time clock

For proper naming of files, the real-time clock needs to be set to the
right time. The easiest way to achieve this, is to compile and upload
the [`8channel-logger.ino`](8channel-logger.ino) sketch from the
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
and then editing the file to some time in the near future.

Then insert the SD card into the Teensy. Start the Teensy by
connecting it to power. On start up the
[`8channel-logger.ino`](8channel-logger.ino) sketch reads in the
`settime.cfg` file, sets the real-time clock to this time, and then
deletes the file to avoid resetting the time at the next start up.


## Logging

1. *Format the SD card.*
2. Copy your teegrid.cfg file onto the SD card.
3. Insert the SD card into the Teensy.
4. Connect the Teensy to a battery and let it record the data.

The SD card needs to be *reformatted after every usage* of the
logger. The logger runs until the battery is drained and therefore
cannot properly close the last files. This leaves the file system in a
corrupted state, which apparently results in very long delays when
writing to the SD card again.

You may use the sketch provided by the SdFat library for formatting
the SD card directly on the Teensy: In the Arduino IDE menu select
File - Examples, browse to the last section ("Examples from custom
libraries"), select "SdFat" and then "SdFormatter". Upload the script
on Teensy and open the serial monitor. Follow the instructions.


### LED

The on-board LED of the Teensy indicates the following events:

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
directory specified by `Path` with names `FileName`.wav .

Sensor data (currently only water temperature) are stored in the same
directory as an csv file with name `FileName`-temperatures.csv .
First column is a time stamp in ISO date/time formar. Second column is
the water temperature in degrees celsius.
