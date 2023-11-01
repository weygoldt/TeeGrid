#ifndef R41_CAN_h
#define R41_CAN_h

#include <CANBase.h>
#include <TeensyBoard.h>

#ifdef TEENSY4

#define CAN_STB_PIN 36
#define CAN_SHDN_PIN 37

#define CAN_IO_UP_PIN 41
#define CAN_IO_DOWN_PIN 40

#define CAN_BASE CANBase<FlexCAN_T4FD, CAN3, CANFD_message_t>


class R41CAN : public CAN_BASE {
  
public:

  R41CAN();

  void begin();
  
  // write CAN2.0 messages (brs = edl = false)
  virtual int write20(CANFD_message_t &msg);
  
  void powerDown();
  void powerUp();
  
};


#endif

#endif
