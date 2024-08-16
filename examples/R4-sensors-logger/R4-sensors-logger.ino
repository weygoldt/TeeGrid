#include <Wire.h>
#include <ControlPCM186x.h>
#include <InputTDM.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <Configurator.h>
#include <Settings.h>
#include <InputTDMSettings.h>
#include <SetupPCM.h>
#include <FileStorage.h>
#include <R41CAN.h>
#include <ESensors.h>
#include <TemperatureDS18x20.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define NCHANNELS     16       // number of channels (even, from 2 to 16)
#define SAMPLING_RATE 48000    // samples per second and channel in Hertz
#define PREGAIN       10.0     // gain factor of preamplifier
#define GAIN          0.0     // dB

#define PATH          "recordings"   // folder where to store the recordings
#define FILENAME      "logger1-SDATETIME.wav"  // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 5*60    // seconds
#define INITIAL_DELAY  10.0    // seconds

#define LED_PIN        26    // R4.1
//#define LED_PIN        27    // R4.2

#define TEMP_PIN         35     // pin for DATA line of DS18x20 themperature sensor
#define SENSORS_INTERVAL 10.0  // interval between sensors readings in seconds


// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R4-senors-logger v1.0"

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

Configurator config;
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
InputTDMSettings aisettings(SAMPLING_RATE, NCHANNELS, GAIN);                  
RTClock rtclock;
Blink blink(LED_PIN, true, LED_BUILTIN, false);

ESensors sensors;

TemperatureDS18x20 temp(&sensors);


void setupSensors() {
  temp.begin(TEMP_PIN);
  temp.setName("water-temperature");
  temp.setSymbol("T_water");
  sensors.setInterval(SENSORS_INTERVAL);
  sensors.setPrintTime(ESensors::ISO_TIME);
  sensors.report();
  sensors.start();
  sensors.read();
  sensors.start();
  sensors.read();
}


// -----------------------------------------------------------------------------

void setup() {
  can.powerDown();
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  Serial.println("\n=======================================================================\n");
  Serial.println(SOFTWARE);
  Serial.println();
  rtclock.check();
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  settings.disable("PulseFrequency");
  settings.disable("DisplayTime");
  config.setConfigFile("logger.cfg");
  config.load(sdcard);
  //if (Serial)
  //  config.configure(Serial);
  config.report();
  Serial.println();
  aidata.setSwapLR();
  setupSensors();
  Wire.begin();
  Wire1.begin();
  for (int k=0;k < NPCMS; k++) {
    Serial.printf("Setup PCM186x %d on TDM %d: ", k, pcms[k]->TDMBus());
    R4SetupPCM(aidata, *pcms[k], k%2==1, aisettings, &pcm);
  }
  Serial.println();
  aidata.begin();
  aidata.check();
  aidata.start();
  aidata.report();
  blink.switchOff();
  if (settings.initialDelay() >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*settings.initialDelay())-1000);
  }
  else
    delay(uint32_t(1000.0*settings.initialDelay()));
  char gs[16];
  pcm->gainStr(gs, PREGAIN);
  setupStorage(SOFTWARE, aidata, gs);
  openNextFile();
  String sfile = file.baseName();
  sfile.append("-sensors");
  sensors.openCSV(sdcard, sfile.c_str());
}


void loop() {
  storeData();
  blink.update();
  if (sensors.update()) {
    sensors.writeCSV();
    sensors.print(true, true);
  }
}
