#ifndef R41_CAN_h
#define R41_CAN_h

#include <CANBase.h>

#define CAN_STB_PIN 36
#define CAN_SHDN_PIN 37

#define CAN_IO_UP_PIN 40
#define CAN_IO_DOWN_PIN 41


class R41CAN :
  public CANBase<FlexCAN_T4FD, CAN3, CANFD_message_t> {
  
public:

  R41CAN();

  void begin();
  
  void powerDown();
  void powerUp();
  
};


#endif
