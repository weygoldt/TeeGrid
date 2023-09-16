#include <Arduino.h>
#include <RTClock.h>
#include <Blink.h>
#include <R41CAN.h>

R41CAN can;
RTClock rtclock;
Blink blink(LED_BUILTIN);


void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  blink.switchOff();
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
  can.receiveTime();
}


void loop() {
  can.events();
  blink.update();
}
