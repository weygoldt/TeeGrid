#ifndef R41_CAN_h
#define R41_CAN_h

#include <CANBase.h>

#define CAN_IO_UP_PIN 41
#define CAN_IO_DOWN_PIN 40


class R41CAN :
  public CANBase<FlexCAN_T4FD, CAN3, CANFD_message_t> {
  
public:

  R41CAN();

  void begin();
  
};

  
R41CAN::R41CAN() :
  CANBase(CAN_IO_UP_PIN, CAN_IO_DOWN_PIN) {
}

  
void R41CAN::begin() {
  CANBase<FlexCAN_T4FD, CAN3, CANFD_message_t>::begin();
  //Can.setBaudRate(1000000);
  #warning need to implement baud rate
}


#endif
