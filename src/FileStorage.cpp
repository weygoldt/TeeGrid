//#define SINGLE_FILE_MTP

#include <Input.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <DeviceID.h>
#include <Blink.h>
#include <Settings.h>
#include <FileStorage.h>
#ifdef SINGLE_FILE_MTP
#include <MTP_Teensy.h>
#endif


extern SDCard sdcard;
extern SDWriter file;
extern SDCard sdcard1;
extern SDWriter file1;
extern Settings settings;
extern RTClock rtclock;
extern DeviceID deviceid;
extern Blink blink;


int restarts = 0;
String prevname; // previous file name
Input *aiinput = 0;


void setupFile(SDWriter &sdfile, const char *software, char *gainstr) {
  sdfile.setWriteInterval(2*aiinput->DMABufferTime());
  sdfile.setMaxFileTime(settings.fileTime());
  sdfile.header().setSoftware(software);
  if (gainstr != 0)
    sdfile.header().setGain(gainstr);
}


void setupStorage(const char *software, Input &aidata, char *gainstr) {
  
  prevname = "";
  restarts = 0;
  aiinput = &aidata;
  if (settings.fileTime() > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.path()))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.path());
  file1.sdcard()->dataDir(settings.path());
  setupFile(file, software, gainstr);
  setupFile(file1, software, gainstr);
  file.start();
  file1.start(file);
}


void openNextFile() {
  blink.setSingle();
  blink.blinkSingle(0, 2000);
  String fname = deviceid.makeStr(settings.fileName());
  time_t t = now();
  fname = rtclock.makeStr(fname, t, true);
  if (fname != prevname) {
    file.sdcard()->resetFileCounter();
    prevname = fname;
  }
  fname = file.sdcard()->incrementFileName(fname);
  if (fname.length() == 0) {
    blink.clear();
    Serial.println("WARNING: failed to increment file name.");
    Serial.println("SD card probably not inserted -> HALT");
    Serial.println();
    aiinput->stop();
    while (1) { yield(); };
    return;
  }
  char dts[20];
  rtclock.dateTime(dts, t);
  if (! file.openWave(fname.c_str(), -1, dts)) {
    blink.clear();
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> HALT");
    aiinput->stop();
    while (1) { yield(); };
    return;
  }
  ssize_t samples = file.write();
  if (samples == -4) {   // overrun
    file.start(aiinput->nbuffer()/2);   // skip half a buffer
    file.write();                       // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", file.baseName().c_str());
    Serial.println(mfs);
    File mf = sdcard.openWrite(mfs);
    mf.close();
  }
  Serial.println(file.name());
}


void openBackupFile() {
  if (!file1.sdcard()->available())
    return;
  file1.openWave(file.name().c_str(), file.header());
  ssize_t samples = file1.write();
  if (samples == -4) {   // overrun
    file1.start(aiinput->nbuffer()/2);   // skip half a buffer
    file1.write();                       // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", file1.baseName().c_str());
    Serial.println(mfs);
    File mf = sdcard1.openWrite(mfs);
    mf.close();
  }
}


void storeData() {
  if (!file.pending())
    return;
  ssize_t samples = file.write();
  if (samples < 0) {
    blink.clear();
    Serial.println();
    Serial.println("ERROR in writing data to file:");
    char errorstr[20];
    switch (samples) {
      case -1:
        Serial.println("  File not open.");
        break;
      case -2:
        Serial.println("  File already full.");
        break;
      case -3:
        aiinput->stop();
        Serial.println("  No data available, data acquisition probably not running.");
        Serial.printf("  dmabuffertime = %.2fms, writetime = %.2fms\n", 1000.0*aiinput->DMABufferTime(), 1000.0*file.writeTime());
        strcpy(errorstr, "nodata");
        delay(20);
        break;
      case -4:
        Serial.println("  Buffer overrun.");
        strcpy(errorstr, "overrun");
        break;
      case -5:
        Serial.println("  Nothing written into the file.");
        Serial.println("  SD card probably full -> HALT");
        aiinput->stop();
        while (1) { yield(); };
        break;
    }
    if (samples <= -3) {
      file.closeWave();
      char mfs[100];
      sprintf(mfs, "%s-error%d-%s.msg", file.baseName().c_str(), restarts+1, errorstr);
      Serial.println(mfs);
      File mf = sdcard.openWrite(mfs);
      mf.close();
      Serial.println();
    }
  }
  if (file.endWrite() || samples < 0) {
    file.close();  // file size was set by openWave()
#ifdef SINGLE_FILE_MTP
    aiinput->stop();
    delay(50);
    Serial.println();
    Serial.println("MTP file transfer.");
    Serial.flush();
    blink.setTriple();
    MTP.begin();
    MTP.addFilesystem(sdcard, "logger");
    while (true) {
      MTP.loop();
      blink.update();
      yield();
    }
#endif      
    if (samples < 0) {
      restarts++;
      if (restarts >= 5) {
        Serial.println("ERROR: Too many file errors -> HALT");
        aiinput->stop();
        while (1) { yield(); };
      }
      if (!aiinput->running())
        aiinput->start();
      file.start();
    }
    openNextFile();
  }
}
