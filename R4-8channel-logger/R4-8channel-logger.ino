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
#define GAIN 20.0            // dB

#define PATH          "recordings" // folder where to store the recordings
#define FILENAME      "grid1-SDATETIME.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 10*60 // seconds
#define INITIAL_DELAY  2.0   // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R4-8channel-logger v1.2"

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
  Serial.println("new file");
  if (! file.openWave(fname.c_str(), -1, dts)) {
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> halt");
    aidata.stop();
    while (1) { yield(); };
    return;
  }
  ssize_t samples = file.write();
  if (samples == -4) {   // overrun
    file.start(aidata.nbuffer()/2);   // skip half a buffer
    file.write();                     // write all available data
    // report overrun:
    char mfs[100];
    sprintf(mfs, "%s-error0-overrun.msg", file.baseName().c_str());
    Serial.println(mfs);
    File mf = sdcard.openWrite(mfs);
    mf.close();
  }
  Serial.println(file.name());
  blink.setSingle();
  blink.blinkSingle(0, 1000);
}


void setupStorage() {
  prevname = "";
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval(2*aidata.DMABufferTime());
  file.setMaxFileTime(settings.FileTime);
  file.header().setSoftware(SOFTWARE);
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
        aidata.stop();
        Serial.println("  No data available, data acquisition probably not running.");
        Serial.printf("  dmabuffertime = %.2fms, writetime = %.2fms\n", 1000.0*aidata.DMABufferTime(), 1000.0*file.writeTime());
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
        aidata.stop();
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
    if (samples < 0) {
      restarts++;
      if (restarts >= 5) {
        Serial.println("ERROR: Too many file errors -> halt.");
        aidata.stop();
        while (1) { yield(); };
      }
      if (!aidata.running())
        aidata.start();
      file.start();
    }
    String name = makeFileName();
    openNextFile(name);
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
  setupPCM(aidata, pcm1, false);
  setupPCM(aidata, pcm2, true);
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
