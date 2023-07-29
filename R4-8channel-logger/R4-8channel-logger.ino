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
// (may be overwritten by config file logger.cfg)
#define PREGAIN 1.0           // gain factor of a preamplifier.
#define SAMPLING_RATE 48000 // samples per second and channel in Hertz
#define GAIN 20.0            // dB

#define PATH          "recordings" // folder where to store the recordings
#ifdef SINGLE_FILE_MTP
#define FILENAME      "recording"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#else
#define FILENAME      "SDATELNUM"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#endif
#define FILE_SAVE_TIME 10   // seconds
#define INITIAL_DELAY  2.0  // seconds

// ----------------------------------------------------------------------------

#define VERSION        "1.0"

DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
ControlPCM186x pcm1(PCM186x_I2C_ADDR1);
ControlPCM186x pcm2(PCM186x_I2C_ADDR2);
TeensyTDM aidata(AIBuffer, NAIBuffer);

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
RTClock rtclock;
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


void openNextFile(const String &name) {
  blink.clear();
  if (name.length() == 0) {
    Serial.println();
    Serial.println("WARNING: no file name.");
    aidata.stop();
    while (1) { yield(); };
    return;
  }
  String fname = name + ".wav";
  char dts[20];
  rtclock.dateTime(dts);
  if (! file.openWave(fname.c_str(), -1, dts)) {
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> halt");
    aidata.stop();
    while (1) { yield(); };
    return;
  }
  file.write();
  Serial.println(fname);
  blink.setSingle();
  blink.blinkSingle(0, 1000);
}


void setupStorage() {
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval();
  file.setMaxFileTime(settings.FileTime);
  char ss[40] = "TeeGrid R4-8channel-logger v";
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
          break;
      }
      if (samples == -3) {
        aidata.stop();
        file.closeWave();
        char mfs[30];
        sprintf(mfs, "error%d-%d.msg", restarts+1, -samples);
        File mf = sdcard.openWrite(mfs);
        mf.close();
      }
    }
    if (file.endWrite() || samples < 0) {
      file.close();  // file size was set by openWave()
#ifdef SINGLE_FILE_MTP
      blink.clear();
      Serial.println();
      Serial.println("MTP file transfer.");
      Serial.flush();
      MTP.begin();
      MTP.addFilesystem(sdcard, "logger");
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
          while (1) { yield(); };
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


// -----------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  rtclock.check();
  prevname = "";
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("logger.cfg");
  config.configure(sdcard);
  aidata.setRate(SAMPLING_RATE);
  aidata.setSwapLR();
  aidata.begin();
  Wire.begin();
  pcm1.begin();
  pcm1.setMicBias(false, true);
  //pcm1.setupTDM(aidata, ControlPCM186x::CH1L, ControlPCM186x::CH1R, ControlPCM186x::CH2L, ControlPCM186x::CH2R, false);
  pcm1.setupTDM(aidata, ControlPCM186x::CH3L, ControlPCM186x::CH3R, ControlPCM186x::CH4L, ControlPCM186x::CH4R, false);
  pcm1.setGain(ControlPCM186x::ADCLR, GAIN);
  pcm1.setFilters(ControlPCM186x::FIR, false);
  pcm2.begin();
  pcm2.setMicBias(false, true);
  //pcm2.setupTDM(aidata, ControlPCM186x::CH1L, ControlPCM186x::CH1R, ControlPCM186x::CH2L, ControlPCM186x::CH2R, false);
  pcm2.setupTDM(aidata, ControlPCM186x::CH3L, ControlPCM186x::CH3R, ControlPCM186x::CH4L, ControlPCM186x::CH4R, true);
  pcm2.setGain(ControlPCM186x::ADCLR, GAIN);
  pcm1.setFilters(ControlPCM186x::FIR, false);
  aidata.check();
  aidata.start();
  aidata.report();
  setupStorage();
  blink.switchOff();
  if (settings.InitialDelay >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*settings.InitialDelay)-1000);
  }
  else
    delay(uint32_t(1000.0*settings.InitialDelay));
  char gs[16];
  pcm1.gainStr(ControlPCM186x::ADC1L, gs, PREGAIN);
  file.header().setGain(gs);
  String name = makeFileName();
  file.start();
  openNextFile(name);
}


void loop() {
  storeData();
  blink.update();
}
