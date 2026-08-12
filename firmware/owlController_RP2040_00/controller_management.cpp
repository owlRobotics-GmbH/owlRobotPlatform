#include "controller_management.h"

#include <LittleFS.h>
#include <PicoOTA.h>
#include <Wire.h>
#include <hardware/watchdog.h>

#include "can.h"
#include "owlcan.h"

extern char __flash_binary_end;

namespace {
const uint8_t kCommandRequest = 1;
const uint8_t kCommandSet = 2;
const uint8_t kCommandSave = 3;
const uint8_t kCommandAck = 4;
const uint8_t kValueUploadFirmware = 16;
const uint8_t kValueFirmwareCrc = 17;
const uint8_t kValueFirmwareVersion = 18;
const uint8_t kValueI2cCount = 43;
const uint8_t kValueI2cDevice = 44;
const uint8_t kValueI2cScan = 45;
const uint8_t kTcaAddress = 0x70;
const uint8_t kDirectBus = 0xff;
const char *kUpdateFile = "/update.bin";

File updateFile;
PicoOTA picoOta;

uint32_t decodeU32(const uint8_t data[4]) {
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

ControllerManagement controllerManagement;

void ControllerManagement::begin() {
  LittleFS.begin();
  const uint8_t *start = reinterpret_cast<const uint8_t *>(XIP_BASE);
  const uint8_t *end = reinterpret_cast<const uint8_t *>(&__flash_binary_end);
  firmwareCrc_ = 0;
  for (const uint8_t *p = start; p <= end; ++p) firmwareCrc_ += *p;
  scanI2c();
}

void ControllerManagement::run() {
  if (scanRequested_) {
    scanRequested_ = false;
    scanI2c();
  }
}

bool ControllerManagement::probe(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

void ControllerManagement::addDevice(uint8_t channel, uint8_t address) {
  uint8_t count = deviceCount_;
  if (count >= kMaxI2cDevices) return;
  devices_[count].channel = channel;
  devices_[count].address = address;
  deviceCount_ = count + 1;
}

void ControllerManagement::scanI2c() {
  deviceCount_ = 0;
  const bool hasMux = probe(kTcaAddress);
  if (hasMux) {
    addDevice(kDirectBus, kTcaAddress);
    for (uint8_t channel = 0; channel < 8; channel++) {
      Wire.beginTransmission(kTcaAddress);
      Wire.write(1U << channel);
      if (Wire.endTransmission() != 0) continue;
      delay(2);
      for (uint8_t address = 1; address < 127; address++) {
        if (address != kTcaAddress && probe(address)) addDevice(channel, address);
      }
    }
    Wire.beginTransmission(kTcaAddress);
    Wire.write(0);
    Wire.endTransmission();
  } else {
    for (uint8_t address = 1; address < 127; address++) {
      if (probe(address)) addDevice(kDirectBus, address);
    }
  }
}

void ControllerManagement::sendFrame(uint8_t destination, uint8_t command,
                                     uint8_t value, const uint8_t payload[4]) {
  canNodeType_t node;
  node.sourceAndDest.sourceNodeID = CONTROLLER_MANAGEMENT_NODE_ID;
  node.sourceAndDest.destNodeID = destination;
  node.sourceAndDest.reserved = 0;
  can_frame_t frame;
  frame.can_id = CAN_CONTROLLER_MANAGEMENT_MSG_ID;
  frame.can_dlc = 8;
  frame.data[0] = node.byteVal[0];
  frame.data[1] = node.byteVal[1];
  frame.data[2] = command;
  frame.data[3] = value;
  memcpy(frame.data + 4, payload, 4);
  can.write(frame);
}

void ControllerManagement::sendAck(uint8_t destination, uint8_t value,
                                   const uint8_t payload[4]) {
  sendFrame(destination, kCommandAck, value, payload);
}

bool ControllerManagement::onCanReceived(unsigned long id, int len,
                                         unsigned char data[8]) {
  if (id != CAN_CONTROLLER_MANAGEMENT_MSG_ID || len != 8) return false;
  canNodeType_t node;
  node.byteVal[0] = data[0];
  node.byteVal[1] = data[1];
  if (node.sourceAndDest.destNodeID != CONTROLLER_MANAGEMENT_NODE_ID &&
      node.sourceAndDest.destNodeID != 63)
    return true;

  const uint8_t command = data[2];
  const uint8_t value = data[3];
  const uint8_t *payload = data + 4;
  uint8_t response[4] = {0, 0, 0, 0};

  if (command == kCommandRequest) {
    if (value == kValueFirmwareVersion) {
      encodeU32(OWL_CONTROLLER_FIRMWARE_VERSION, response);
    } else if (value == kValueFirmwareCrc) {
      encodeU32(firmwareCrc_, response);
    } else if (value == kValueI2cCount) {
      response[0] = deviceCount_;
    } else if (value == kValueI2cDevice) {
      const uint8_t index = payload[0];
      response[0] = index;
      if (index < deviceCount_) {
        response[1] = devices_[index].channel;
        response[2] = devices_[index].address;
        response[3] = 1;
      }
    } else {
      return true;
    }
    sendFrame(node.sourceAndDest.sourceNodeID, 0, value, response);
    return true;
  }

  if (command == kCommandSet && value == kValueI2cScan) {
    scanRequested_ = true;
    sendAck(node.sourceAndDest.sourceNodeID, value, payload);
    return true;
  }

  if (command == kCommandSet && value == kValueUploadFirmware) {
    const uint32_t offset = ((uint32_t)payload[0] << 16) |
                            ((uint32_t)payload[1] << 8) | payload[2];
    if (offset == 0) {
      if (updateFile) updateFile.close();
      LittleFS.remove(kUpdateFile);
      updateFile = LittleFS.open(kUpdateFile, "w");
    }
    if (updateFile && updateFile.position() == offset && updateFile.write(payload[3]) == 1)
      sendAck(node.sourceAndDest.sourceNodeID, value, payload);
    return true;
  }

  if (command == kCommandSave && value == kValueUploadFirmware) {
    if (updateFile) updateFile.close();
    const uint32_t expectedCrc = decodeU32(payload);
    File file = LittleFS.open(kUpdateFile, "r");
    if (!file || !file.size()) return true;
    uint32_t actualCrc = 0;
    while (file.available()) actualCrc += static_cast<uint8_t>(file.read());
    file.close();
    if (actualCrc != expectedCrc) return true;
    picoOta.begin();
    if (!picoOta.addFile(kUpdateFile)) return true;
    if (!picoOta.commit()) return true;
    sendAck(node.sourceAndDest.sourceNodeID, value, payload);
    delay(100);
    watchdog_reboot(0, 0, 0);
  }
  return true;
}
