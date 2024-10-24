/*
  LoggerFileStorage - High level handling of file storage of logger data.
  Created by Jan Benda, August 28th, 2023.
*/

#ifndef LoggerFileStorage_h
#define LoggerFileStorage_h

#include <Input.h>
#include <SDCard.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <DeviceID.h>
#include <Blink.h>


class LoggerFileStorage {
  
public:

  LoggerFileStorage(Input &aiinput, SDCard &sdcard0, SDCard &sdcard1,
		    const RTClock &rtclock, const DeviceID &deviceid,
		    Blink &blink);

  // Check accessibility of SD cards.
  // Halt if the main SD card can not be written.
  // If check_backup force checking backup SD card as well.
  bool check(bool check_backup, Stream &stream=Serial);

  // Report device identifier and current date and time.
  void report(Stream &stream=Serial) const;

  // Delay with double blinks for initial_delay seconds.
  void initialDelay(float initial_delay);

  // Initialize recording directory and firt files.
  void start(const char *path, const char *filename, float filetime,
	     const char *software, char *gainstr=0, bool randomblinks=false);

  // Call this in loop() for writing data to files.
  void update();

  String baseName() const { return File0.baseName(); };


protected:

  // Stop the sketch.
  void halt(Stream &stream=Serial);

  // Provide timing and metadata to file.
  void setup(SDWriter &sdfile, float filetime,
	     const char *software, char *gainstr);
  
  // Generate file name, open main file and write first chunk of data.
  void open(bool backup);
  
  // Open file that stores blink times.
  void openBlinkFiles();
  
  // Store blink times in files.
  void storeBlinks();

  // Write recorded data to files.
  bool store(SDWriter &sdfile, bool backup);

  // Derived classes can insert code here before the next file is opened.
  virtual bool synchronize() { return false; };

  Input &AIInput;
  SDCard &SDCard0;
  SDCard &SDCard1;
  SDWriter File0;
  SDWriter File1;
  const RTClock &Clock;
  const DeviceID &DeviceIdent;
  Blink &BlinkLED;
  
  bool RandomBlinks;
  FsFile BlinkFile0;
  FsFile BlinkFile1;
  
  const char *Filename;  // Template for filename
  String PrevFilename;   // Previous file name
  int FileCounter;
  int Restarts;
  int NextStore;
  int NextOpen;
  
};


#endif
