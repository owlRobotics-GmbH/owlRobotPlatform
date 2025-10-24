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
      can_val_error             = 1,  // error status
      can_val_battery_voltage   = 2,  // battery voltage
      can_val_bumper_state      = 3,  // bumper state
      can_val_stop_button_state = 4,  // STOP button state
      can_val_buzzer_state      = 5,  // buzzer state
      can_val_rain_state        = 6,  // rain state
      can_val_charger_voltage   = 7,  // charger voltage
      can_val_lift_state        = 8,  // lift state
      can_val_slow_down_state   = 9,  // slow-down state
      can_val_ip_address        = 10, // show Raspberry Pi IP address on display 
      can_val_relais_state      = 11, // relais state
      can_val_power_off_state   = 12, // power-off GPIO state
      can_val_power_off_command = 13, // schedule power-off command
  };

  // motor driver error values
  enum errorType_t: uint8_t {
      err_ok           = 0, // everything OK
      err_no_comm      = 1, // no CAN communication
      err_undervoltage = 2, // undervoltage triggered
      err_overvoltage  = 3, // overvoltage triggered
      err_overtemp     = 4, // over-temperature triggered    
  };

  enum powerOffState_t: uint8_t {
      power_off_inactive = 0,
      power_off_active = 1,
      power_off_shutdown_pending = 2,
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
    float chargerVoltage;  // volts
    byte bumperState;      // bumper state
    bool stopButtonState;  // STOP button state
    bool buzzerState;      // buzzer state
    bool liftState;        // lift state
    bool rainState;        // rain state
    bool slowDownState;    // slow-down state
    unsigned long rxPacketCounter;    // number of received CAN packets for this node
    unsigned long rxPacketTime;       // last time we received a CAN packet for this node
    String raspberryPiIP; // Raspberry Pi IP address (if available)
    unsigned long rcvIpAddressTimeout;

    owlDriveCAN *canDriver; // driver to send/receive data

    owlControl(owlDriveCAN *aCanDriver, int driverNodeId, int aOperatorNodeId = MY_NODE_ID, int aCanMsgId = CAN_CONTROL_MSG_ID);
    
    owlControl();

    void setPowerOffPinState(bool state);
    bool powerOffPinLatched() const { return powerOffState >= owlctl::power_off_active; }
    
    // call this for any CAN packet received via your CAN interface
    void onCanReceived(int id, int len, unsigned char canData[8]);    

    void requestError();
    void requestBatteryVoltage();
    void requestChargerVoltage();    
    void requestBumperState();
    void requestLiftState();    
    void requestStopButtonState();
    void requestBuzzerState();
    void requestRainState();
    void requestSlowDownState();

    void sendError(int destNodeId, owlctl::errorType_t error);
    void sendBatteryVoltage(int destNodeId, float value);
    void sendChargerVoltage(int destNodeId, float value);   
    void sendBumperState(int destNodeId, byte value);
    void sendLiftState(int destNodeId, bool value);    
    void sendStopButtonState(int destNodeId, bool value);
    void sendBuzzerState(int destNodeId, bool value);
    void sendRainState(int destNodeId, bool value);
    void sendSlowDownState(int destNodeId, bool value);
    void sendPowerOffState(int destNodeId);
    void sendPowerOffCommandAck(int destNodeId, bool accepted, uint8_t delaySeconds);

    void run();
    void setStopButtonState(bool state);
    void setBumperState(byte state);
    void setRainState(bool state);
    void setLiftState(bool state);
    void setSlowDownState(bool state);

  protected:
    unsigned long buzzerStateTimeout;
    unsigned long stopButtonStateTimeout;
    unsigned long bumperStateTimeout;
    bool powerOffPinState;
    unsigned long powerOffPinChangedTime;
    owlctl::powerOffState_t powerOffState;
    unsigned long shutdownScheduledTime;
    bool shutdownCommandAccepted;
    uint8_t shutdownDelaySeconds;
    
             
    void init();    
    void sendCanData(int destNodeId, canCmdType_t cmd, owlctl::canValueType_t val, canDataType_t data);
    void printCanFrame(unsigned char canData[8]);

};


#endif
