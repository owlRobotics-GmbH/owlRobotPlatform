/*
  owlDrive/owlControl SDK (CAN)
  common data types
*/

#ifndef OWL_CAN_H
#define OWL_CAN_H

#include <Arduino.h>

// The lower the numerical ID, the higher the message priority. 
#define CAN_CONTROL_MSG_ID     200    // owlControl

// The lower the numerical ID, the higher the message priority. 
#define CAN_MOTOR_MSG_ID       300    // owlDrive

// The lower the numerical ID, the higher the message priority.
#define CAN_RELAIS_MSG_ID      400    // owlRelais

#define OWL_DISPLAY_MSG_ID     500    // owlDisplay payload

// Legacy relay identifiers (unused in CAN display firmware, kept for compatibility)
#define RELAIS_NODE_1 1
#define RELAIS_NODE_2 2

#define OWL_DISPLAY_NODE_ID    1


// -----CAN frame data types----------------

// what action to do...
enum canCmdType_t: uint8_t {
    can_cmd_info       = 0,  // broadcast something
    can_cmd_request    = 1,  // request something
    can_cmd_set        = 2,  // set something
    can_cmd_save       = 3,  // save something        
};

// source/destination node ID 
typedef union canNodeType_t {
    uint8_t byteVal[2];
    struct __attribute__((__packed__)) {
        uint8_t sourceNodeID : 6;   // 6 bits for source node ID (valid node IDs: 1-62)
        uint8_t destNodeID   : 6;   // 6 bits for destination node ID (valid node IDs: 1-62, value 63 means all nodes)
        uint8_t reserved     : 4;   // 4 bits reserved
    } sourceAndDest;
} canNodeType_t;


// which data the variable has, CAN data can be different variants 
typedef union {
    uint8_t byteVal[4];  // either 4 bytes
    int32_t intValue;    // either integer (4 bytes)
    float floatVal;      // either float (4 bytes)
    struct __attribute__ ((__packed__)) ofs_val_t {   // either short (2 bytes) offset and 1 byte
        uint16_t ofsVal;
        uint8_t  byteVal;
    } ofsAndByte;
} __attribute__((packed)) canDataType_t;

namespace owldisplay {
    enum valueType_t : uint8_t {
        can_val_rp2040_serial_0 = 0x24,
        can_val_rp2040_serial_1 = 0x25,
        can_val_sat_summary      = 0x10,
        can_val_rtk_age          = 0x11,
        can_val_wifi_signal      = 0x12,
        can_val_ip_address       = 0x14,
        can_val_touch_event      = 0x22,
        can_val_battery_voltage  = 0x40,
        can_val_battery_current  = 0x41,
        can_val_map_progress     = 0x50,
        can_val_state_code       = 0x51,
        can_val_status_message   = 0x52,
        can_val_ultrasonic_alert = 0x60
    };

    enum stateCode_t : uint8_t {
        state_unknown = 0,
        state_mow     = 1,
        state_dock    = 2,
        state_idle    = 3,
        state_charge  = 4,
        state_error   = 5
    };
}

// -----------------------------------------


// subclass this - your subclassed CAN driver connects CAN packet interface with owlDrives
class owlDriveCAN
{
  public:
    // owlDrive wants to send a CAN packet - send it via your CAN interface 
    virtual void sendPacket(unsigned long id, int len, unsigned char data[8], bool enableUsbBridge = true) = 0;
    virtual void onPacketReceived(unsigned long id, int len, unsigned char data[8], bool enableUsbBridge = true) = 0;

};




#endif
