#include <Wire.h>
#include <ControlPCM186x.h>
#include <InputTDM.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <CANFileStorage.h>
#include <R41CAN.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define NCHANNELS      16       // number of channels (even, from 2 to 16)
#define PREGAIN        10.0     // gain factor of preamplifier
int     SamplingRate = 48000;   // samples per second and channel in Hertz
float   Gain         = 20.0;    // dB

#define PATH           "recordings"   // folder where to store the recordings
String  FileName     = "GRIDDEV-SDATETIME.wav";  // may include GRID, DEV, DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
float   FileTime     = 10.0;  // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R41-CAN-recorder v1.0"

//DATA_BUFFER(AIBuffer, NAIBuffer, 512*256)
EXT_DATA_BUFFER(AIBuffer, NAIBuffer, 16*512*256)
InputTDM aidata(AIBuffer, NAIBuffer);
#define NPCMS 4
ControlPCM186x pcm1(Wire, PCM186x_I2C_ADDR1, InputTDM::TDM1);
ControlPCM186x pcm2(Wire, PCM186x_I2C_ADDR2, InputTDM::TDM1);
ControlPCM186x pcm3(Wire1, PCM186x_I2C_ADDR1, InputTDM::TDM2);
ControlPCM186x pcm4(Wire1, PCM186x_I2C_ADDR2, InputTDM::TDM2);
ControlPCM186x *pcms[NPCMS] = {&pcm1, &pcm2, &pcm3, &pcm4};
ControlPCM186x *pcm = 0;

R41CAN can;
SDCard sdcard;
SDWriter file(sdcard, aidata);
RTClock rtclock;
Blink blink(LED_BUILTIN);
//Blink blink(31, true);


void setupCAN() {
  can.begin();
  can.assignDevice();
  if (can.id() == 3)
    blink.setTriple();
  else if (can.id() == 2)
    blink.setDouble();
  else if (can.id() == 1)
    blink.setSingle();
  else
    blink.switchOff();
  //can.setupRecorderMBs();
  char gs[32];
  can.receiveGrid(gs);
  can.receiveTime();
  SamplingRate = can.receiveSamplingRate();
  Gain = can.receiveGain();
  FileTime = can.receiveFileTime();
  FileName.replace("GRID", gs);
  char devs[8];
  sprintf(devs, "%02d", can.id());
  FileName.replace("DEV", devs);
}


bool setupPCM(InputTDM &tdm, ControlPCM186x &cpcm, bool offs) {
  cpcm.begin();
  bool r = cpcm.setMicBias(false, true);
  if (!r) {
    Serial.println("not available");
    return false;
  }
  cpcm.setRate(tdm, SamplingRate);
  if (tdm.nchannels() < NCHANNELS) {
    if (NCHANNELS - tdm.nchannels() == 2) {
      cpcm.setupTDM(tdm, ControlPCM186x::CH2L, ControlPCM186x::CH2R,
                    offs, ControlPCM186x::INVERTED);
      Serial.println("configured for 2 channels");
    }
    else {
      cpcm.setupTDM(tdm, ControlPCM186x::CH2L, ControlPCM186x::CH2R,
                    ControlPCM186x::CH3L, ControlPCM186x::CH3R,
                    offs, ControlPCM186x::INVERTED);
      Serial.println("configured for 4 channels");
    }
    cpcm.setSmoothGainChange(false);
    cpcm.setGain(Gain);
    cpcm.setFilters(ControlPCM186x::FIR, false);
    pcm = &cpcm;
  }
  else {
    // channels not recorded:
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
  setupCAN();  
  rtclock.check();
  sdcard.begin();
  rtclock.report();
  aidata.setSwapLR();
  Wire.begin();
  Wire1.begin();
  for (int k=0;k < NPCMS; k++) {
    Serial.printf("Setup PCM186x %d on TDM %d: ", k, pcms[k]->TDMBus());
    setupPCM(aidata, *pcms[k], k%2==1);
  }
  Serial.println();
  aidata.begin();
  aidata.check();
  aidata.start();
  aidata.report();
  blink.switchOff();
  char gs[16];
  pcm->gainStr(gs, PREGAIN);
  setupGridStorage(PATH, SOFTWARE, aidata, gs);
  can.receiveStart();
  blink.setSingle();
  openNextGridFile();
}


void loop() {
  storeGridData();
  can.events();
  blink.update();
}
