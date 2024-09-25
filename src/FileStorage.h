/*
  FileStorage - High level handling of file storage of logger data.
  Created by Jan Benda, August 28th, 2023.
*/

#ifndef FileStorage_h
#define FileStorage_h

#include <Input.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <DeviceID.h>
#include <Blink.h>


class FileStorage {
  
public:

  FileStorage(Input &aiinput, SDCard &sdcard0, SDCard &sdcard1,
	      const RTClock &rtclock, const DeviceID &deviceid, Blink &blink);

  // Check accessibility of SD cards.
  // Halt if the main SD card can not be written.
  bool check(Stream &stream=Serial);

  // Report device identifier and current date and time.
  void report(Stream &stream=Serial) const;

  // Delay with double blinks for initial_delay seconds.
  void initialDelay(float initial_delay);

  // Initialize recording directory and firt files.
  void start(const char *path, const char *filename, float filetime,
	     const char *software, char *gainstr=0);

  // Write recorded data to files.
  void storeData();

  String baseName() const { return File0.baseName(); };


protected:

  // Provide timing and metadata to file.
  void setupFile(SDWriter &sdfile, float filetime,
		 const char *software, char *gainstr);
  
  // Generate file name, open main file and write first chunk of data.
  void openNextFile();
  
  // Open backup file and write first chunk of data.
  void openBackupFile();

  Input &AIInput;
  SDCard &SDCard0;
  SDCard &SDCard1;
  SDWriter File0;
  SDWriter File1;
  const RTClock &Clock;
  const DeviceID &DeviceIdent;
  Blink &BlinkLED;
  
  const char *Filename;  // Template for filename
  String PrevFilename;   // Previous file name
  int Restarts;
  
};


#endif

