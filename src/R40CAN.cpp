#include <R40CAN.h>

#ifdef TEENSY4

R40CAN::R40CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}


void R40CAN::begin() {
  CAN_BASE::begin();
  Can.setBaudRate(1000000);
}

#endif
