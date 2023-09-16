#include <R41CAN.h>


R41CAN::R41CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}

  
void R41CAN::begin() {
  pinMode(CAN_STB_PIN, OUTPUT);
  pinMode(CAN_SHDN_PIN, OUTPUT);
  digitalWrite(CAN_STB_PIN, LOW);
  digitalWrite(CAN_SHDN_PIN, LOW);
  CANBase<FlexCAN_T4FD, CAN3, CANFD_message_t>::begin();
  CANFD_timings_t config;
  config.clock = CLK_24MHz;
  config.baudrate = 1000000;
  config.baudrateFD = 2000000;
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 70;
  Can.setBaudRate(config);
}


void R41CAN::powerDown() {
  digitalWrite(CAN_SHDN_PIN, HIGH);
}


void R41CAN::powerUp() {
  digitalWrite(CAN_SHDN_PIN, LOW);
}

