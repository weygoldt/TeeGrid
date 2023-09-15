#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <Blink.h>
#include <R41-CAN.h>


FlexCAN_T4FD<CAN3, RX_SIZE_256, TX_SIZE_16> can;

CANFD_message_t msg;
int msg_id = 0;
elapsedMillis time_passed;
int DeviceID = 0;

Blink blink(LED_BUILTIN);


void canSniff(const CANFD_message_t &msg) {
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


void assignDevice() {
  CANFD_message_t msg;
  elapsedMillis timeout;

  Serial.println("Setting up device ID:");
  // clear device IDs:
  Serial.println("  wait for clear devices command");
  timeout = 0;
  msg.id = 0;
  while (!can.read(msg) && timeout < 100000) {
    delay(10);
  };
  Serial.printf("  got message 0x%02x\n", msg.id);
  if (msg.id != CAN_ID_CLEAR_DEVICES)
    return;
  DeviceID = 0;
  digitalWrite(CAN_IO_DOWN_PIN, false);

  // assign device ID:
  for (int id=1; ; id++) {
    Serial.printf("  check for ID=%d\n", id);
    timeout = 0;
    msg.id = 0;
    Serial.printf("    wait for find devices\n");
    while ((!can.read(msg) || msg.id == CAN_ID_REPORT_DEVICE) && timeout < 10000) {
      delay(10);
    };
    Serial.printf("    got message 0x%02x\n", msg.id);
    if (msg.id != CAN_ID_FIND_DEVICES)
      break;
    if (digitalRead(CAN_IO_UP_PIN)) {
      DeviceID = id;
      Serial.printf("    assign ID %d\n", DeviceID);
      msg.id = CAN_ID_REPORT_DEVICE;
      int r = can.write(msg);
      Serial.printf("    write found message, r=%d\n", r);
      delay(10);
      digitalWrite(CAN_IO_DOWN_PIN, true);
      break;
    }
    else {
      Serial.println("    IO pin is low");
      delay(10);
    }
  }
  Serial.println("  wait for all devices to be detected");
  while (!can.read(msg) || msg.id != CAN_ID_GOT_DEVICES) {
    delay(10);
  };
  digitalWrite(CAN_IO_DOWN_PIN, false);
  Serial.println("  done");
  if (DeviceID == 2)
    blink.setDouble();
  else
    blink.setSingle();
  while (1) {
    blink.update();
  };
}


void setup() {
  blink.switchOn();
  Serial.begin(9600);
  while (!Serial && millis() < 2000) {};
  blink.switchOff();
  pinMode(CAN_IO_UP_PIN, INPUT);
  pinMode(CAN_IO_DOWN_PIN, OUTPUT);
  can.begin();
  can.setBaudRate(1000000);
  assignDevice();
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
  if (time_passed > 2000) {
    msg.id = msg_id;
    sprintf((char *)msg.buf, "ID%d", msg_id);
    int r = can.write(msg);
    Serial.printf("write message id=%d, r=%d\n", msg_id, r);
    msg_id++;
    if (msg_id > 4)
      msg_id = 0;
    time_passed = 0;
  }
}
