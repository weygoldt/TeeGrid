# R4.0 logger

Logger for 2, 4, 6, or 8 channels based on [TeensyAmp
  R4.0](https://github.com/janscience/Teensy_Amp/tree/main/R4.0)
  connected to a [Teensy 4.1](https://www.pjrc.com/store/teensy41.html).

Designed by Jan Benda in July 2023.


## Dependencies

The [`R40-logger.ino`](R40-logger.ino) sketch is based on the
[TeeRec](https://github.com/janscience/TeeRec) library.


## Installation

1. Download and install Teensyduino and the Arduino IDE, following the
   [installation instructions](https://www.pjrc.com/teensy/td_download.html),
   summarized [here](https://github.com/janscience/TeeRec/blob/main/docs/install.md).

2. Install [TeeRec library](https://github.com/janscience/TeeRec) from
   the Library Manager of the Arduino IDE. See also the [installation
   instructions](https://github.com/janscience/TeeRec/blob/main/docs/install.md).

3. Clone TeeGrid into `Arduino/libraries`
   ```sh
   cd Arduino/libraries
   git clone https://github.com/janscience/TeeGrid.git
   ```

   As with TeeRec, alternatively, you can download the whole
   repository as a zip archive (open
   https://github.com/janscience/TeeGrid in your browser and click on
   the green `Code` button). Then unpack the zip file:
   ```sh
   cd Arduino/libraries
   unzip ~/Downloads/TeeGrid-main.zip
   ```

4. Load `Arduino/TeeGrid/R40-logger/R40-logger.ino` into the
   Arduino IDE (`File` - `Open`, `Ctrl-O`).

5. Connect the Teensy to the USB and make sure that it is selected in
   the toolbar of the Arduino IDE.

6. Compile and upload the `R40-logger.ino` sketch by pressing
   `Ctrl-U`.


## Setup

The behavior of the logger sketch can be modfied in various
ways. Either by editing some defines directly in the sketch, as
described here, or by a configuration file provided on the SD card, as
described in the next section.

*WARNING* the settings of the config file overwrite the settings made
 in the sketch!

For editing the sketch, open the
[`R40-logger.ino`](R40-logger.ino) sketch in the Arduino IDE
(`File` - `Open`, `Ctrl-O`) and edit it appropriately as described in
the next paragraphs.


### Data acquisition

In the top section of the sketch marked as `Default settings` you may
adapt settings according to your needs. This first section is about
the data acquisition. You can set the maximum number of channels, the
gain of the pre-amplifier, sampling rate, and gain of the PCM1865
chips. If you change these settings, check the output on the serial
monitor and the performance before using the logger!


### File size and naming

The following section is about the files that store the data on the SD
card.  The files are stored in a directory whose name is specified by
the `PATH` define. The file names in this directory are specified by
`FILENAME`. The following special strings in the file name are
replaced by the current date, time, or a number:

- `DATE`: the current date as ISO string (YYYY-MM-DD)
- `SDATE`: "short date" - the current date as YYYYMMDD
- `TIME`: the current time as ISO string but with dashes instead of colons (HH-MM-SS)
- `STIME`: "short time" - the current time as HHMMSS
- `DATETIME`: the current date and time as YYYY-MM-DDTHH-MM-SS
- `SDATETIME`: "short data and time" - the current date and time as YYYYMMDDTHHMMSS
- `ANUM`: a two character string numbering the files: 'aa', 'ab', 'ac', ..., 'ba', 'bb', ...
- `NUM`: a two character string numbering the files: '01', '02', '03', ..., '10', '11', ...

`FILE_SAVE_TIME` specifies for how many seconds data should be saved in
each file.

`INITIAL_DELAY` specifies an initial delay right after start up the
sketch waits before starting to store data on SD card.

Once you modified the sketch to your needs, compile and upload it to
the Teensy (`Ctrl-U`).


## Configuration file

Most of the settings described above can be configured via a
configuration file. Simply place a configuration file named
[`teegrid.cfg`](teegrid.cfg) into the root folder of the SD card. If
present, this file is read once on startup.

The content of the configuration file should look like this:

```txt
# Configuration file for TeeGrid R40 logger unit.

Settings:
  Path           : recordings  # path where to store data
  FileName       : grid1-SDATETIME  # may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM; the wav extension is added by the sketch.
  FileTime       : 10min       # s, min, or h
  InitialDelay   : 10s         # ms, s, or min

ADC:
  SamplingRate   : 48kHz       # Hz, kHz, MHz
  NChannels      : 8
  Gain           : 20
``` 

Everything behind '#' is a comment. All lines without a colon are
ignored.  Unknown keys are ignored but reported. Times and frequencies
understand various units as indicated in the comments. Check the
serial monitor of the Arduino IDE (`Ctrl+Shif+M`) to confirm the right
settings.


## Real-time clock

For proper naming of files, the real-time clock needs to be set to the
right time. The easiest way to achieve this, is to compile and upload
the [`R40-logger.ino`](R40-logger.ino) sketch from the
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
to power. On start up the [`R40-logger.ino`](R40-logger.ino) sketch
reads the `settime.cfg` file, sets the real-time clock to this time,
and then deletes the file from the SD card to avoid resetting the time
at the next start up.


## Logging

1. Format the SD card.
2. Copy your teegrid.cfg file onto the SD card.
3. Insert the SD card into the Teensy.
4. Connect the Teensy to a battery and let it record the data.

The SD card needs to be *reformatted frequently* of the
logger. The logger runs until the battery is drained and therefore
cannot properly close the last files. This leaves the file system in a
corrupted state, which apparently results in very long delays when
writing to the SD card again.

You may use the sketch provided by the SdFat library for formatting
the SD card directly on the Teensy: In the Arduino IDE menu select
File - Examples - SdFat and then "SdFormatter". Upload the script on
Teensy and open the serial monitor. Follow the instructions to erase
and format the SD card.


### LED

The on-board LED of the Teensy indicates the following events:

- On startup the LED is switched. This can last for up to 2 seconds
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
directory specified by `PATH` with names `FILENAME`. In the meta-data
of the wave file the gain and the channel names are stored.
