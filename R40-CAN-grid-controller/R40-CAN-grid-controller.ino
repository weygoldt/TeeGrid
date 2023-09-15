#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <R40-CAN.h>


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


int detectDevices() {
  CAN_message_t msg;
  elapsedMillis timeout;

  Serial.println("Detect all devices:");
  digitalWrite(CAN_IO_DOWN_PIN, true);
  // clear device IDs:
  msg.id = CAN_ID_CLEAR_DEVICES;
  int r = can.write(msg);
  Serial.printf("  write clear message, r=%d\n", r);
  delay(1000);

  // assign device IDs:
  int id;
  for (id=1; ; id++) {
    Serial.printf("  check for ID=%d\n", id);
    msg.id = CAN_ID_FIND_DEVICES;
    int r = can.write(msg);
    Serial.printf("    write find message, r=%d\n", r);
    timeout = 0;
    msg.id = 0;
    while (!can.read(msg) && timeout < 1000) {
      delay(10);
    };
    if (msg.id != CAN_ID_REPORT_DEVICE) {
      Serial.println("    no device responded");
      break;
    }
    Serial.printf("    device reported id %d\n", id);
    delay(100);
  }
  msg.id = CAN_ID_GOT_DEVICES;
  r = can.write(msg);
  Serial.printf("  write got devices message, r=%d\n", r);
  digitalWrite(CAN_IO_DOWN_PIN, false);
  Serial.printf("  got %d devices\n", id-1);
  while (1) {};
  return id - 1;
}


void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  pinMode(CAN_IO_DOWN_PIN, OUTPUT);
  can.begin();
  can.setBaudRate(1000000);
  detectDevices();
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
