#include <R41CAN.h>

#ifdef TEENSY4

R41CAN::R41CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}

  
void R41CAN::begin() {
  pinMode(CAN_STB_PIN, OUTPUT);
  pinMode(CAN_SHDN_PIN, OUTPUT);
  digitalWrite(CAN_STB_PIN, LOW);
  digitalWrite(CAN_SHDN_PIN, LOW);
  CAN_BASE::begin();
  CANFD_timings_t config;
  config.clock = CLK_24MHz;
  config.baudrate = 500000;
  config.baudrateFD = 2000000;
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 70;
  Can.setBaudRate(config);
}


int R41CAN::write20(CANFD_message_t &msg) {
  msg.brs = false;
  msg.edl = false;
  return Can.write(msg);
}


void R41CAN::powerDown() {
  digitalWrite(CAN_SHDN_PIN, HIGH);
}


void R41CAN::powerUp() {
  digitalWrite(CAN_SHDN_PIN, LOW);
}

#endif
