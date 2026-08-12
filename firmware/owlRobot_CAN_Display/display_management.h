#ifndef DISPLAY_MANAGEMENT_H_
#define DISPLAY_MANAGEMENT_H_

#include <Arduino.h>
#include <LittleFS.h>

#include "can.h"

#define CAN_DISPLAY_MANAGEMENT_MSG_ID 700
#define DISPLAY_MANAGEMENT_NODE_ID 58
#define OWL_DISPLAY_FIRMWARE_VERSION 2

class DisplayManagement {
public:
  void begin();
  bool onCanReceived(const can_frame_t &frame);

private:
  static constexpr uint32_t kMaxFirmwareSize = 1024U * 1024U;
  File upload_;
  uint32_t uploadSize_ = 0;
  uint32_t firmwareCrc_ = 0;

  bool writeByte(uint32_t offset, uint8_t value);
  bool finish(uint32_t expectedCrc);
  void sendFrame(uint8_t destination, uint8_t command, uint8_t value,
                 const uint8_t payload[4]);
};

extern DisplayManagement displayManagement;

#endif
