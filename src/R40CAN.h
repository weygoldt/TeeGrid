#ifndef R40CAN_h
#define R40CAN_h

#include <FlexCAN_T4.h>
#include <CANBase.h>

#define CAN_IO_UP_PIN 24
#define CAN_IO_DOWN_PIN 25


class R40CAN :
  public CANBase<FlexCAN_T4, CAN2, CAN_message_t> {
  
public:

  R40CAN();

  void begin();
  
};


R40CAN::R40CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}


void R40CAN::begin() {
  CANBase<FlexCAN_T4, CAN2, CAN_message_t>::begin();
  Can.setBaudRate(1000000);
}


#endif
