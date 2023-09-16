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


#endif
