#include <Input.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <CANFileStorage.h>


extern SDCard sdcard;
extern SDWriter file;
extern RTClock rtclock;
extern Blink blink;

extern float FileTime;
extern String FileName;

int can_restarts = 0;
String can_prevname; // previous file name
Input *can_aiinput = 0;


void setupGridStorage(const char *path, const char *software,
		      Input &aidata, char *gainstr) {
  can_prevname = "";
  can_restarts = 0;
  can_aiinput = &aidata;
  if (FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", path);
  file.setWriteInterval(2*can_aiinput->DMABufferTime());
  file.setMaxFileTime(FileTime);
  file.header().setSoftware(software);
  if (gainstr != 0)
    file.header().setGain(gainstr);
  file.start();
}


void openNextGridFile() {
  blink.setSingle();
  blink.blinkSingle(0, 1000);
  time_t t = now();
  String fname = rtclock.makeStr(FileName, t, true);
  if (fname != can_prevname) {
    file.sdcard()->resetFileCounter();
    can_prevname = fname;
  }
  fname = file.sdcard()->incrementFileName(fname);
  if (fname.length() == 0) {
    blink.clear();
    Serial.println("WARNING: failed to increment file name.");
    Serial.println("SD card probably not inserted.");
    Serial.println();
    can_aiinput->stop();
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
    can_aiinput->stop();
    while (1) { yield(); };
    return;
  }
  ssize_t samples = file.write();
  if (samples == -4) {   // overrun
    file.start(can_aiinput->nbuffer()/2);   // skip half a buffer
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


void storeGridData() {
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
        can_aiinput->stop();
        Serial.println("  No data available, data acquisition probably not running.");
        Serial.printf("  dmabuffertime = %.2fms, writetime = %.2fms\n", 1000.0*can_aiinput->DMABufferTime(), 1000.0*file.writeTime());
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
        can_aiinput->stop();
        while (1) {};
        break;
    }
    if (samples <= -3) {
      file.closeWave();
      char mfs[100];
      sprintf(mfs, "%s-error%d-%s.msg", file.baseName().c_str(), can_restarts+1, errorstr);
      Serial.println(mfs);
      File mf = sdcard.openWrite(mfs);
      mf.close();
      Serial.println();
    }
  }
  if (file.endWrite() || samples < 0) {
    file.close();  // file size was set by openWave()
    if (samples < 0) {
      can_restarts++;
      if (can_restarts >= 5) {
        Serial.println("ERROR: Too many file errors -> halt.");
        can_aiinput->stop();
        while (1) { yield(); };
      }
      if (!can_aiinput->running())
        can_aiinput->start();
      file.start();
    }
    openNextGridFile();
  }
}
