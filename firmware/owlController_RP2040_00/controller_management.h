#ifndef CONTROLLER_MANAGEMENT_H_
#define CONTROLLER_MANAGEMENT_H_

#include <Arduino.h>

#define CAN_CONTROLLER_MANAGEMENT_MSG_ID 500
#define CONTROLLER_MANAGEMENT_NODE_ID 60
#define OWL_CONTROLLER_FIRMWARE_VERSION 2

class ControllerManagement {
public:
  void begin();
  void run();
  bool onCanReceived(unsigned long id, int len, unsigned char data[8]);

private:
  struct I2cDevice { uint8_t channel; uint8_t address; };
  static const uint8_t kMaxI2cDevices = 48;
  I2cDevice devices_[kMaxI2cDevices];
  volatile uint8_t deviceCount_ = 0;
  volatile bool scanRequested_ = false;
  uint32_t firmwareCrc_ = 0;

  void scanI2c();
  bool probe(uint8_t address);
  void addDevice(uint8_t channel, uint8_t address);
  void sendFrame(uint8_t destination, uint8_t command, uint8_t value,
                 const uint8_t payload[4]);
  void sendAck(uint8_t destination, uint8_t value, const uint8_t payload[4]);
};

extern ControllerManagement controllerManagement;

#endif
