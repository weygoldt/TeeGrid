#include <Arduino.h>
#include <RTClock.h>
#include <Blink.h>
#include <R41CAN.h>


#define GRID              "grid1"     // unique name of the grid
#define SAMPLING_RATE     96000      // sampling rate in Hz
#define GAIN              20.0       // gain in dB
#define FILE_TIME         20.0       // seconds
#define INITIAL_DELAY     20.0       // seconds

R41CAN can;
RTClock rtclock;
Blink blink(LED_BUILTIN);

elapsedMillis Time;


void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  char dts[] = "2024-09-16T23:20:42";
  rtclock.set(dts);
  can.begin();
  can.detectDevices();
  // stop if no device have been found
  //can.setupControllerMBs();
  delay(100);
  can.sendGrid(GRID);
  can.sendTime();
  can.sendSamplingRate(SAMPLING_RATE);
  can.sendGain(GAIN);
  can.sendFileTime(FILE_TIME);
  blink.switchOff();
  if (INITIAL_DELAY >= 2.0) {
    delay(1000);
    blink.setDouble();
    blink.delay(uint32_t(1000.0*INITIAL_DELAY)-1000);
  }
  else
    delay(uint32_t(1000.0*INITIAL_DELAY));
  Time = 0;
  can.sendStart();
  blink.setSingle();
}


void loop() {
  can.events();
  blink.update();
  if (0.001*Time > FILE_TIME - 0.05) {
    can.receiveEndFile();
    can.sendStart();
    Time = 0;
    blink.setSingle();
  }
}
