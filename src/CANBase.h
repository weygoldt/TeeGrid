#ifndef CANBase_h
#define CANBase_h

#include <Arduino.h>
#include <FlexCAN_T4.h>
#include <RTClock.h>

#define CAN_ID_CLEAR_DEVICES 0x01
#define CAN_ID_FIND_DEVICES  0x02
#define CAN_ID_REPORT_DEVICE 0x03
#define CAN_ID_GOT_DEVICES   0x04

#define CAN_ID_SET_DATE      0x0A
#define CAN_ID_SET_TIME      0x0B
#define CAN_ID_SET_GRID      0x0C
#define CAN_ID_SET_RATE      0x0D
#define CAN_ID_SET_GAIN      0x0E


extern RTClock rtclock;


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

  // wait for maximum timeout ms and poll for a message with specific ID
  bool read(CAN_MSG &msg, unsigned int id, unsigned int timeout=1000);
  
  int detectDevices();
  int assignDevice();

  void setupControllerMBs();
  void setupRecorderMBs();

  void sendTime();
  void receiveTime();

  void sendGrid(const char gs[8]);
  void receiveGrid(char gs[8]);

  void sendSamplingRate(int rate);
  int receiveSamplingRate();

  void sendGain(float gain);
  float receiveGain();

  void sendFileTime(float filetime);
  float receiveFileTime();

  uint64_t events() { return Can.events(); };

  
protected:

  CANCLASS<BUS, RX_SIZE_16, TX_SIZE_8> Can;
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
bool CANBase<CANCLASS, BUS, CAN_MSG>::read(CAN_MSG &msg, unsigned int id,
					   unsigned int timeout) {
  elapsedMillis timepassed = 0;
  msg.id = 0;
  memset(msg.buf, 0, 8);
  while ((!Can.read(msg) || msg.id != id) && timepassed < timeout) {
    delay(1);
  };
  return (msg.id == id);
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
int CANBase<CANCLASS, BUS, CAN_MSG>::assignDevice() {
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
    return 0;
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
  return DeviceID;
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::setupControllerMBs() {
  // Can.setMaxMB(10); only for CAN2.0
  int i;
  for (i=0; i<5; i++)
    Can.setMB((FLEXCAN_MAILBOX)i, RX, STD);
  /*
  for (; i<10; i++)
    Can.setMB((FLEXCAN_MAILBOX)i, TX, STD);
  */
  Can.setMBFilter(REJECT_ALL);
  Can.enableMBInterrupts();
  //Can.onReceive(MB0, canSniff);
  //Can.setMBFilter(MB0, 0x001);
  Can.mailboxStatus();
}


template <typename CAN_MSG>
void setTime(const CAN_MSG &msg) {
  time_t t = *(time_t *)(&msg.buf[0]);
  rtclock.set(t);
  rtclock.report();
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::setupRecorderMBs() {
  int i;
  for (i=0; i<5; i++)
    Can.setMB((FLEXCAN_MAILBOX)i, RX, STD);
  /*
  for (; i<10; i++)
    Can.setMB((FLEXCAN_MAILBOX)i, TX, STD);
  */
  Can.setMBFilter(REJECT_ALL);
  Can.enableMBInterrupts();
  Can.onReceive(MB0, setTime);
  Can.setMBFilter(MB0, CAN_ID_SET_TIME);
  Can.mailboxStatus();
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::sendTime() {
  CAN_MSG msg;
  time_t t = now();
  char ds[10];
  rtclock.date(ds, t, true);
  char ts[10];
  rtclock.time(ts, t, true);
  msg.id = CAN_ID_SET_DATE;
  memcpy((void *)msg.buf, (void *)ds, 8);
  Can.write(msg);
  delay(5);
  msg.id = CAN_ID_SET_TIME;
  memcpy((void *)msg.buf, (void *)ts, 6);
  Can.write(msg);
  Serial.printf("sent date %s and time %s\n", ds, ts);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::receiveTime() {
  CAN_MSG msg;
  if (!read(msg, CAN_ID_SET_DATE))
    return;
  char datetime[20];
  memcpy((void *)&datetime[0], (void *)&msg.buf[0], 4);
  datetime[4] = '-';
  memcpy((void *)&datetime[5], (void *)&msg.buf[4], 2);
  datetime[7] = '-';
  memcpy((void *)&datetime[8], (void *)&msg.buf[6], 2);
  if (!read(msg, CAN_ID_SET_TIME))
    return;
  datetime[10] = 'T';
  memcpy((void *)&datetime[11], (void *)&msg.buf[0], 2);
  datetime[13] = ':';
  memcpy((void *)&datetime[14], (void *)&msg.buf[2], 2);
  datetime[16] = ':';
  memcpy((void *)&datetime[17], (void *)&msg.buf[4], 2);
  datetime[19] = '\0';
  Serial.printf("received time %s\n", datetime);
  rtclock.set(datetime);
  rtclock.report();
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::sendGrid(const char gs[8]) {
  CAN_MSG msg;
  msg.id = CAN_ID_SET_GRID;
  strncpy((char *)msg.buf, gs, 7);
  Can.write(msg);
  delay(5);
  Serial.printf("sent grid name %s\n", gs);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::receiveGrid(char gs[8]) {
  CAN_MSG msg;
  Serial.println("wait for grid name message");
  read(msg, CAN_ID_SET_GRID);
  memcpy((void *)&gs[0], (void *)&msg.buf[0], 8);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::sendSamplingRate(int rate) {
  CAN_MSG msg;
  msg.id = CAN_ID_SET_RATE;
  *(int *)(&msg.buf[0]) = rate;
  Can.write(msg);
  delay(5);
  Serial.printf("sent sampling rate %dHz\n", rate);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
int CANBase<CANCLASS, BUS, CAN_MSG>::receiveSamplingRate() {
  CAN_MSG msg;
  Serial.println("wait for sampling rate message");
  read(msg, CAN_ID_SET_RATE);
  int rate = *(int *)(&msg.buf[0]);
  return rate;
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::sendGain(float gain) {
  CAN_MSG msg;
  msg.id = CAN_ID_SET_GAIN;
  *(float *)(&msg.buf[0]) = gain;
  Can.write(msg);
  delay(5);
  Serial.printf("sent gain %.1fdB\n", gain);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
float CANBase<CANCLASS, BUS, CAN_MSG>::receiveGain() {
  CAN_MSG msg;
  Serial.println("wait for gain message");
  read(msg, CAN_ID_SET_GAIN);
  float gain = *(float *)(&msg.buf[0]);
  return gain;
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
void CANBase<CANCLASS, BUS, CAN_MSG>::sendFileTime(float filetime) {
  CAN_MSG msg;
  msg.id = CAN_ID_SET_GAIN;
  *(float *)(&msg.buf[0]) = filetime;
  Can.write(msg);
  delay(5);
  Serial.printf("sent file time %.0fs\n", filetime);
}


template <template<CAN_DEV_TABLE, FLEXCAN_RXQUEUE_TABLE,
		   FLEXCAN_TXQUEUE_TABLE> typename CANCLASS,
	  CAN_DEV_TABLE BUS,
	  typename CAN_MSG>
float CANBase<CANCLASS, BUS, CAN_MSG>::receiveFileTime() {
  CAN_MSG msg;
  Serial.println("wait for file time message");
  read(msg, CAN_ID_SET_GAIN);
  float filetime = *(float *)(&msg.buf[0]);
  return filetime;
}


#endif
