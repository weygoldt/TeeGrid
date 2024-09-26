// Define SINGLE_FILE_MTP to stop recording after the first file has
// been written, and make then the SD card available over USB as MTB
// filesystem.
//#define SINGLE_FILE_MTP

#include <FileStorage.h>
#ifdef SINGLE_FILE_MTP
#include <MTP_Teensy.h>
#endif


FileStorage::FileStorage(Input &aiinput, SDCard &sdcard0, SDCard &sdcard1,
			 const RTClock &rtclock, const DeviceID &deviceid,
			 Blink &blink) :
  AIInput(aiinput),
  SDCard0(sdcard0),
  SDCard1(sdcard1),
  File0(sdcard0, aiinput, 5),
  File1(sdcard1, aiinput, 5),
  Clock(rtclock),
  DeviceIdent(deviceid),
  BlinkLED(blink),
  Filename(NULL),
  PrevFilename(""),
  FileCounter(0),
  Restarts(0) {
}


bool FileStorage::check(Stream &stream) {
  if (!SDCard0.check(1e9)) {
    stream.println("HALT");
    SDCard0.end();
    BlinkLED.switchOff();
    while (true) { yield(); };
    return false;
  }
  if (SDCard1.available() && !SDCard1.check(SDCard0.free()))
    SDCard1.end();
  return true;
}

  
void FileStorage::report(Stream &stream) const {
  DeviceIdent.report(stream);
  Clock.report(stream);
}


void FileStorage::initialDelay(float initial_delay) {
  if (initial_delay >= 2.0) {
    delay(1000);
    BlinkLED.setDouble();
    BlinkLED.delay(uint32_t(1000.0*initial_delay) - 1000);
  }
  else
    delay(uint32_t(1000.0*initial_delay));
}


void FileStorage::setupFile(SDWriter &sdfile, float filetime,
			    const char *software, char *gainstr) {
  sdfile.setWriteInterval(2*AIInput.DMABufferTime());
  sdfile.setMaxFileTime(filetime);
  sdfile.header().setSoftware(software);
  if (gainstr != 0)
    sdfile.header().setGain(gainstr);
}


void FileStorage::start(const char *path, const char *filename,
			float filetime, const char *software,
			       char *gainstr) {
  Filename = filename;
  PrevFilename = "";
  Restarts = 0;
  if (filetime > 30)
    BlinkLED.setTiming(5000);
  else
    BlinkLED.setTiming(2000);
  if (File0.sdcard()->dataDir(path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", path);
  File1.sdcard()->dataDir(path);
  setupFile(File0, filetime, software, gainstr);
  setupFile(File1, filetime, software, gainstr);
  File0.start();
  File1.start(File0);
  openNextFile();
  openBackupFile();
}


void FileStorage::openNextFile() {
  BlinkLED.setSingle();
  BlinkLED.blinkSingle(0, 2000);
  String fname = DeviceIdent.makeStr(Filename);
  time_t t = now();
  fname = Clock.makeStr(fname, t, true);
  if (fname != PrevFilename) {
    File0.sdcard()->resetFileCounter();
    PrevFilename = fname;
  }
  fname = File0.sdcard()->incrementFileName(fname);
  if (fname.length() == 0) {
    BlinkLED.clear();
    Serial.println("WARNING: failed to increment file name.");
    Serial.println("SD card probably not inserted -> HALT");
    Serial.println();
    AIInput.stop();
    BlinkLED.switchOff();
    while (true) { yield(); };
    return;
  }
  char dts[20];
  Clock.dateTime(dts, t);
  if (! File0.openWave(fname.c_str(), -1, dts)) {
    BlinkLED.clear();
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> HALT");
    AIInput.stop();
    BlinkLED.switchOff();
    while (true) { yield(); };
    return;
  }
  FileCounter++;
  ssize_t samples = File0.write();
  if (samples == -4) {   // overrun
    File0.start(AIInput.nbuffer()/2);   // skip half a buffer
    File0.write();                      // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", File0.baseName().c_str());
    Serial.println(mfs);
    File mf = SDCard0.openWrite(mfs);
    mf.close();
  }
  Serial.println(File0.name());
}


void FileStorage::openBackupFile() {
  if (!File1.sdcard()->available())
    return;
  File1.openWave(File0.name().c_str(), File0.header());
  ssize_t samples = File1.write();
  if (samples == -4) {   // overrun
    File1.start(AIInput.nbuffer()/2);   // skip half a buffer
    File1.write();                      // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", File1.baseName().c_str());
    Serial.println(mfs);
    File mf = SDCard1.openWrite(mfs);
    mf.close();
  }
}


void FileStorage::storeData() {
  if (!File0.pending())
    return;
  ssize_t samples = File0.write();
  if (samples < 0) {
    BlinkLED.clear();
    Serial.println();
    Serial.println("ERROR in writing data to file in FileStorage::storeData():");
    char errorstr[20];
    switch (samples) {
      case -1:
        Serial.println("  File not open.");
        strcpy(errorstr, "notopen");
        break;
      case -2:
        Serial.println("  File already full.");
        strcpy(errorstr, "full");
        break;
      case -3:
        AIInput.stop();
        Serial.println("  No data available, data acquisition probably not running.");
        Serial.printf("  dmabuffertime = %.2fms, writetime = %.2fms\n", 1000.0*AIInput.DMABufferTime(), 1000.0*File0.writeTime());
        strcpy(errorstr, "nodata");
        break;
      case -4:
        Serial.println("  Buffer overrun.");
        Serial.printf("  dmabuffertime = %.2fms, writetime = %.2fms\n", 1000.0*AIInput.DMABufferTime(), 1000.0*File0.writeTime());
        strcpy(errorstr, "overrun");
        break;
      case -5:
        Serial.println("  Nothing written into the file.");
        Serial.println("  SD card probably full -> HALT");
        AIInput.stop();
	BlinkLED.switchOff();
        while (true) { yield(); };
        break;
    }
    File0.closeWave();
    // write error file:
    char mfs[100];
    sprintf(mfs, "%s-error%d-%s.msg", File0.baseName().c_str(), Restarts+1, errorstr);
    Serial.println(mfs);
    File mf = SDCard0.openWrite(mfs);
    mf.close();
    Serial.println();
    // halt after too many errors:
    Restarts++;
    Serial.printf("Incremented restarts to %d, samples=%d\n", Restarts, samples);
    if (Restarts >= 5) {
      Serial.println("ERROR in FileStorage::storeData(): too many file errors -> HALT");
      AIInput.stop();
      BlinkLED.switchOff();
      while (true) { yield(); };
    }
    // restart analog input:
    if (!AIInput.running())
      AIInput.start();
    // open next file:
    File0.start();
    openNextFile();
  }
  if (File0.endWrite()) {
    File0.close();  // file size was set by openWave()
#ifdef SINGLE_FILE_MTP
    AIInput.stop();
    delay(50);
    Serial.println();
    Serial.println("MTP file transfer.");
    Serial.flush();
    BlinkLED.setTriple();
    MTP.begin();
    MTP.addFilesystem(SDCard0, "logger");
    while (true) {
      MTP.loop();
      BlinkLED.update();
      yield();
    }
#endif      
    openNextFile();
  }
}
