#ifndef CAN_MSG_HANDLER_H
#define CAN_MSG_HANDLER_H

#include <Arduino.h>
#include <math.h>
#include "can.h"
#include "owlcan.h"

class DashboardDisplay;

struct CanValueSelector {
    uint16_t frameId;
    canCmdType_t command;
    uint8_t valueCode;
};

struct CanDisplayConfig {
    static constexpr uint8_t kRainStateValueCode = 6; // mirrors owlctl::can_val_rain_state
    CanValueSelector satSummary      { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_sat_summary };
    CanValueSelector rtkAge          { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_rtk_age };
    CanValueSelector wifiSignal      { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_wifi_signal };
    CanValueSelector ipAddress       { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_ip_address };
    CanValueSelector voltage         { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_battery_voltage };
    CanValueSelector current         { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_battery_current };
    CanValueSelector mapProgress     { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_map_progress };
    CanValueSelector stateCode       { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_state_code };
    CanValueSelector rainState       { CAN_CONTROL_MSG_ID, can_cmd_info,     kRainStateValueCode };
    CanValueSelector statusMessage   { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_status_message };
    CanValueSelector ultrasonicAlert { OWL_DISPLAY_MSG_ID, can_cmd_info,     owldisplay::can_val_ultrasonic_alert };

    uint8_t localNodeId = 63;   // accept any destination (set to OWL_DISPLAY_NODE_ID to filter strictly)
    uint8_t rtkDecimals = 1;
};

class CanMsgHandler {
public:
    CanMsgHandler(DashboardDisplay &display, CanDisplayConfig cfg = {});

    bool begin();
    void process();
    void enableDebug(bool enable);

    unsigned long lastVoltageUpdate() const { return lastVoltageUpdateMs_; }
    float lastVoltage() const { return lastVoltage_; }

private:
    void handleFrame(const can_frame_t &frame);
    void handleRequest(const can_frame_t &frame, const canNodeType_t &node);
    void sendRp2040SerialChunk(uint8_t destNodeId, uint8_t valueCode);
    bool matches(const CanValueSelector &selector, const can_frame_t &frame) const;

    void handleSatSummary(const can_frame_t &frame);
    void handleRtkAge(const can_frame_t &frame);
    void handleWifiSignal(const can_frame_t &frame);
    void handleIpAddress(const can_frame_t &frame);
    void handleVoltage(const can_frame_t &frame);
    void handleCurrent(const can_frame_t &frame);
    void handleMapProgress(const can_frame_t &frame);
    void handleStateCode(const can_frame_t &frame);
    void handleRainState(const can_frame_t &frame);
    void handleStatusMessage(const can_frame_t &frame);
    void handleUltrasonicAlert(const can_frame_t &frame);
    void updateStateDisplay();

    void printFrame(const can_frame_t &frame) const;
    bool acceptsDestination(uint8_t destId) const;

    DashboardDisplay &display_;
    CanDisplayConfig config_;
    bool debug_ = false;
    unsigned long lastVoltageUpdateMs_ = 0;
    float lastVoltage_ = NAN;

    String lastMessage_;
    bool rainDetected_ = false;
    uint8_t lastStateCode_ = owldisplay::state_unknown;
};

#endif // CAN_MSG_HANDLER_H
