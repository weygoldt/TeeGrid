#include <Wire.h>
#include <ControlPCM186x.h>
#include <InputTDM.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <Configurator.h>
#include <Settings.h>
#include <InputTDMSettings.h>
#include <FileStorage.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define NCHANNELS     8        // number of channels (2, 4, 8)
#define PREGAIN       10.0     // gain factor of preamplifier (1 or 10).
#define SAMPLING_RATE 96000    // samples per second and channel in Hertz
#define GAIN          20.0     // dB

#define PATH          "recordings"   // folder where to store the recordings
#define FILENAME      "grid1-SDATETIME.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 10   // seconds
#define INITIAL_DELAY  2.0  // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R40-logger v1.4"

DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
//EXT_DATA_BUFFER(AIBuffer, NAIBuffer, 32*512*256)
InputTDM aidata(AIBuffer, NAIBuffer);
#define NPCMS 2
ControlPCM186x pcm1(Wire, PCM186x_I2C_ADDR1, InputTDM::TDM1);
ControlPCM186x pcm2(Wire, PCM186x_I2C_ADDR2, InputTDM::TDM1);
ControlPCM186x *pcms[NPCMS] = {&pcm1, &pcm2};

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
InputTDMSettings aisettings(&aidata, SAMPLING_RATE, GAIN);                  
RTClock rtclock;
Blink blink(LED_BUILTIN);
//Blink blink(31, true);


bool setupPCM(InputTDM &tdm, ControlPCM186x &pcm, bool offs) {
  pcm.begin();
  bool r = pcm.setMicBias(false, true);
  if (!r)
    return false;
  pcm.setRate(tdm, aisettings.rate());
  if (tdm.nchannels() < NCHANNELS) {
    if (NCHANNELS - tdm.nchannels() == 2) {
      // two channels:
      if (PREGAIN == 1.0)
        pcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R, offs, true);
      else
        pcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R, offs, true);
    }
    else {
      // all four channels:
      if (PREGAIN == 1.0)
        pcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                     ControlPCM186x::CH4L, ControlPCM186x::CH4R, offs, true);
      else
        pcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
                     ControlPCM186x::CH2L, ControlPCM186x::CH2R, offs, true);
    }
    pcm.setGain(aisettings.gain());
    pcm.setFilters(ControlPCM186x::FIR, false);
  }
  else {
    // channels not recorded:
    pcm.powerdown();
  }
  return true;
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
  for (int k=0;k < NPCMS; k++)
    setupPCM(aidata, *pcms[k], k%2==1);
  aidata.begin();
  aidata.check();
  aidata.start();
  aidata.report();
  setupStorage(SOFTWARE, aidata);
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
