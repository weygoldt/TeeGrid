#include <Arduino.h>
#include <FlexCAN_T4.h>


//FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> can;
FlexCAN_T4<CAN2, RX_SIZE_256, TX_SIZE_16> can;


//void canSniff(const CANFD_message_t &msg) {
void canSniff(const CAN_message_t &msg) {
  Serial.print("MB "); Serial.print(msg.mb);
  Serial.print("  OVERRUN: "); Serial.print(msg.flags.overrun);
  Serial.print("  LEN: "); Serial.print(msg.len);
  Serial.print(" EXT: "); Serial.print(msg.flags.extended);
  Serial.print(" TS: "); Serial.print(msg.timestamp);
  Serial.print(" ID: "); Serial.print(msg.id, HEX);
  Serial.print(" Buffer: ");
  for ( uint8_t i = 0; i < msg.len; i++ ) {
    Serial.print(msg.buf[i], HEX); Serial.print(" ");
  } Serial.println();
}


void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  can.begin();
  can.setBaudRate(1000000);
  can.setMBFilter(REJECT_ALL);
  can.enableMBInterrupts();
  can.onReceive(canSniff);
  can.setMBFilter(MB0, 0x01);
  can.setMBFilter(MB1, 0x02);
  can.setMBFilter(MB2, 0x03);
  can.mailboxStatus();
}


void loop() {
  can.events();
}
