//#define SINGLE_FILE_MTP

#include <Input.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <Settings.h>
#include <FileStorage.h>
#ifdef SINGLE_FILE_MTP
#include <MTP_Teensy.h>
#endif


extern SDCard sdcard;
extern SDWriter file;
extern Settings settings;
extern RTClock rtclock;
extern Blink blink;


int restarts = 0;
String prevname; // previous file name
Input *aiinput = 0;


void setupStorage(const char *software, Input &aidata, char *gainstr) {
  prevname = "";
  restarts = 0;
  aiinput = &aidata;
  if (settings.fileTime() > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.path()))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.path());
  file.setWriteInterval(2*aiinput->DMABufferTime());
  file.setMaxFileTime(settings.fileTime());
  file.header().setSoftware(software);
  if (gainstr != 0)
    file.header().setGain(gainstr);
  file.start();
}


void openNextFile() {
  blink.setSingle();
  blink.blinkSingle(0, 2000);
  time_t t = now();
  String fname = rtclock.makeStr(settings.fileName(), t, true);
  if (fname != prevname) {
    file.sdcard()->resetFileCounter();
    prevname = fname;
  }
  fname = file.sdcard()->incrementFileName(fname);
  if (fname.length() == 0) {
    blink.clear();
    Serial.println("WARNING: failed to increment file name.");
    Serial.println("SD card probably not inserted.");
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
    Serial.println("SD card probably not inserted or full -> halt");
    aiinput->stop();
    while (1) { yield(); };
    return;
  }
  ssize_t samples = file.write();
  if (samples == -4) {   // overrun
    file.start(aiinput->nbuffer()/2);   // skip half a buffer
    file.write();                     // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", file.baseName().c_str());
    Serial.println(mfs);
    File mf = sdcard.openWrite(mfs);
    mf.close();
  }
  Serial.println(file.name());
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
        Serial.println("  SD card probably full -> halt");
        aiinput->stop();
        while (1) {};
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
        Serial.println("ERROR: Too many file errors -> halt.");
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
