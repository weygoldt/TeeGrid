#include <Arduino.h>
#include <R40CAN.h>

R40CAN can;


void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  can.begin();
  can.detectDevices();
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
