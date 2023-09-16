#include <Arduino.h>
#include <Blink.h>
#include <R41CAN.h>

R41CAN can;
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
  else
    blink.setSingle();
  while (1) {
    blink.update();
  };
  /*
  can.setMBFilter(REJECT_ALL);
  can.enableMBInterrupts();
  can.onReceive(canSniff);
  can.setMBFilter(MB0, 0x01);
  can.setMBFilter(MB1, 0x02);
  can.setMBFilter(MB2, 0x03);
  can.mailboxStatus();
  */
}


void loop() {
  //can.events();
}
