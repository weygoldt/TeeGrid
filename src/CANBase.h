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
  Can.begin();
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
  int r = Can.write(msg);
  Serial.printf("  write clear message, r=%d\n", r);
  delay(1000);

  // assign device IDs:
  int id;
  for (id=1; ; id++) {
    Serial.printf("  check for ID=%d\n", id);
    msg.id = CAN_ID_FIND_DEVICES;
    int r = Can.write(msg);
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
    Serial.printf("    device reported id %d\n", id);
    delay(100);
  }
  msg.id = CAN_ID_GOT_DEVICES;
  r = Can.write(msg);
  Serial.printf("  write got devices message, r=%d\n", r);
  digitalWrite(DownPin, LOW);
  Serial.printf("  got %d devices\n", id-1);
  while (1) {};
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
  Serial.println("  wait for clear devices command");
  timeout = 0;
  msg.id = 0;
  while (!Can.read(msg) && timeout < 100000) {
    delay(10);
  };
  Serial.printf("  got message 0x%02x\n", msg.id);
  if (msg.id != CAN_ID_CLEAR_DEVICES)
    return;
  DeviceID = 0;
  digitalWrite(DownPin, LOW);

  // assign device ID:
  for (int id=1; ; id++) {
    Serial.printf("  check for ID=%d\n", id);
    timeout = 0;
    msg.id = 0;
    Serial.printf("    wait for find devices\n");
    while ((!Can.read(msg) || msg.id == CAN_ID_REPORT_DEVICE) && timeout < 10000) {
      delay(10);
    };
    Serial.printf("    got message 0x%02x\n", msg.id);
    if (msg.id != CAN_ID_FIND_DEVICES)
      break;
    if (digitalRead(UpPin)) {
      DeviceID = id;
      Serial.printf("    assign ID %d\n", DeviceID);
      msg.id = CAN_ID_REPORT_DEVICE;
      int r = Can.write(msg);
      Serial.printf("    write found message, r=%d\n", r);
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
}


#endif
