/*
  owlDrive/owlControl SDK (CAN)
  common data types
*/

#ifndef OWL_CAN_H
#define OWL_CAN_H

#include <Arduino.h>

// The lower the numerical ID, the higher the message priority. 
#define CAN_CONTROL_MSG_ID     200    // high priority ID

// The lower the numerical ID, the higher the message priority. 
#define CAN_MOTOR_MSG_ID       300

// The lower the numerical ID, the higher the message priority.
#define CAN_RELAIS_MSG_ID      128    // relais control ID

#define MY_NODE_ID 60


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
    struct __attribute__ ((__packed__)) {   
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
