#define SINGLE_FILE_MTP

#include <Wire.h>
#include <ControlPCM186x.h>
#include <TeensyTDM.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <Configurator.h>
#include <Settings.h>
#ifdef SINGLE_FILE_MTP
#include <MTP_Teensy.h>
#endif

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file teegrid.cfg)
#define SAMPLING_RATE 48000  // samples per second and channel in Hertz
#define GAIN 20.0            // dB

#define PATH          "recordings"      // folder where to store the recordings
#define FILENAME      "grid1-SDATETIME" // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 10    // seconds
#define INITIAL_DELAY  2.0   // seconds

// ----------------------------------------------------------------------------

#define VERSION        "1.0"

RTClock rtclock;

DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
ControlPCM186x pcm1;
TeensyTDM aidata(AIBuffer, NAIBuffer);

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 100.0,
                  0.0, INITIAL_DELAY);
String prevname; // previous file name
Blink blink(LED_BUILTIN);

int restarts = 0;


String makeFileName() {
  time_t t = now();
  String name = rtclock.makeStr(settings.FileName, t, true);
  if (name != prevname) {
    file.sdcard()->resetFileCounter();
    prevname = name;
  }
  name = file.sdcard()->incrementFileName(name);
  if (name.length() == 0) {
    Serial.println("WARNING: failed to increment file name.");
    Serial.println("SD card probably not inserted.");
    Serial.println();
    return "";
  }
  return name;
}


bool openNextFile(const String &name) {
  blink.clear();
  if (name.length() == 0)
    return false;
  String fname = name + ".wav";
  char dts[20];
  rtclock.dateTime(dts);
  if (! file.openWave(fname.c_str(), -1, dts)) {
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> halt");
    aidata.stop();
    while (1) {};
    return false;
  }
  file.write();
  Serial.println(fname);
  blink.setSingle();
  blink.blinkSingle(0, 1000);
  return true;
}


void setupStorage() {
  prevname = "";
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval();
  file.setMaxFileTime(settings.FileTime);
  char ss[30] = "TeeGrid R4-8channel-logger v";
  strcat(ss, VERSION);
  file.header().setSoftware(ss);
}


void storeData() {
  if (file.pending()) {
    ssize_t samples = file.write();
    if (samples <= 0) {
      blink.clear();
      Serial.println();
      Serial.println("ERROR in writing data to file:");
      switch (samples) {
        case 0:
          Serial.println("  Nothing written into the file.");
          Serial.println("  SD card probably full -> halt");
          aidata.stop();
          while (1) {};
          break;
        case -1:
          Serial.println("  File not open.");
          break;
        case -2:
          Serial.println("  File already full.");
          break;
        case -3:
          Serial.println("  No data available, data acquisition probably not running.");
          Serial.println("  sampling rate probably too high,");
          Serial.println("  given the number of channels, averaging, sampling and conversion speed.");
          break;
      }
      if (samples == -3) {
        aidata.stop();
        file.closeWave();
        char mfs[20];
        sprintf(mfs, "error%d-%d.msg", restarts+1, -samples);
        File mf = sdcard.openWrite(mfs);
        mf.close();
      }
    }
    if (file.endWrite() || samples < 0) {
      file.close();  // file size was set by openWave()
#ifdef SINGLE_FILE_MTP
      MTP.begin();
      MTP.addFilesystem(SD, "Grid");
      while (true) {
        MTP.loop();
	yield();
      }
#endif      
      String name = makeFileName();
      if (samples < 0) {
        restarts++;
        if (restarts >= 5) {
          Serial.println("ERROR: Too many file errors -> halt.");
          aidata.stop();
          while (1) {};
        }
      }
      if (samples == -3) {
        aidata.start();
        file.start();
      }
      openNextFile(name);
    }
  }
}


// ----------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  rtclock.check();
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("teegrid.cfg");
  config.configure(sdcard);
  setupStorage();
  //aidata.configure(aisettings);
  Wire.begin();
  pcm1.begin();
  pcm1.setMicBias(false, true);
  pcm1.setupTDM(ControlPCM186x::CH1L, ControlPCM186x::CH1R, ControlPCM186x::CH2L, ControlPCM186x::CH2R, false);
  pcm1.setGain(ControlPCM186x::ADCLR, GAIN);
  pcm1.setFilters(ControlPCM186x::FIR, false);
  char ws[30];
  pcm1.channelsStr(ws);
  file.header().setChannels(ws);
  pcm1.gainStr(ControlPCM186x::ADC1L, ws);
  file.header().setGain(ws);
  aidata.setResolution(32);
  aidata.setRate(SAMPLING_RATE);
  aidata.setNChannels(4);   // TODO: take it from pcm!
  aidata.begin();
  aidata.check();
  aidata.start();
  aidata.report();
  blink.switchOff();
  if (settings.InitialDelay >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*settings.InitialDelay) - 1000);
  }
  else
    delay(uint32_t(1000.0*settings.InitialDelay));
  String name = makeFileName();
  if (name.length() == 0) {
    Serial.println("-> halt");
    aidata.stop();
    while (1) {};
  }
  file.start();
  openNextFile(name);
}


void loop() {
  storeData();
  blink.update();
}
