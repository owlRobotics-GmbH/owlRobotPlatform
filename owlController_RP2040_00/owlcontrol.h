/*
  owlControl PCB SDK (CAN)
*/

#ifndef OWL_CONTROL_H
#define OWL_CONTROL_H

#include <Arduino.h>
#include "owlcan.h"



// -----CAN frame data types----------------

namespace owlctl {

  // which variable to use for the action...
  enum canValueType_t: uint8_t {
      can_val_error             = 1, // error status
      can_val_battery_voltage   = 2, // battery voltage
      can_val_bumper_state      = 3, // bumper state
      can_val_stop_button_state = 4, // STOP button state
      can_val_buzzer_state      = 5, // buzzer state
      can_val_rain_state        = 6, // rain state
  };

  // motor driver error values
  enum errorType_t: uint8_t {
      err_ok           = 0, // everything OK
      err_no_comm      = 1, // no CAN communication
      err_undervoltage = 2, // undervoltage triggered
      err_overvoltage  = 3, // overvoltage triggered
      err_overtemp     = 4, // over-temperature triggered    
  };

}  // namespace

// -----------------------------------------


// use this for the 'owlControl PCB' CAN node
class owlControl
{
  public:
    bool debug;          // output debug info?
    int canMsgId;        // CAN message ID used for owlControl PCB
    int driverNodeId;    // node ID used in your owlControl PCB     
    int operatorNodeId;  // node ID used for operator
    owlctl::errorType_t error;     // error status (see error constants above)
    float batteryVoltage;  // volts
    byte bumperState;      // bumper state
    bool stopButtonState;  // STOP button state
    bool buzzerState;      // buzzer state
    bool rainState;        // rain state
    unsigned long rxPacketCounter;    // number of received CAN packets for this node
    unsigned long rxPacketTime;       // last time we received a CAN packet for this node

    owlDriveCAN *canDriver; // driver to send/receive data

    owlControl(owlDriveCAN *aCanDriver, int driverNodeId, int aOperatorNodeId = MY_NODE_ID, int aCanMsgId = CAN_CONTROL_MSG_ID);

    // call this for any CAN packet received via your CAN interface
    void onCanReceived(int id, int len, unsigned char canData[8]);    

    void requestError();
    void requestBatteryVoltage();
    void requestBumperState();
    void requestStopButtonState();
    void requestBuzzerState();
    void requestRainState();

    void sendError(int destNodeId, owlctl::errorType_t error);
    void sendBatteryVoltage(int destNodeId, float value);
    void sendBumperState(int destNodeId, byte value);
    void sendStopButtonState(int destNodeId, bool value);
    void sendBuzzerState(int destNodeId, bool value);
    void sendRainState(int destNodeId, bool value);

    void run();
    void setStopButtonState(bool state);
    void setBumperState(byte state);
    void setRainState(bool state);

  protected:
    unsigned long buzzerStateTimeout;
    unsigned long stopButtonStateTimeout;
    unsigned long bumperStateTimeout;         
    void init();    
    void sendCanData(int destNodeId, canCmdType_t cmd, owlctl::canValueType_t val, canDataType_t data);
    void printCanFrame(unsigned char canData[8]);

};


#endif
