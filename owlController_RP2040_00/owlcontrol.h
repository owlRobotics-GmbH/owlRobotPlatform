/*
  owlControl SDK (CAN)
*/

#ifndef OWL_CONTROL_H
#define OWL_CONTROL_H

#include <Arduino.h>

// The lower the numerical ID, the higher the message priority. 
#define CAN_EMERGENCY_BUTTON   260    // high priority ID
#define CAN_LIFT_MSG_ID        261
#define CAN_BUMPER_MSG_ID      262
#define CAN_DOCK_MSG_ID        263

#define CAN_IMU_MSG_ID         280
#define CAN_JOYSTICK_MSG_ID    281
#define CAN_BATTERY_MSG_ID     282    
#define CAN_RAIN_MSG_ID        283
#define CAN_BUZZER_MSG_ID      284
#define CAN_LED_MSG_ID         285

#define CAN_MOTOR_MSG_ID       300



#define MY_NODE_ID 60 


// -----CAN frame data types----------------

// source/destination node ID 
typedef union canNodeType_t {   
    uint8_t byteVal[2];
    struct __attribute__ ((__packed__)) {   
        uint8_t sourceNodeID : 6;   // 6 bits for source node ID (valid node IDs: 1-62)
        uint8_t destNodeID   : 6;   // 6 bits for destination node ID (valid node IDs: 1-62, value 63 means all nodes)    
        uint8_t reserved     : 4;   // 4 bits reserved
    } sourceAndDest;     
} canNodeType_t;


// what action to do...
enum canCmdType_t: uint8_t {
    can_cmd_info       = 0,  // broadcast something
    can_cmd_request    = 1,  // request something
    can_cmd_set        = 2,  // set something
    can_cmd_save       = 3,  // save something        
};

// which variable to use for the action...
enum canValueType_t: uint8_t {
    can_val_target          = 0, // target
    can_val_voltage         = 1, // voltage
    can_val_current         = 2, // current
    can_val_velocity        = 3, // velocity
    can_val_angle           = 4, // angle
    can_val_motion_ctl_mode = 5, // motion control mode
    can_val_cfg_mem         = 6, // config memory
    can_val_motor_enable    = 7, // motor enable state
    can_val_pAngleP         = 8, // angle P controller
    can_val_velocityLimit   = 9, // max. velocity of the position control (rad/s)
    can_val_pidVelocityP    = 10, // velocity P   
    can_val_pidVelocityI    = 11, // velocity I   
    can_val_pidVelocityD    = 12, // velocity D
    can_val_pidVelocityRamp = 13, // velocity PID output ramp  (max. output change/s)
    can_val_lpfVelocityTf   = 14, // velocity low-pass filtering time constant (sec)
    can_val_error           = 15, // error status
};

// motor driver error values
enum errType_t: uint8_t {
    err_ok           = 0, // everything OK
    err_no_comm      = 1, // no CAN communication
    err_no_settings  = 2, // no settings
    err_undervoltage = 3, // undervoltage triggered
    err_overvoltage  = 4, // overvoltage triggered
    err_overcurrent  = 5, // overcurrent triggered
    err_overtemp     = 6, // over-temperature triggered    
};

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
    virtual void sendPacket(unsigned long id, int len, unsigned char data[8]) = 0;
};


// use this for each owlDrive motor driver
class owlDrive
{
  public:
    bool debug;          // output debug info?
    int canMsgId;        // CAN message ID used in all owlDrives
    int driverNodeId;    // node ID used in your owlDrive     
    int operatorNodeId;  // node ID used for operator
    errType_t error;     // error status (see error constants above)
    int motionControlMode;  // motion control mode (torque, velocity, angle, velocity_openloop, angle_openloop)
    float target;        // voltage/velocity/angle
    float velocity;      // motor rad/s
    float voltage;       // volts
    float current;       // amps
    float angle;         // motor rad
    unsigned long rxPacketCounter;    // number of received CAN packets for this node
    unsigned long rxPacketTime;       // last time we received a CAN packet for this node

    owlDriveCAN *canDriver; // driver to send/receive data

    owlDrive(owlDriveCAN *aCanDriver, int driverNodeId, int aOperatorNodeId = MY_NODE_ID, int aCanMsgId = CAN_MOTOR_MSG_ID);

    // call this for any CAN packet received via your CAN interface
    void onCanReceived(int id, int len, unsigned char canData[8]);    

    void sendTarget(float value);
    void sendVoltage(float value);
    void sendVelocity(float value);
    void sendAngle(float value);
    void sendEnable(bool enable);
    void sendPAngleP(float value);
    void sendVelocityLimit(float value);
    void sendPidVelocityP(float value);
    void sendPidVelocityI(float value);
    void sendPidVelocityD(float value);
    void sendPidVelocityRamp(float value);
    void sendLpfVelocityTf(float value);

    void requestError();  
    void requestMotionControlMode();
    void requestVoltage();
    void requestCurrent();    
    void requestTarget();
    void requestVelocity();
    void requestAngle();          

  protected:
    void init();    
    void sendCanData(int destNodeId, canCmdType_t cmd, canValueType_t val, canDataType_t data);
    void printCanFrame(unsigned char canData[8]);

};



#endif
