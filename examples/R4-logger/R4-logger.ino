#include <TeeGridBanner.h>
#include <Wire.h>
#include <ControlPCM186x.h>
#include <InputTDM.h>
#include <SDCard.h>
#include <RTClock.h>
#include <DeviceID.h>
#include <Blink.h>
#include <Configurator.h>
#include <Settings.h>
#include <InputTDMSettings.h>
#include <SetupPCM.h>
#include <FileStorage.h>
#include <R41CAN.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define NCHANNELS     16       // number of channels (even, from 2 to 16)
#define SAMPLING_RATE 48000    // samples per second and channel in Hertz
#define PREGAIN       1.0     // gain factor of preamplifier
#define GAIN          0.0     // dB

#define PATH          "recordings"   // folder where to store the recordings
#define DEVICEID      1              // may be used for naming files
#define FILENAME      "loggerID-SDATETIME.wav"  // may include ID, IDA, DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 20 //5*60   // seconds
#define INITIAL_DELAY  10.0  // seconds

// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R4-logger v1.6"
#define LED_PIN        26    // R4.1
//#define LED_PIN        27    // R4.2

#define SDCARD1_CS     10    // CS pin for second SD card on SPI bus

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

RTClock rtclock;
DeviceID deviceid(DEVICEID);
Blink blink(LED_PIN, true, LED_BUILTIN, false);
SDCard sdcard0;
SDCard sdcard1;

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
InputTDMSettings aisettings(SAMPLING_RATE, NCHANNELS, GAIN);                  

FileStorage files(aidata, sdcard0, sdcard1, rtclock, deviceid, blink);


// -----------------------------------------------------------------------------

void setup() {
  can.powerDown();
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  printTeeGridBanner(SOFTWARE);
  rtclock.check();
  sdcard0.begin();
  sdcard1.begin(SDCARD1_CS, DEDICATED_SPI, 20, &SPI);
  files.check();
  rtclock.setFromFile(sdcard0);
  settings.disable("PulseFrequency");
  settings.disable("DisplayTime");
  settings.disable("SensorsInterval");
  config.setConfigFile("logger.cfg");
  config.load(sdcard0);
  if (Serial)
    config.configure(Serial, 10000);
  config.report();
  Serial.println();
  aidata.setSwapLR();
  Wire.begin();
  Wire1.begin();
  for (int k=0;k < NPCMS; k++) {
    Serial.printf("Setup PCM186x %d on TDM %d: ", k, pcms[k]->TDMBus());
    R4SetupPCM(aidata, *pcms[k], k%2==1, aisettings, &pcm);
  }
  Serial.println();
  aidata.begin();
  if (!aidata.check()) {
    Serial.println("Fix ADC settings and check your hardware.");
    Serial.println("HALT");
    while (true) { yield(); };
  }
  aidata.start();
  aidata.report();
  blink.switchOff();
  files.report();
  files.initialDelay(settings.initialDelay());
  char gs[16];
  pcm->gainStr(gs, PREGAIN);
  files.start(settings.path(), settings.fileName(), settings.fileTime(),
              SOFTWARE, gs);
}


void loop() {
  files.storeData();
  blink.update();
}
