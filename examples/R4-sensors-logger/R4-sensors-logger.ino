#include <TeeGridBanner.h>
#include <Wire.h>
#include <ControlPCM186x.h>
#include <InputTDM.h>
#include <SDCard.h>
#include <RTClock.h>
#include <DeviceID.h>
#include <Blink.h>
#include <Configurator.h>
#include <ToolActions.h>
#include <Settings.h>
#include <InputTDMSettings.h>
#include <SetupPCM.h>
#include <FileStorage.h>
#include <R41CAN.h>
#include <ESensors.h>
#include <TemperatureDS18x20.h>

// Default settings: ----------------------------------------------------------
// (may be overwritten by config file logger.cfg)
#define NCHANNELS        16       // number of channels (even, from 2 to 16)
#define SAMPLING_RATE    48000    // samples per second and channel in Hertz
#define PREGAIN          10.0     // gain factor of preamplifier
#define GAIN             0.0     // dB

#define PATH             "recordings"   // folder where to store the recordings
#define DEVICEID         1              // may be used for naming files
#define FILENAME         "loggerID-SDATETIME.wav"  // may include ID, IDA, DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, NUM, ANUM
#define FILE_SAVE_TIME   5*60    // seconds
#define INITIAL_DELAY    10.0    // seconds

#define LED_PIN          26    // R4.1
//#define LED_PIN        27    // R4.2

#define TEMP_PIN         35    // pin for DATA line of DS18x20 themperature sensor
#define SENSORS_INTERVAL 10.0  // interval between sensors readings in seconds

#define SDCARD1_CS       10    // CS pin for second SD card on SPI bus


// ----------------------------------------------------------------------------

#define SOFTWARE      "TeeGrid R4-sensors-logger v2.0"

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
Settings settings(PATH, DEVICEID, FILENAME, FILE_SAVE_TIME, 0.0,
                  0.0, INITIAL_DELAY);
InputTDMSettings aisettings(SAMPLING_RATE, NCHANNELS, GAIN);                  
Configurable datetime_menu("Date & time", Action::StreamInput);
ReportRTCAction report_rtc_act(datetime_menu, "Print date & time", rtclock);
SetRTCAction set_rtc_act(datetime_menu, "Set date & time", rtclock);
Configurable config_menu("Configuration", Action::StreamInput);
ReportConfigAction report_act(config_menu, "Print configuration");
SaveConfigAction save_act(config_menu,"Save configuration", sdcard0);
LoadConfigAction load_act(config_menu, "Load configuration", sdcard0);
RemoveConfigAction remove_act(config_menu, "Erase configuration", sdcard0);
Configurable sdcard_menu("SD card", Action::StreamInput);
SDInfoAction sdinfo_act(sdcard_menu, "SD card info", sdcard0);
SDCheckAction sdcheck_act(sdcard_menu, "SD card check", sdcard0);
SDBenchmarkAction sdbench_act(sdcard_menu, "SD card benchmark", sdcard0);
SDFormatAction format_act(sdcard_menu, "Format SD card", sdcard0);
SDEraseFormatAction eraseformat_act(sdcard_menu, "Erase and format SD card", sdcard0);
SDListRootAction listroot_act(sdcard_menu, "List files in root directory", sdcard0);
SDListRecordingsAction listrecs_act(sdcard_menu, "List all recordings", sdcard0, settings);
SDRemoveRecordingsAction eraserecs_act(sdcard_menu, "Erase all recordings", sdcard0, settings);
Configurable sdcard1_menu("Backup SD card", Action::StreamInput);
SDInfoAction sd1info_act(sdcard1_menu, "Backup SD card info", sdcard1);
SDCheckAction sd1check_act(sdcard1_menu, "Backup SD card check", sdcard1);
SDBenchmarkAction sd1bench_act(sdcard1_menu, "Backup SD card benchmark", sdcard1);
SDFormatAction format1_act(sdcard1_menu, "Format backup SD card", sdcard1);
SDEraseFormatAction eraseformat1_act(sdcard1_menu, "Erase and format backup SD card", sdcard1);
SDListRootAction listroot1_act(sdcard1_menu, "List files in root directory", sdcard1);
SDListRecordingsAction listrecs1_act(sdcard1_menu, "List all recordings", sdcard1, settings);
SDRemoveRecordingsAction eraserecs1_act(sdcard1_menu, "Erase all recordings", sdcard1, settings);
#ifdef FIRMWARE_UPDATE
Configurable firmware_menu("Firmware", Action::StreamInput);
ListFirmwareAction listfirmware_act(firmware_menu, "List available updates", sdcard0);
UpdateFirmwareAction updatefirmware_act(firmware_menu, "Update firmware", sdcard0);
#endif

ESensors sensors;

TemperatureDS18x20 temp(&sensors);

FileStorage files(aidata, sdcard0, sdcard1, rtclock, deviceid, blink);


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
  printTeeGridBanner(SOFTWARE);
  rtclock.check();
  sdcard0.begin();
  sdcard1.begin(SDCARD1_CS, DEDICATED_SPI, 20, &SPI);
  files.check();
  rtclock.setFromFile(sdcard0);
  settings.disable("PulseFrequency");
  settings.disable("DisplayTime");
  config.setConfigFile("logger.cfg");
  config.load(sdcard0);
  if (Serial)
    config.configure(Serial, 10000);
  config.report();
  Serial.println();
  setupSensors();
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
  String sfile = files.baseName();
  sfile.append("-sensors");
  sensors.openCSV(sdcard0, sfile.c_str());
}


void loop() {
  files.storeData();
  blink.update();
  if (sensors.update()) {
    sensors.writeCSV();
    sensors.print(true, true);
  }
}
