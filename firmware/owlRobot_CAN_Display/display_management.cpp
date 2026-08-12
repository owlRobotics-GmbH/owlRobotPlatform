#include "display_management.h"

#include <PicoOTA.h>
#include <hardware/watchdog.h>

#include "owlcan.h"

extern char __flash_binary_end;

namespace {
constexpr uint8_t kCommandInfo = 0;
constexpr uint8_t kCommandRequest = 1;
constexpr uint8_t kCommandSet = 2;
constexpr uint8_t kCommandSave = 3;
constexpr uint8_t kCommandAck = 4;
constexpr uint8_t kValueUploadFirmware = 16;
constexpr uint8_t kValueFirmwareCrc = 17;
constexpr uint8_t kValueFirmwareVersion = 18;
constexpr const char *kUploadFile = "display-firmware.bin";
PicoOTA picoOta;

uint32_t decodeU32(const uint8_t *data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24);
}

void encodeU32(uint32_t value, uint8_t data[4]) {
  data[0] = value;
  data[1] = value >> 8;
  data[2] = value >> 16;
  data[3] = value >> 24;
}
}

DisplayManagement displayManagement;

void DisplayManagement::begin() {
  const uint8_t *start = reinterpret_cast<const uint8_t *>(XIP_BASE);
  const uint8_t *end = reinterpret_cast<const uint8_t *>(&__flash_binary_end);
  for (const uint8_t *p = start; p <= end; ++p) firmwareCrc_ += *p;
}

void DisplayManagement::sendFrame(uint8_t destination, uint8_t command,
                                  uint8_t value, const uint8_t payload[4]) {
  canNodeType_t node{};
  node.sourceAndDest.sourceNodeID = DISPLAY_MANAGEMENT_NODE_ID;
  node.sourceAndDest.destNodeID = destination;
  can_frame_t tx{};
  tx.can_id = CAN_DISPLAY_MANAGEMENT_MSG_ID;
  tx.can_dlc = 8;
  tx.data[0] = node.byteVal[0];
  tx.data[1] = node.byteVal[1];
  tx.data[2] = command;
  tx.data[3] = value;
  memcpy(tx.data + 4, payload, 4);
  can.write(tx);
  can.processTxFifo();
}

bool DisplayManagement::writeByte(uint32_t offset, uint8_t value) {
  if (offset >= kMaxFirmwareSize) return false;
  if (offset == 0) {
    if (upload_) upload_.close();
    if (!LittleFS.begin() && (!LittleFS.format() || !LittleFS.begin())) return false;
    LittleFS.remove(kUploadFile);
    upload_ = LittleFS.open(kUploadFile, "w");
    uploadSize_ = 0;
  }
  if (!upload_ || offset > static_cast<uint32_t>(upload_.position())) return false;
  if (offset != static_cast<uint32_t>(upload_.position()) && !upload_.seek(offset)) return false;
  if (upload_.write(value) != 1) return false;
  uploadSize_ = offset + 1;
  return true;
}

bool DisplayManagement::finish(uint32_t expectedCrc) {
  if (!upload_) return false;
  upload_.close();
  File file = LittleFS.open(kUploadFile, "r");
  if (!file) return false;
  uint32_t actualCrc = 0;
  for (uint32_t i = 0; i < uploadSize_; ++i) {
    int value = file.read();
    if (value < 0) { file.close(); return false; }
    actualCrc += static_cast<uint8_t>(value);
  }
  file.close();
  if (actualCrc != expectedCrc) return false;
  picoOta.begin();
  if (!picoOta.addFile(kUploadFile) || !picoOta.commit()) return false;
  LittleFS.end();
  delay(200);
  watchdog_reboot(0, 0, 0);
  while (true) tight_loop_contents();
}

bool DisplayManagement::onCanReceived(const can_frame_t &frame) {
  if (frame.can_id != CAN_DISPLAY_MANAGEMENT_MSG_ID || frame.can_dlc != 8)
    return false;
  canNodeType_t node{};
  node.byteVal[0] = frame.data[0];
  node.byteVal[1] = frame.data[1];
  if (node.sourceAndDest.destNodeID != DISPLAY_MANAGEMENT_NODE_ID &&
      node.sourceAndDest.destNodeID != 63) return true;

  uint8_t response[4] = {0, 0, 0, 0};
  const uint8_t command = frame.data[2];
  const uint8_t value = frame.data[3];
  const uint8_t *payload = frame.data + 4;
  if (command == kCommandRequest) {
    if (value == kValueFirmwareVersion) encodeU32(OWL_DISPLAY_FIRMWARE_VERSION, response);
    else if (value == kValueFirmwareCrc) encodeU32(firmwareCrc_, response);
    else return true;
    sendFrame(node.sourceAndDest.sourceNodeID, kCommandInfo, value, response);
  } else if (command == kCommandSet && value == kValueUploadFirmware) {
    uint32_t offset = ((uint32_t)payload[0] << 16) |
                      ((uint32_t)payload[1] << 8) | payload[2];
    if (writeByte(offset, payload[3]))
      sendFrame(node.sourceAndDest.sourceNodeID, kCommandAck, value, payload);
  } else if (command == kCommandSave && value == kValueUploadFirmware) {
    if (finish(decodeU32(payload)))
      sendFrame(node.sourceAndDest.sourceNodeID, kCommandAck, value, payload);
  }
  return true;
}
