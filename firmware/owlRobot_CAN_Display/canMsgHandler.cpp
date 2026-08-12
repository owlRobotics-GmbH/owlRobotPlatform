#include "canMsgHandler.h"

#include <Arduino.h>
#include <math.h>
#include <pico/unique_id.h>
#include "display.h"
#include "display_management.h"

namespace {
    inline float bytesToFloat(const uint8_t *bytes) {
        canDataType_t data;
        data.byteVal[0] = bytes[0];
        data.byteVal[1] = bytes[1];
        data.byteVal[2] = bytes[2];
        data.byteVal[3] = bytes[3];
        return data.floatVal;
    }

    inline void fillRp2040SerialChunk(canDataType_t &data, uint8_t chunkIndex) {
        pico_unique_board_id_t uid;
        pico_get_unique_board_id(&uid);
        const uint8_t offset = chunkIndex ? 4 : 0;
        for (uint8_t i = 0; i < 4; ++i) {
            data.byteVal[i] = uid.id[offset + i];
        }
    }
}

CanMsgHandler::CanMsgHandler(DashboardDisplay &display, CanDisplayConfig cfg)
    : display_(display), config_(cfg) {}

bool CanMsgHandler::begin() {
    return can.begin();
}

void CanMsgHandler::process() {
    can.processTxFifo();
    can.fillRxFifo();

    can_frame_t frame{};
    while (can.read(frame)) {
        handleFrame(frame);
    }
}

void CanMsgHandler::enableDebug(bool enable) {
    debug_ = enable;
}

void CanMsgHandler::handleFrame(const can_frame_t &frame) {
    if (displayManagement.onCanReceived(frame)) return;
    if (frame.can_dlc < 4) {
        if (debug_) {
            Serial.println("[CAN] Frame too short (<4 bytes)");
            printFrame(frame);
        }
        return;
    }

    canNodeType_t node{};
    node.byteVal[0] = frame.data[0];
    node.byteVal[1] = frame.data[1];

    if (!acceptsDestination(node.sourceAndDest.destNodeID)) {
        if (debug_) {
            Serial.print("[CAN] Dest mismatch: ");
            Serial.println(node.sourceAndDest.destNodeID);
            printFrame(frame);
        }
        return;
    }

    if (debug_) {
        Serial.print("[CAN] RX id=");
        Serial.print(frame.can_id, HEX);
        Serial.print(" cmd=");
        Serial.print(frame.data[2], HEX);
        Serial.print(" val=");
        Serial.print(frame.data[3], HEX);
        Serial.print(" dest=");
        Serial.println(node.sourceAndDest.destNodeID);
    }

    if (frame.data[2] == can_cmd_request) {
        handleRequest(frame, node);
        return;
    }

    if (matches(config_.voltage, frame)) {
        handleVoltage(frame);
    } else if (matches(config_.current, frame)) {
        handleCurrent(frame);
    } else if (matches(config_.satSummary, frame)) {
        handleSatSummary(frame);
    } else if (matches(config_.rtkAge, frame)) {
        handleRtkAge(frame);
    } else if (matches(config_.wifiSignal, frame)) {
        handleWifiSignal(frame);
    } else if (matches(config_.ipAddress, frame)) {
        handleIpAddress(frame);
    } else if (matches(config_.mapProgress, frame)) {
        handleMapProgress(frame);
  } else if (matches(config_.stateCode, frame)) {
      handleStateCode(frame);
  } else if (matches(config_.rainState, frame)) {
      handleRainState(frame);
  } else if (matches(config_.statusMessage, frame)) {
      handleStatusMessage(frame);
  } else if (matches(config_.ultrasonicAlert, frame)) {
      handleUltrasonicAlert(frame);
  } else if (debug_) {
      Serial.print("[CAN] No display mapping for value ");
      Serial.println(frame.data[3], HEX);
      printFrame(frame);
  }
}

void CanMsgHandler::handleRequest(const can_frame_t &frame, const canNodeType_t &node) {
    if (frame.can_id != OWL_DISPLAY_MSG_ID) {
        return;
    }

    const uint8_t valueCode = frame.data[3];
    if (valueCode != owldisplay::can_val_rp2040_serial_0 &&
        valueCode != owldisplay::can_val_rp2040_serial_1) {
        if (debug_) {
            Serial.print("[CAN] Unsupported display request value ");
            Serial.println(valueCode, HEX);
        }
        return;
    }

    sendRp2040SerialChunk(node.sourceAndDest.sourceNodeID, valueCode);
}

void CanMsgHandler::sendRp2040SerialChunk(uint8_t destNodeId, uint8_t valueCode) {
    canNodeType_t node{};
    node.sourceAndDest.sourceNodeID = OWL_DISPLAY_NODE_ID;
    node.sourceAndDest.destNodeID = destNodeId;

    canDataType_t payload{};
    fillRp2040SerialChunk(payload, valueCode == owldisplay::can_val_rp2040_serial_1 ? 1 : 0);

    can_frame_t tx{};
    tx.can_id = OWL_DISPLAY_MSG_ID;
    tx.can_dlc = 8;
    tx.data[0] = node.byteVal[0];
    tx.data[1] = node.byteVal[1];
    tx.data[2] = can_cmd_info;
    tx.data[3] = valueCode;
    tx.data[4] = payload.byteVal[0];
    tx.data[5] = payload.byteVal[1];
    tx.data[6] = payload.byteVal[2];
    tx.data[7] = payload.byteVal[3];

    can.write(tx);
    can.processTxFifo();

    if (debug_) {
        Serial.print("[CAN] TX RP2040 serial chunk value=");
        Serial.print(valueCode, HEX);
        Serial.print(" dest=");
        Serial.println(destNodeId);
    }
}

bool CanMsgHandler::acceptsDestination(uint8_t destId) const {
    (void)destId;
    return true;
}

bool CanMsgHandler::matches(const CanValueSelector &selector, const can_frame_t &frame) const {
    return (frame.can_id == selector.frameId &&
            frame.data[2] == selector.command &&
            frame.data[3] == selector.valueCode);
}

void CanMsgHandler::handleSatSummary(const can_frame_t &frame) {
    if (frame.can_dlc < 7) {
        if (debug_) Serial.println("[CAN] Sat frame too short");
        return;
    }

    const uint8_t statusCode = frame.data[4];
    const uint8_t used = frame.data[5];
    const uint8_t total = frame.data[6];

    DashboardDisplay::SatStatus satStatus;
    switch (statusCode) {
        case 0: satStatus = DashboardDisplay::Fix; break;
        case 1: satStatus = DashboardDisplay::Float; break;
        default: satStatus = DashboardDisplay::Invalid; break;
    }

    display_.setSat(satStatus, used, total);
}

void CanMsgHandler::handleRtkAge(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        if (debug_) Serial.println("[CAN] RTK frame too short");
        return;
    }

    const float age = bytesToFloat(&frame.data[4]);
    if (isnan(age) || isinf(age)) {
        if (debug_) Serial.println("[CAN] RTK age invalid");
        return;
    }

    display_.setRTK(age, config_.rtkDecimals);
}

void CanMsgHandler::handleWifiSignal(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        if (debug_) Serial.println("[CAN] WiFi frame too short");
        return;
    }

    const float rawDbm = bytesToFloat(&frame.data[4]);
    if (isnan(rawDbm) || isinf(rawDbm)) {
        if (debug_) Serial.println("[CAN] WiFi signal invalid");
        return;
    }

    const int16_t dbm = static_cast<int16_t>(roundf(rawDbm));
    display_.setWifiSignal(dbm);
}

void CanMsgHandler::handleIpAddress(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        if (debug_) Serial.println("[CAN] IP frame too short");
        return;
    }

    const uint8_t a = frame.data[4];
    const uint8_t b = frame.data[5];
    const uint8_t c = frame.data[6];
    const uint8_t d = frame.data[7];

    String ip = String(a) + "." + String(b) + "." + String(c) + "." + String(d);
    display_.setIP(ip);
}

void CanMsgHandler::handleVoltage(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        if (debug_) Serial.println("[CAN] Voltage frame too short");
        return;
    }

    const float voltage = bytesToFloat(&frame.data[4]);
    if (isnan(voltage) || isinf(voltage)) {
        if (debug_) Serial.println("[CAN] Voltage invalid");
        return;
    }

    lastVoltage_ = voltage;
    lastVoltageUpdateMs_ = millis();
    display_.setVolt(voltage, 1);

    if (debug_) {
        Serial.print("[CAN] Voltage: ");
        Serial.println(voltage, 3);
        printFrame(frame);
    }
}

void CanMsgHandler::handleCurrent(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        if (debug_) Serial.println("[CAN] Current frame too short");
        return;
    }

    const float current = bytesToFloat(&frame.data[4]);
    if (isnan(current) || isinf(current)) {
        if (debug_) Serial.println("[CAN] Current invalid");
        return;
    }

    display_.setAmp(current, 1);
}

void CanMsgHandler::handleMapProgress(const can_frame_t &frame) {
    if (frame.can_dlc < 7) {
        if (debug_) Serial.println("[CAN] Map frame too short");
        return;
    }

    const uint16_t count = static_cast<uint16_t>(frame.data[4]) |
                           (static_cast<uint16_t>(frame.data[5]) << 8);
    const uint8_t percent = frame.data[6];

    display_.setMap(count, percent);
}

void CanMsgHandler::handleStateCode(const can_frame_t &frame) {
    if (frame.can_dlc < 5) {
        if (debug_) Serial.println("[CAN] State frame too short");
        return;
    }

    const uint8_t code = frame.data[4];
    lastStateCode_ = code;
    updateStateDisplay();
}

void CanMsgHandler::handleRainState(const can_frame_t &frame) {
    if (frame.can_dlc < 5) {
        if (debug_) Serial.println("[CAN] Rain frame too short");
        return;
    }

    const bool active = frame.data[4] != 0;
    if (active == rainDetected_) {
        return;
    }

    rainDetected_ = active;
    if (lastStateCode_ == owldisplay::state_dock) {
        updateStateDisplay();
    }
}

void CanMsgHandler::handleStatusMessage(const can_frame_t &frame) {
    if (frame.can_dlc < 5) {
        if (debug_) Serial.println("[CAN] Message frame too short");
        return;
    }

    const uint8_t control = frame.data[4];
    const bool append = control & 0x01;
    const bool clear = control & 0x02;
    const uint8_t payloadLen = frame.can_dlc > 5 ? (frame.can_dlc - 5) : 0;

    if (clear) {
        lastMessage_ = "";
    }

    if (payloadLen > 0) {
        String chunk;
        chunk.reserve(payloadLen);
        for (uint8_t i = 0; i < payloadLen; ++i) {
            chunk += static_cast<char>(frame.data[5 + i]);
        }

        if (append) {
            lastMessage_ += chunk;
        } else {
            lastMessage_ = chunk;
        }
    }

    display_.showMessage(lastMessage_, 0);
}

void CanMsgHandler::handleUltrasonicAlert(const can_frame_t &frame) {
    if (frame.can_dlc < 8) {
        return;
    }

    const uint8_t sensorId = frame.data[4];
    const uint16_t distance = static_cast<uint16_t>(frame.data[5]) |
                               (static_cast<uint16_t>(frame.data[6]) << 8);
    const bool active = frame.data[7] != 0;

    display_.setUltrasonicAlert(sensorId, active, distance);
}

void CanMsgHandler::printFrame(const can_frame_t &frame) const {
    Serial.print("ID=");
    Serial.print(frame.can_id, HEX);
    Serial.print(" DLC=");
    Serial.print(frame.can_dlc);
    Serial.print(" DATA=");
    for (int i = 0; i < frame.can_dlc; ++i) {
        if (frame.data[i] < 0x10) Serial.print('0');
        Serial.print(frame.data[i], HEX);
        Serial.print(' ');
    }
    Serial.println();
}

void CanMsgHandler::updateStateDisplay() {
    String label;

    switch (lastStateCode_) {
        case owldisplay::state_mow:
            label = "MOW";
            break;
        case owldisplay::state_dock:
            label = rainDetected_ ? "DOCK\nRAIN" : "DOCK";
            break;
        case owldisplay::state_idle:
            label = "IDLE";
            break;
        case owldisplay::state_charge:
            label = "CHARGE";
            break;
        case owldisplay::state_error:
            label = "ERROR";
            break;
        default:
            label = "";
            break;
    }

    display_.setState(label);
}
