#include <R40CAN.h>


R40CAN::R40CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}


void R40CAN::begin() {
  CANBase<FlexCAN_T4, CAN2, CAN_message_t>::begin();
  Can.setBaudRate(1000000);
}

