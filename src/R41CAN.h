#ifndef R41_CAN_h
#define R41_CAN_h

#include <CANBase.h>

#define CAN_IO_UP_PIN 40
#define CAN_IO_DOWN_PIN 41


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
  CANFD_timings_t config;
  config.clock = CLK_24MHz;
  config.baudrate = 1000000;
  config.baudrateFD = 2000000;
  config.propdelay = 190;
  config.bus_length = 1;
  config.sample = 70;
  Can.setBaudRate(config);
}


#endif
