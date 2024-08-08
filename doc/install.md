# Installation

## Installation via library manager of the Arduino IDE

You can install the TeeGrid library via the Library manager of the
Arduino IDE (Tools menu). Let it install all its dependencies (in
particular the [TeeRec library](https://github.com/janscience/TeeRec)
and the [ESensors
library](https://github.com/janscience/ESensors)). That's it!

Make sure that you have a recent [Arduino
IDE](https://www.arduino.cc/en/software) (version > 2.0) and the
corresponding [Teensy
support](https://www.pjrc.com/arduino-ide-2-0-0-teensy-support/) (> 1.58).
See [here](https://github.com/janscience/TeeRec/blob/main/docs/install.md)
for installation instructions for the Arduino IDE with Teensy support.


## Installation from github repositories

As an alternative to the installation via the library manager, you may
want to install the [TeeRec
library](https://github.com/janscience/TeeRec), the [ESensors
library](https://github.com/janscience/ESensors), and
[TeeGrid](https://github.com/janscience/TeeGrid) from their github
repositories, because you want to get the latest updates. Simply
clone these repositories into the `Arduino/libraries` folder.

Start with installing [TeeRec](https://github.com/janscience/TeeRec)
and [ESensors](https://github.com/janscience/ESensors). See the
[TeeRec installation
instructions](https://github.com/janscience/TeeRec/blob/main/docs/install.md)
and the [ESensors installation
instructions](https://github.com/janscience/ESensors/blob/main/docs/install.md)
for details.

### TeeGrid

For installing TeeGrid from github, clone the
[TeeGrid](https://github.com/janscience/TeeGrid) repository directly
into 'Arduino/libraries':
```sh
cd Arduino/libraries
git clone https://github.com/janscience/TeeGrid.git
```

Alternatively, download the whole repository as a zip archive (open
https://github.com/janscience/TeeGrid in your browser and click on the
green "Code" button). Unpack the zip file:
```sh
cd Arduino/libraries
unzip ~/Downloads/TeeGrid-main.zip
```

If you want to edit the TeeGrid files, mark the library as developmental:
```sh
cd Arduino/libraries/TeeGrid
touch .development
```

Close the Arduino IDE and open it again. Then the Arduino IDE knows
about the TeeGrid library.


## Upload new TeeGrid version to Arduino library manager

See
[TeeRec](https://github.com/janscience/TeeRec/blob/main/docs/install.md#arduino-ide-and-teensyduino)
for instructions on how to upload a new TeeGrid version to the Arduino
library manager.

