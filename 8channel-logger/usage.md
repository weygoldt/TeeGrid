# Usage

## Dependencies

The [`8channel-logger.ino`](8channel-logger.ino) sketch is based on
the following libraries:

- [Arduino Time Library](https://github.com/PaulStoffregen/Time)
- [ADC](https://github.com/pedvide/ADC)
- [SdFat version2](https://github.com/greiman/SdFat)
- [TeeRec](https://github.com/janscience/TeeRec) library.


## Installation

The [Arduino Time Library](https://github.com/PaulStoffregen/Time) and
[ADC](https://github.com/pedvide/ADC) libraries are already included
in [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html).

For installing [SdFat version2](https://github.com/greiman/SdFat) open in
the Arduino IDE Tools - Manage libraries. Search for SdFat and install it.

For [TeeRec](https://github.com/janscience/TeeRec) clone the
repository in 'Arduino/libraries':
```sh
cd Arduino/libraries
git clone https://github.com/janscience/TeeRec.git
```

Alternatively, download the whole repository as a zip archive (open
https://github.com/janscience/TeeRec in your browser and click on the
green "Code" button). Unpack the zip file:
```sh
cd Arduino/libraries
unzip ~/Downloads/TeeRec-main.zip
```

Close the Arduino IDE and open it again. Then the Arduino IDE knows
about the newly installed libraries.


## Setup

Open the [`8channel-logger.ino`](8channel-logger.ino) sketch in the
Arduino IDE.

### Data acquisition

In the top section marked as "Settings" you may adapt some settings
according to your needs. The first block is about the data
acquisition. You can set the sampling rate, bit resolution, number of
averages per sample, conversion and sampling speeds, and input pins
(channels). If you change these settings, check the output on the
serial monitor and the performance before using the logger! See
[TeeRec](https://github.com/janscience/TeeRec) for various sketches
and tools that help you to select the best settings for the data
acquisition.

### File size and naming

The second section is about the files that store the data on the SD
card.  The files are stored in a directory whose name is specified by
the `path` variable. The file names in this directory are specified by
the `fileName` variable. The file name can be an arbitrary string, but
should end with the '.wav' extension. The following special strings in
the file name are replaced by the current date, time, or a number:

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

Once you modified the sketch to your needs, upload it to the Teensy
(`CTRL U`).


## Configuration

Most of the settings described above can be configured via a
configuration file. Simply place a configuration file named
[`teegrid.cfg`](teegrid.cfg) into the root folder of the SD card. If
present, this file is read once on startup.

*WARNING* the settings of the config file overwrite the settings made
 in the sketch!

The content of the configuration file should look like this:

```txt
# Configuration file for TeeGrid logger unit.

Settings:
  Path        : recordings     # path where to store data
  FileName    : grid1-SDATETIME.wav  # may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
  FileTime    : 10min       # s, min, or h
  InitialDelay: 10s         # s, min, or h

ADC:
  SamplingRate: 20kHz      # Hz, kHz, MHz
  Averaging   : 4
  Conversion  : high
  Sampling    : high
  Resolution  : 12bit
  Reference   : 3.3V
``` 

Everything behind '#' is a comment. All lines without a colon are
ignored.  Unknown keys are ignored but reported. Times and frequencies
understand various units as indicated in the comments. Check the
serial monitor to confirm the right settings.


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
Insert the SD card into the Teensy. Then start the Teensy by
connecting it to power. On start up the
[`8channel-logger.ino`](8channel-logger.ino) sketch reads in the
`settime.cfg` file and sets the real-time clock to this time. Then the
file is deleted to avoid resetting the time.


## Usage

Connect the Teensy to a battery and let it record the data.


### LED

The on-board LED of the Teensy indicates the following events:

- On startup the LED is switched on during the initialization of the
  data acqusition and SD card. This can last for up to 2 seconds
  (timeout for establishing a serial connection with a computer).

- For the time of the initial delay (nothing is recorded to SD card)
  a double-blink every 2s is emitted.

- Normal operation, i.e. data acquisition is running and data are
  written to SD card: the LED briefly flashes every 5 seconds.

- Whenever a file is closed (and a new one opened), the LED lights for
  1 second. Then it continues with flashes every 5 seconds.

- The LED is switched off if no data can be written on the SD card (no
  SD card present or SD card full) or data acquisition is not working.
  Connect the Teensy to the computer and open the serial monitor of
  the Arduino IDE (`Ctrl+Shif+M`). On startup the settings for the
  data acquisition are reported and in case of problems an error
  message is displayed.

