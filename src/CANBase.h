#ifndef CANBase_h
#define CANBase_h

#include <Arduino.h>
#include <FlexCAN_T4.h>

#define CAN_ID_CLEAR_DEVICES 0x01
#define CAN_ID_FIND_DEVICES  0x02
#define CAN_ID_REPORT_DEVICE 0x03
#define CAN_ID_GOT_DEVICES   0x04


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
class CANBase {
  
public:

  CANBase(uint8_t up_pin, uint8_t down_pin);

  void begin();

  int id() const { return DeviceID; };

  // write CAN2.0 messages (brs = edl = false)
  virtual int write20(CAN_MSG &msg);
  
  int detectDevices();
  void assignDevice();


protected:

  CANCLASS<BUS, RX_SIZE_256, TX_SIZE_16> Can;
  uint8_t UpPin;
  uint8_t DownPin;

  int DeviceID;
  
};


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
CANBase<CANCLASS, BUS, CAN_MSG>::CANBase(uint8_t up_pin, uint8_t down_pin) :
  UpPin(up_pin),
  DownPin(down_pin),
  DeviceID(0) {
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::begin() {
  pinMode(UpPin, INPUT);
  pinMode(DownPin, OUTPUT);
  digitalWrite(DownPin, LOW);
  Can.begin();
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
int CANBase<CANCLASS, BUS, CAN_MSG>::write20(CAN_MSG &msg) {
  return Can.write(msg);
}

  
template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
int CANBase<CANCLASS, BUS, CAN_MSG>::detectDevices() {
  CAN_MSG msg;
  elapsedMillis timeout;

  Serial.println("Detect all devices:");
  digitalWrite(DownPin, HIGH);
  // clear device IDs:
  msg.id = CAN_ID_CLEAR_DEVICES;
  int r = write20(msg);
  Serial.printf("  write clear message, r=%d\n", r);
  delay(10);

  // assign device IDs:
  int id;
  for (id=1; ; id++) {
    Serial.printf("  check for ID=%d\n", id);
    msg.id = CAN_ID_FIND_DEVICES;
    *(int *)(&msg.buf[0]) = id;
    int r = write20(msg);
    Serial.printf("    write find message, r=%d\n", r);
    timeout = 0;
    msg.id = 0;
    while (!Can.read(msg) && timeout < 1000) {
      delay(10);
    };
    if (msg.id != CAN_ID_REPORT_DEVICE) {
      Serial.println("    no device responded");
      break;
    }
    int devid = *(int *)(&msg.buf[0]);
    Serial.printf("    device reported id %d\n", devid);
    if (devid != id)
      Serial.println("WARNING reported device id does not match expectation!");
    delay(10);
  }
  msg.id = CAN_ID_GOT_DEVICES;
  r = write20(msg);
  Serial.printf("  write got devices message, r=%d\n", r);
  digitalWrite(DownPin, LOW);
  delay(10);
  Serial.printf("  got %d devices\n", id-1);
  Serial.println();
  return id - 1;
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::assignDevice() {
  CAN_MSG msg;
  elapsedMillis timeout;

  Serial.println("Setting up device ID:");
  // clear device IDs:
  Serial.printf("  wait for clear devices command 0x%02x\n", CAN_ID_CLEAR_DEVICES);
  timeout = 0;
  msg.id = 0;
  while (!Can.read(msg) && timeout < 100000) {
    delay(10);
  };
  Serial.printf("  got message 0x%02x\n", msg.id);
  if (msg.id != CAN_ID_CLEAR_DEVICES) {
    Serial.println("  timeout");
    Serial.println();
    return;
  }
  DeviceID = 0;
  digitalWrite(DownPin, LOW);

  // assign device ID:
  while (true) {
    timeout = 0;
    msg.id = 0;
    Serial.printf("  wait for find devices command 0x%02x\n", CAN_ID_FIND_DEVICES);
    while ((!Can.read(msg) || msg.id == CAN_ID_REPORT_DEVICE) && timeout < 1000) {
      delay(10);
    };
    Serial.printf("    got message 0x%02x\n", msg.id);
    if (msg.id != CAN_ID_FIND_DEVICES)
      break;
    if (digitalRead(UpPin)) {
      DeviceID = *(int *)(&msg.buf[0]);
      Serial.printf("    assign ID %d\n", DeviceID);
      msg.id = CAN_ID_REPORT_DEVICE;
      *(int *)(&msg.buf[0]) = DeviceID;
      int r = write20(msg);
      Serial.printf("    write report device message, r=%d\n", r);
      delay(10);
      digitalWrite(DownPin, HIGH);
      break;
    }
    else {
      Serial.println("    IO pin is low");
      delay(10);
    }
  }
  Serial.println("  wait for all devices to be detected");
  while (!Can.read(msg) || msg.id != CAN_ID_GOT_DEVICES) {
    delay(10);
  };
  digitalWrite(DownPin, LOW);
  Serial.println("  done");
  Serial.println();
}


#endif
