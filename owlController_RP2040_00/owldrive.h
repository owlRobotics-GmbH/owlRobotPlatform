/*
  owlDrive motor SDK (CAN)
*/

#ifndef OWL_DRIVE_H
#define OWL_DRIVE_H

#include <Arduino.h>
#include "owlcan.h"

// -----CAN frame data types----------------

namespace owldrv {

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
  enum errorType_t: uint8_t {
      err_ok           = 0, // everything OK
      err_no_comm      = 1, // no CAN communication
      err_no_settings  = 2, // no settings
      err_undervoltage = 3, // undervoltage triggered
      err_overvoltage  = 4, // overvoltage triggered
      err_overcurrent  = 5, // overcurrent triggered
      err_overtemp     = 6, // over-temperature triggered    
  };

} // namespace

// -----------------------------------------



// use this for each owlDrive motor driver
class owlDrive
{
  public:
    bool debug;          // output debug info?
    int canMsgId;        // CAN message ID used in all owlDrives
    int driverNodeId;    // node ID used in your owlDrive     
    int operatorNodeId;  // node ID used for operator
    owldrv::errorType_t error;     // error status (see error constants above)
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
    void sendCanData(int destNodeId, canCmdType_t cmd, owldrv::canValueType_t val, canDataType_t data);
    void printCanFrame(unsigned char canData[8]);

};



#endif
