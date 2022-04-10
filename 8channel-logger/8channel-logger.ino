#include <Configurator.h>
#include <ContinuousADC.h>
#include <Sensors.h>
#include <Temperature.h>
#include <SDWriter.h>
#include <RTClock.h>
#include <Settings.h>
#include <Blink.h>
#include <TestSignals.h>


// Default settings: -----------------------------------------------------------------------
// (may be overwritten by config file teegrid.cfg)

uint32_t samplingRate = 20000;       // samples per second and channel in Hertz
int bits = 12;                       // resolution: 10bit 12bit, or 16bit
int averaging = 4;                   // number of averages per sample: 0, 4, 8, 16, 32
ADC_CONVERSION_SPEED convs = ADC_CONVERSION_SPEED::HIGH_SPEED;
ADC_SAMPLING_SPEED sampls = ADC_SAMPLING_SPEED::HIGH_SPEED;
int8_t channels0 [] =  {A4, A5, A6, A7, -1, A4, A5, A6, A7, A8, A9};      // input pins for ADC0
int8_t channels1 [] =  {A2, A3, A20, A22, -1, A20, A22, A12, A13};  // input pins for ADC1

uint8_t tempPin = 10;                    // pin for DATA of thermometer
float sensorsInterval = 10.0;             // interval between sensors readings in seconds

char path[] = "recordings";              // folder where to store the recordings
char fileName[] = "grid1-SDATETIME.wav"; // may include DATE, SDATE, TIME, STIME, DATETIME, SDATETIME, ANUM, NUM
float fileSaveTime = 10*60;              // seconds

float initialDelay = 10.0;               // seconds

int pulseFrequency = 230;                // Hertz
int signalPins[] = {9, 8, 7, 6, 5, 4, 3, 2, -1}; // pins where to put out test signals

// ------------------------------------------------------------------------------------------

const char version[4] = "2.0";

RTClock rtclock;
Configurator config;
ContinuousADC aidata;
Temperature temp;
Sensors sensors(rtclock);
SDCard sdcard;
SDWriter file(sdcard, aidata);
Settings settings(path, fileName, fileSaveTime, 100.0,
                  0.0, initialDelay);
String prevname; // previous file name
Blink blink(LED_BUILTIN);


void setupADC() {
  aidata.setChannels(0, channels0);
  aidata.setChannels(1, channels1);
  aidata.setRate(samplingRate);
  aidata.setResolution(bits);
  aidata.setAveraging(averaging);
  aidata.setConversionSpeed(convs);
  aidata.setSamplingSpeed(sampls);
  aidata.check();
}


void setupSensors() {
  if (!temp.available() && !temp.configured() && tempPin >= 0)
    temp.begin(tempPin);
  temp.setName("Twater");
  sensors.report();
  Serial.println();
}


String makeFileName() {
  time_t t = now();
  String name = rtclock.makeStr(settings.FileName, t, true);
  if (name != prevname) {
    file.resetFileCounter();
    prevname = name;
  }
  name = file.incrementFileName(name);
  if (name.length() == 0) {
    Serial.println("WARNING: failed to open file on SD card.");
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
  file.openWave(fname.c_str(), -1, dts);
  file.write();
  Serial.println(fname);
  if (file.isOpen()) {
    blink.setSingle();
    blink.blinkSingle(0, 1000);
    return true;
  }
  else {
    Serial.println();
    Serial.println("WARNING: failed to open file on SD card.");
    Serial.println("SD card probably not inserted.");
    return false;
  }
}


void setupStorage() {
  if (settings.FileTime > 30)
    blink.setTiming(5000);
  if (file.dataDir(settings.Path))
    Serial.printf("Save recorded data in folder \"%s\".\n\n", settings.Path);
  file.setWriteInterval();
  file.setMaxFileTime(settings.FileTime);
  char ss[30] = "TeeGrid 8channel-logger v";
  strcat(ss, version);
  file.setSoftware(ss);
}


void storeData() {
  if (file.pending()) {
    size_t samples = file.write();
    if (samples == 0) {
      blink.clear();
      Serial.println();
      Serial.println("ERROR: data acquisition not running.");
      Serial.println("sampling rate probably too high,");
      Serial.println("given the number of channels, averaging, sampling and conversion speed.");
      while (1) {};
    }
    if (file.endWrite()) {
      file.close();  // file size was set by openWave()
      String name = makeFileName();
      openNextFile(name);
    }
  }
}


// ------------------------------------------------------------------------------------------

void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  rtclock.check();
  prevname = "";
  setupADC();
  sensors.addSensor(temp);
  sensors.setInterval(sensorsInterval);
  sdcard.begin();
  rtclock.setFromFile(sdcard);
  rtclock.report();
  config.setConfigFile("teegrid.cfg");
  config.configure(sdcard);
  setupTestSignals(signalPins, pulseFrequency);
  setupStorage();
  setupSensors();
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
  String sname = name + "-temperatures";
  sensors.openCSV(sdcard, sname.c_str());
  sensors.start();
  file.start();
  openNextFile(name);
}


void loop() {
  storeData();
  sensors.update();
  if (file.writeTime() < 0.005 &&
      sensors.pending())
    sensors.writeCSV();
  blink.update();
}
