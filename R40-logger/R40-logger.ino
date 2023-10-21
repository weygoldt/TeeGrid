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
// (may be overwritten by config file teegrid.cfg)
#define NCHANNELS     8        // number of channels (2, 4, 6, 8)
#define PREGAIN       10.0     // gain factor of preamplifier (1 or 10).
#define SAMPLING_RATE 48000    // samples per second and channel in Hertz
#define GAIN          20.0     // dB

#define PATH          "recordings"   // folder where to store the recordings
#define FILENAME      "logger1-SDATETIME.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 5*60   // seconds
#define INITIAL_DELAY  10.0  // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R40-logger v1.6"
#define LED_PIN       31

//DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
EXT_DATA_BUFFER(AIBuffer, NAIBuffer, 16*512*256)
InputTDM aidata(AIBuffer, NAIBuffer);
#define NPCMS 2
ControlPCM186x pcm1(Wire, PCM186x_I2C_ADDR1, InputTDM::TDM1);
ControlPCM186x pcm2(Wire, PCM186x_I2C_ADDR2, InputTDM::TDM1);
ControlPCM186x *pcms[NPCMS] = {&pcm1, &pcm2};
ControlPCM186x *pcm = 0;

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
InputTDMSettings aisettings(&aidata, SAMPLING_RATE, NCHANNELS, GAIN);                  
RTClock rtclock;
Blink blink(LED_PIN, true, LED_BUILTIN, false);


bool setupPCM(InputTDM &tdm, ControlPCM186x &cpcm, bool offs) {
  cpcm.begin();
  bool r = cpcm.setMicBias(false, true);
  if (!r) {
    Serial.println("not available");
    return false;
  }
  cpcm.setRate(tdm, aisettings.rate());
  if (tdm.nchannels() < aisettings.nchannels()) {
    if (aisettings.nchannels() - tdm.nchannels() == 2) {
      if (PREGAIN == 1.0) {
        cpcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
	              offs, ControlPCM186x::INVERTED);
        Serial.println("configured for 2 channels without preamplifier");
      }
      else {
        cpcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
	              offs, ControlPCM186x::INVERTED);
        Serial.printf("configured for 2 channels with preamplifier x%.0f\n", PREGAIN);
      }
    }
    else {
      if (PREGAIN == 1.0) {
        cpcm.setupTDM(tdm, ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                      ControlPCM186x::CH4L, ControlPCM186x::CH4R,
		      offs, ControlPCM186x::INVERTED);
        Serial.println("configured for 4 channels without preamplifier");
      }
      else {
        cpcm.setupTDM(tdm, ControlPCM186x::CH1L, ControlPCM186x::CH1R,
                      ControlPCM186x::CH2L, ControlPCM186x::CH2R,
		      offs, ControlPCM186x::INVERTED);
        Serial.printf("configured for 4 channels with preamplifier x%.0f\n", PREGAIN);
      }
    }
    cpcm.setSmoothGainChange(false);
    cpcm.setGain(aisettings.gain());
    cpcm.setFilters(ControlPCM186x::FIR, false);
    pcm = &cpcm;
  }
  else {
    // channels not recorded, but need to be configured to not corupt TDM bus:
    cpcm.setupTDM(ControlPCM186x::CH1L, ControlPCM186x::CH1R, offs);
    cpcm.powerdown();
    Serial.println("powered down");
  }
  return true;
}


// -----------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  Serial.println("\n=======================================================================\n");
  rtclock.check();
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("teegrid.cfg");
  config.configure(sdcard);
  aidata.setSwapLR();
  Wire.begin();
  for (int k=0;k < NPCMS; k++) {
    Serial.printf("Setup PCM186x %d: ", k);
    setupPCM(aidata, *pcms[k], k%2==1);
  }
  Serial.println();
  aidata.begin();
  aidata.check();
  aidata.start();
  aidata.report();
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
  setupStorage(SOFTWARE, aidata, gs);
  openNextFile();
}


void loop() {
  storeData();
  blink.update();
}
