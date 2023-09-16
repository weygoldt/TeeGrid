#include <Arduino.h>
#include <RTClock.h>
#include <R41CAN.h>

R41CAN can;
RTClock rtclock;


void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  char dts[] = "2024-09-16T23:20:42";
  rtclock.set(dts);
  can.begin();
  can.detectDevices();
  //can.setupControllerMBs();
  delay(100);
  can.sendTime();
}


void loop() {
  can.events();
}
