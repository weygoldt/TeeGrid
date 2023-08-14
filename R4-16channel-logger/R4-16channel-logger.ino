//#define SINGLE_FILE_MTP

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
#define PREGAIN 10.0           // gain factor of preamplifier (1 or 20).
#define SAMPLING_RATE 48000 // samples per second and channel in Hertz
#define GAIN 0.0            // dB

#define PATH          "recordings"   // folder where to store the recordings
#define FILENAME      "FrecNUM.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
//#define FILENAME      "SDATELNUM.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 10*60   // seconds
#define INITIAL_DELAY  2.0  // seconds

// ----------------------------------------------------------------------------

#define VERSION        "1.2"

DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
TeensyTDM aidata(AIBuffer, NAIBuffer);
ControlPCM186x pcm1(Wire, PCM186x_I2C_ADDR1, TeensyTDM::TDM1);
ControlPCM186x pcm2(Wire, PCM186x_I2C_ADDR2, TeensyTDM::TDM1);
ControlPCM186x pcm3(Wire1, PCM186x_I2C_ADDR1, TeensyTDM::TDM2);
ControlPCM186x pcm4(Wire1, PCM186x_I2C_ADDR2, TeensyTDM::TDM2);

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
RTClock rtclock;
String prevname; // previous file name
Blink blink(LED_BUILTIN);
//Blink blink(31, true);

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
    aidata.stop();
    while (1) { yield(); };
    return "";
  }
  return name;
}


void openNextFile(const String &fname) {
  blink.clear();
  if (fname.length() == 0)
    return;
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
  prevname = "";
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval(0.02);
  file.setMaxFileTime(settings.FileTime);
  char ss[40] = "TeeGrid R4-16channel-logger v";
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
      aidata.stop();
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


void setupPCM(TeensyTDM &tdm, ControlPCM186x &pcm, bool offs) {
  pcm.begin();
  pcm.setMicBias(false, true);
  pcm.setRate(tdm, SAMPLING_RATE);
  if (PREGAIN == 1.0)
    pcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                 ControlPCM186x::CH4L, ControlPCM186x::CH4R, offs, true);
  else
    pcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
                 ControlPCM186x::CH2L, ControlPCM186x::CH2R, offs, true);  
  pcm.setGain(ControlPCM186x::ADCLR, GAIN);
  pcm.setFilters(ControlPCM186x::FIR, false);
}


// -----------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  rtclock.check();
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("logger.cfg");
  config.configure(sdcard);
  aidata.setSwapLR();
  Wire.begin();
  Wire1.begin();
  setupPCM(aidata, pcm1, false);
  setupPCM(aidata, pcm2, true);
  setupPCM(aidata, pcm3, false);
  setupPCM(aidata, pcm4, true);
  aidata.begin();
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
