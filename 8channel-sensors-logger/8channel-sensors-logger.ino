#include <TeensyADC.h>
#include <ESensors.h>
#include <TemperatureDS18x20.h>
#include <SenseBME280.h>
#include <LightTSL2591.h>
#include <DewPoint.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Blink.h>
#include <TestSignals.h>
#include <Configurator.h>
#include <Settings.h>
#include <TeensyADCSettings.h>


// Default settings: ----------------------------------------------------------
// (may be overwritten by config file teegrid.cfg)

#define SAMPLING_RATE 20000 // samples per second and channel in Hertz
#define BITS             12 // resolution: 10bit 12bit, or 16bit
#define AVERAGING         4 // number of averages per sample: 0, 4, 8, 16, 32
#define CONVERSION    ADC_CONVERSION_SPEED::HIGH_SPEED
#define SAMPLING      ADC_SAMPLING_SPEED::HIGH_SPEED
#define REFERENCE     ADC_REFERENCE::REF_3V3
int8_t channels0 [] =  {A4, A5, A6, A7, -1, A4, A5, A6, A7, A8, A9};      // input pins for ADC0
int8_t channels1 [] =  {A2, A3, A20, A22, -1, A20, A22, A12, A13};  // input pins for ADC1

#define TEMP_PIN         25   // pin for DATA of thermometer
#define SENSORS_INTERVAL 10.0 // interval between sensors readings in seconds

#define PATH          "recordings"      // folder where to store the recordings
#define FILENAME      "grid1-SDATETIME" // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
#define FILE_SAVE_TIME 10*60 // seconds
#define INITIAL_DELAY  2.0   // seconds

#define PULSE_FREQUENCY 230 // Hertz
int signalPins[] = {9, 8, 7, 6, 5, 4, 3, 2, -1}; // pins where to put out test signals

// ----------------------------------------------------------------------------

#define VERSION        "1.2"

RTClock rtclock;

DATA_BUFFER(AIBuffer, NAIBuffer, 256*256)
TeensyADC aidata(AIBuffer, NAIBuffer, channels0, channels1);

SDCard sdcard;
SDWriter file(sdcard, aidata);

Configurator config;
TeensyADCSettings aisettings(SAMPLING_RATE, BITS, AVERAGING,
			     CONVERSION, SAMPLING, REFERENCE);
Settings settings(PATH, FILENAME, FILE_SAVE_TIME, PULSE_FREQUENCY,
                  0.0, INITIAL_DELAY, SENSORS_INTERVAL);
Blink blink(LED_BUILTIN);

ESensors sensors;
TemperatureDS18x20 temp(&sensors);
SenseBME280 bme;
TemperatureBME280 tempbme(&bme, &sensors);
HumidityBME280 hum(&bme, &sensors);
DewPoint dp(&hum, &tempbme, &sensors);
PressureBME280 pres(&bme, &sensors);
LightTSL2591 tsl;
IRRatioTSL2591 irratio(&tsl, &sensors);
IlluminanceTSL2591 illum(&tsl, &sensors);

String prevname; // previous file name
int restarts = 0;


void setupSensors() {
  temp.begin(TEMP_PIN);
  temp.setName("water temperature", "Tw");
  Wire1.begin();
  bme.beginI2C(Wire1, 0x77);
  tempbme.setName("air temperature", "Ta");
  hum.setPercent();
  pres.setHecto();
  tsl.begin(Wire1);
  tsl.setGain(LightTSL2591::AUTO_GAIN);
  irratio.setPercent();
  sensors.setInterval(settings.SensorsInterval);
  sensors.setPrintTime(ESensors::ISO_TIME);
  sensors.report();
  Serial.println();
  // init sensors:
  sensors.start();
  sensors.read();
  tsl.setTemperature(bme.temperature());
  sensors.read();
  sensors.start();
}


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
    return "";
  }
  return name;
}


bool openNextFile(const String &name) {
  blink.clear();
  if (name.length() == 0)
    return false;
  String fname = name + ".wav";
  char dts[20];
  rtclock.dateTime(dts);
  if (! file.openWave(fname.c_str(), -1, dts)) {
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted or full -> halt");
    aidata.stop();
    while (1) {};
    return false;
  }
  file.write();
  Serial.println(fname);
  blink.setSingle();
  blink.blinkSingle(0, 1000);
  return true;
}


void setupStorage() {
  prevname = "";
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.sdcard()->dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval();
  file.setMaxFileTime(settings.FileTime);
  char ss[40] = "TeeGrid 8channel-sensors-logger v";
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
          Serial.println("  sampling rate probably too high,");
          Serial.println("  given the number of channels, averaging, sampling and conversion speed.");
          break;
      }
      if (samples == -3) {
        aidata.stop();
        file.closeWave();
        sensors.closeCSV();
        char mfs[20];
        sprintf(mfs, "error%d-%d.msg", restarts+1, -samples);
        SDFILE mf = sdcard.openWrite(mfs);
        mf.close();
      }
    }
    if (file.endWrite() || samples < 0) {
      file.close();  // file size was set by openWave()
      String name = makeFileName();
      if (samples < 0) {
        restarts++;
        if (restarts >= 5) {
          Serial.println("ERROR: Too many file errors -> halt.");
          aidata.stop();
          while (1) {};
        }
      }
      if (samples == -3) {
        String sname = name + "-sensors";
        sensors.openCSV(sdcard.sdcard(), sname.c_str());
        aidata.start();
        file.start();
      }
      openNextFile(name);
    }
  }
}


// ----------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  rtclock.check();
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("teegrid.cfg");
  config.configure(sdcard);
  setupTestSignals(signalPins, settings.PulseFrequency);
  setupStorage();
  setupSensors();
  aidata.configure(aisettings);
  aidata.check();
  aidata.start();
  aidata.report();
  blink.switchOff();
  if (settings.InitialDelay >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*settings.InitialDelay) - 1000);
  }
  else
    delay(uint32_t(1000.0*settings.InitialDelay));
  String name = makeFileName();
  if (name.length() == 0) {
    Serial.println("-> halt");
    aidata.stop();
    while (1) {};
  }
  String sname = name + "-sensors";
  sensors.openCSV(sdcard.sdcard(), sname.c_str());
  sensors.start();
  file.start();
  openNextFile(name);
}


void loop() {
  storeData();
  if (sensors.update()) {
    tsl.setTemperature(bme.temperature());
    sensors.print();
  }
  if (file.writeTime() < 0.01 &&
      sensors.pendingCSV())
    sensors.writeCSV();
  blink.update();
}
