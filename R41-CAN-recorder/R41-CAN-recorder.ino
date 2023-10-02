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
float   Gain         = 40.0;    // dB

#define PATH           "recordings"   // folder where to store the recordings
String  FileName     = "GRID-SDATETIME-RECCOUNT-DEV.wav";  // may include GRID, DEV, COUNT, DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
String LoggerFileName = "loggergrid-RECNUM4-DEV.wav";
float   FileTime     = 30.0;  // seconds
#define GRID           "grid1"
#define DEV            "D"
#define INITIAL_DELAY  20.0    // seconds

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
  if (can.id() > 0 )
    blink.setMultiple(can.id());
  else
    blink.switchOff();
  //can.setupRecorderMBs();
  char gs[32];
  can.receiveGrid(gs);
  if (strlen(gs) == 0 || can.id() > 0)
    strncpy(gs, GRID, 32);
  else
    Serial.printf("  got grid name %s\n", gs);
  can.receiveTime();
  int rate = can.receiveSamplingRate();
  if (rate > 0 || can.id() > 0) {
    SamplingRate = rate;
    Serial.printf("  got %.0fHz sampling rate\n", SamplingRate);
  }
  float gain = can.receiveGain();
  if (gain > -1000 || can.id() > 0) {
    Gain = gain;
    Serial.printf("  got gain of %.1fdB\n", Gain);
  }
  float time = can.receiveFileTime();
  if (time > 0.0 && can.id() > 0)
    FileTime = time;
  if (can.id() == 0)
    FileName = LoggerFileName;
  FileName.replace("GRID", gs);
  if (can.id() == 0)
    FileName.replace("DEV", DEV);
  else {
    char devs[2];
    devs[1] = '\0';
	  devs[0] = char('A' + can.id() - 1);
    FileName.replace("DEV", devs);
  }
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
  blink.setTiming(3000);
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  blink.switchOff();
  rtclock.check();
  sdcard.begin();
  rtclock.report();
  setupCAN();  
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
  char gs[16];
  pcm->gainStr(gs, PREGAIN);
  setupGridStorage(PATH, SOFTWARE, aidata, gs);
  if (can.id() > 0)
    can.receiveStart();
  else {
    blink.switchOff();
    if (INITIAL_DELAY >= 2.0) {
      delay(1000);
      blink.setDouble();
      blink.delay(uint32_t(1000.0*INITIAL_DELAY)-1000);
    }
    else
      delay(uint32_t(1000.0*INITIAL_DELAY));
  }
  openNextGridFile();
}

void loop() {
  storeGridData();
  can.events();
  blink.update();
}
