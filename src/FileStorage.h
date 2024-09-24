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

  bool check(Stream &stream=Serial);
  
  void report(Stream &stream=Serial) const;
  
  void initialDelay(float initial_delay);

  void start(const char *path, const char *filename, float filetime,
	     const char *software, char *gainstr=0);
  
  void openNextFile();
  
  void openBackupFile();
  
  void storeData();

  String baseName() const { return File0.baseName(); };


protected:
  
  void setupFile(SDWriter &sdfile, float filetime,
		 const char *software, char *gainstr);

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

