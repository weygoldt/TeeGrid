#ifndef R40CAN_h
#define R40CAN_h

#include <FlexCAN_T4.h>
#include <CANBase.h>
#include <TeensyBoard.h>

#ifdef TEENSY4

#define CAN_IO_UP_PIN 24
#define CAN_IO_DOWN_PIN 25

#define CAN_BASE CANBase<FlexCAN_T4, CAN2, CAN_message_t>


class R40CAN : public CAN_BASE {
  
public:

  R40CAN();

  void begin();
  
};

#endif

#endif
