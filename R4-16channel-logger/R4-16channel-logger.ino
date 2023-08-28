//#define SINGLE_FILE_MTP

#include <Wire.h>
#include <ControlPCM186x.h>
#include <TeensyTDM.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <Configurator.h>
#include <Settings.h>
#include <TeensyTDMSettings.h>
#ifdef SINGLE_FILE_MTP
#include <MTP_Teensy.h>
#endif
#include <FileStorage.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define PREGAIN 10.0           // gain factor of preamplifier (1 or 20).
#define SAMPLING_RATE 48000 // samples per second and channel in Hertz
#define GAIN 20.0            // dB

#define PATH          "recordings"   // folder where to store the recordings
//#define FILENAME      "recNUM.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILENAME      "grid1-SDATETIME.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 2*60   // seconds
#define INITIAL_DELAY  2.0  // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R4-16channel-logger v1.2"

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
TeensyTDMSettings aisettings(&aidata, SAMPLING_RATE, GAIN);                  
RTClock rtclock;
Blink blink(LED_BUILTIN);
//Blink blink(31, true);


void setupPCM(TeensyTDM &tdm, ControlPCM186x &pcm, bool offs) {
  pcm.begin();
  pcm.setMicBias(false, true);
  pcm.setRate(tdm, aisettings.rate());
  if (PREGAIN == 1.0)
    pcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                 ControlPCM186x::CH4L, ControlPCM186x::CH4R, offs, true);
  else
    pcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
                 ControlPCM186x::CH2L, ControlPCM186x::CH2R, offs, true);  
  pcm.setGain(aisettings.gain());
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
  setupStorage(SOFTWARE);
  blink.switchOff();
  if (settings.InitialDelay >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*settings.InitialDelay)-1000);
  }
  else
    delay(uint32_t(1000.0*settings.InitialDelay));
  char gs[16];
  pcm1.gainStr(gs, PREGAIN);
  file.header().setGain(gs);
  file.start();
  openNextFile();
}


void loop() {
  storeData();
  blink.update();
}
