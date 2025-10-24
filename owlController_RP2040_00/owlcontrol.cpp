/* 
    owlControl SDK (CAN)
*/

#include "owlcontrol.h"
#include "config.h"
#include "Funkt.h"
#include "oledDisp.h"

extern oledDisp oled;
extern Funkt myF;

#ifndef POWER_OFF_PIN_ACTIVE_MS
#define POWER_OFF_PIN_ACTIVE_MS 3000
#endif

#ifndef POWER_OFF_CUTOFF_DELAY_SEC
#define POWER_OFF_CUTOFF_DELAY_SEC 30
#endif



owlControl::owlControl(owlDriveCAN *aCanDriver, int aDriverNodeId, int aOperatorNodeId, int aCanMsgId){
  init();
  canDriver = aCanDriver;
  canMsgId = aCanMsgId;
  driverNodeId = aDriverNodeId;
  operatorNodeId = aOperatorNodeId;
}


void owlControl::init(){
  debug = false;
  error = owlctl::err_no_comm; 
  batteryVoltage = 0;
  chargerVoltage = 0;
  bumperState = 0;
  liftState = false;
  stopButtonState = false;
  buzzerState = false;
  rainState = false;
  slowDownState = false;
  buzzerStateTimeout = 0;
  rxPacketTime = 0;
  rxPacketCounter = 0;
  raspberryPiIP = "";
  rcvIpAddressTimeout = 0;
  powerOffPinState = false;
  powerOffPinChangedTime = 0;
  powerOffState = owlctl::power_off_inactive;
  shutdownScheduledTime = 0;
  shutdownCommandAccepted = false;
  shutdownDelaySeconds = POWER_OFF_CUTOFF_DELAY_SEC;
}

void owlControl::run(){
   //robot.control.buzzerState = !robot.control.buzzerState;  
  if ((buzzerState) && (millis() > buzzerStateTimeout)) {
    buzzerState = false;
  }
  if ((bumperState != 0) && (millis() > bumperStateTimeout)) {
    bumperState = 0;
  }
  if ((stopButtonState) && (millis() > stopButtonStateTimeout)) {
    stopButtonState = false;
  }
  // Track how long the power-off input stays active so we only latch long presses.
  if (powerOffPinState){
    if ((powerOffState == owlctl::power_off_inactive) && (millis() - powerOffPinChangedTime >= POWER_OFF_PIN_ACTIVE_MS)){
      powerOffState = owlctl::power_off_active;
      Serial.println("owlControl: power-off pin latched active");
    }
  } else if (powerOffState != owlctl::power_off_shutdown_pending){
    if (powerOffState != owlctl::power_off_inactive){
      Serial.println("owlControl: power-off pin released");
      powerOffState = owlctl::power_off_inactive;
    }
  }
  // Once a shutdown command was accepted we wait and then drop the power hold line.
  if (shutdownCommandAccepted){
    if (millis() >= shutdownScheduledTime){
      shutdownCommandAccepted = false;
      Serial.println("owlControl: shutdown delay expired, cutting power-hold");
      myF.SW_Power_off();
    }
  }
}


void owlControl::setPowerOffPinState(bool state){
  if (state != powerOffPinState){
    powerOffPinState = state;
    powerOffPinChangedTime = millis();
    Serial.print("owlControl: power-off pin raw level=");
    Serial.println(state ? "HIGH" : "LOW");
    if (!state && (powerOffState != owlctl::power_off_shutdown_pending)){
      powerOffState = owlctl::power_off_inactive;
    }
  }
}

void owlControl::setStopButtonState(bool state){
  if (state) stopButtonStateTimeout = millis() + 500; 
  if (state) stopButtonState = state;
}

void owlControl::setBumperState(byte state){
  if (state != 0) bumperStateTimeout = millis() + 500; 
  if (state) bumperState = state;
}

void owlControl::setRainState(bool state){
  rainState = state;
}

void owlControl::setLiftState(bool state){
  liftState = state;
}

void owlControl::setSlowDownState(bool state){
  slowDownState = state;
}

void owlControl::onCanReceived(int id, int len, unsigned char canData[8]){  
  
    if (debug){
      Serial.print("onCanReceived id=");
      Serial.println(id);    
      printCanFrame(canData);
      Serial.print("MycanMsgId=");
      Serial.println(canMsgId);
      Serial.print("MydriverNodeId=");
      Serial.println(driverNodeId);
    }
    if (id == CAN_RELAIS_MSG_ID){
      Serial.println(); 
      Serial.println("onCanReceived:  ");
      Serial.print("  Raw Data:       ");
      for (int i = 0; i < len; i++) {
      Serial.print("[" + String(i) + "]=" + String(canData[i], DEC) + " ");
      }
      Serial.println();
      printCanFrame(canData);
      Serial.print("  CAN ID:         "); Serial.println(id, DEC);
      //Serial.print("  CAN Msg ID (expected): "); Serial.println(MY_NODE_ID, DEC);
      Serial.print("  Driver Node ID: "); Serial.println(driverNodeId, DEC);
      Serial.print("  Source Node ID: "); Serial.println(canData[0] & 0x3F, DEC); // 6 bits
      Serial.print("  Dest Node ID:   "); Serial.println(canData[1] & 0x3F, DEC);   // 6 bits    
      Serial.print("  Command:        "); Serial.println(canData[2], DEC);
      Serial.print("  Value Type:     "); Serial.println(canData[3], DEC);
      Serial.print("  Data Bytes:     ");
      for (int i = 4; i < 8; i++) {
      Serial.print("[" + String(i) + "]=" + String(canData[i], DEC) + " ");
      }
      Serial.println();
      Serial.println();
    }
    if (id != canMsgId) return;
    canNodeType_t node;
    node.byteVal[0] = canData[0]; // Source node ID (6 bits) and reserved (2 bits)
    node.byteVal[1] = canData[1]; // Destination node ID (6 bits) and reserved (2 bits)   
    //if (node.sourceAndDest.sourceNodeID != driverNodeId) return; // message is not from expected owlControl node  
    if (node.sourceAndDest.destNodeID != driverNodeId) return; // message is not for expected owlControl node  
    
    rxPacketCounter++;
    rxPacketTime = millis();

    int cmd = canData[2];     
    owlctl::canValueType_t val = ((owlctl::canValueType_t)canData[3]);            
    canDataType_t data;
    data.byteVal[0] = canData[4];
    data.byteVal[1] = canData[5];
    data.byteVal[2] = canData[6];
    data.byteVal[3] = canData[7];    

    

    if (cmd == can_cmd_info){
        // info value (volt, velocity, position, ...)
        switch (val){
          case owlctl::can_val_error:
            error = (owlctl::errorType_t)data.byteVal[0]; 
            break;
          case owlctl::can_val_lift_state:
            liftState = data.byteVal[0];
            break;
          case owlctl::can_val_bumper_state:
            bumperState = data.byteVal[0];
            break;
          case owlctl::can_val_battery_voltage:
            batteryVoltage = data.floatVal;
            break;          
          case owlctl::can_val_stop_button_state:
            stopButtonState = data.byteVal[0];
            break;
          case owlctl::can_val_buzzer_state:
            buzzerState = data.byteVal[0];
            break;
          case owlctl::can_val_rain_state:
            rainState = data.byteVal[0];
            break;
          case owlctl::can_val_slow_down_state:
            slowDownState = data.byteVal[0];
            break;
          case owlctl::can_val_charger_voltage:
            chargerVoltage = data.floatVal;
            break;
        }
    } 
    else if (cmd == can_cmd_request){
        switch (val){
          case owlctl::can_val_error:
            sendError(node.sourceAndDest.sourceNodeID, error); 
            break;
          case owlctl::can_val_lift_state:
            sendLiftState(node.sourceAndDest.sourceNodeID, liftState);
            break;          
          case owlctl::can_val_bumper_state:
            sendBumperState(node.sourceAndDest.sourceNodeID, bumperState);
            break;
          case owlctl::can_val_battery_voltage:
            sendBatteryVoltage(node.sourceAndDest.sourceNodeID, batteryVoltage);
            break;
          case owlctl::can_val_stop_button_state:
            sendStopButtonState(node.sourceAndDest.sourceNodeID, stopButtonState);
            break;
          case owlctl::can_val_buzzer_state:
            sendBuzzerState(node.sourceAndDest.sourceNodeID, buzzerState);
            break;
          case owlctl::can_val_rain_state:
            sendRainState(node.sourceAndDest.sourceNodeID, rainState);
            break;
          case owlctl::can_val_slow_down_state:
            sendSlowDownState(node.sourceAndDest.sourceNodeID, slowDownState);
            break;
          case owlctl::can_val_charger_voltage:
            sendChargerVoltage(node.sourceAndDest.sourceNodeID, chargerVoltage);
            break;
          case owlctl::can_val_power_off_state:
            sendPowerOffState(node.sourceAndDest.sourceNodeID);
            break;
        }
    }
    else if (cmd == can_cmd_set){
      switch (val){
        case owlctl::can_val_buzzer_state:
          buzzerState = (bool)data.byteVal[0];
          buzzerStateTimeout = millis() + 2000;
          break;
        case owlctl::can_val_ip_address:
          char ipStr[16];
          snprintf(ipStr, sizeof(ipStr), "%u.%u.%u.%u", data.byteVal[0], data.byteVal[1], data.byteVal[2], data.byteVal[3]);
          raspberryPiIP = String(ipStr);
          oled.setIP(raspberryPiIP); // update OLED display with new IP address
          break;
        case owlctl::can_val_power_off_command:
        {
          uint8_t requestedDelay = data.byteVal[0];
          uint8_t requesterId = node.sourceAndDest.sourceNodeID;
          if (!shutdownCommandAccepted){
            shutdownDelaySeconds = requestedDelay;
            shutdownScheduledTime = millis() + (unsigned long)shutdownDelaySeconds * 1000UL;
            shutdownCommandAccepted = true;
            Serial.print("owlControl: shutdown command accepted, delay=");
            Serial.print(shutdownDelaySeconds);
            Serial.print("s (pin latched: ");
            Serial.print(powerOffState >= owlctl::power_off_active ? "yes" : "no");
            Serial.println(")");
          } else {
            shutdownDelaySeconds = requestedDelay;
            shutdownScheduledTime = millis() + (unsigned long)shutdownDelaySeconds * 1000UL;
            Serial.print("owlControl: shutdown command updated, delay=");
            Serial.print(shutdownDelaySeconds);
            Serial.print("s (pin latched: ");
            Serial.print(powerOffState >= owlctl::power_off_active ? "yes" : "no");
            Serial.println(")");
          }
          powerOffState = owlctl::power_off_shutdown_pending;
          sendPowerOffCommandAck(requesterId, true, shutdownDelaySeconds);
          break;
        }
      }
    }
  
  
  
}


void owlControl::sendCanData(int destNodeId, canCmdType_t cmd, owlctl::canValueType_t val, canDataType_t data){        
    unsigned char canData[8];
    int id = canMsgId;    
    int len;
    if (cmd == can_cmd_request){
      len = 4;
    } else {
      len = 8;
    }
    canNodeType_t node;
    //node.sourceAndDest.sourceNodeID = operatorNodeId;
    node.sourceAndDest.sourceNodeID = driverNodeId;    
    node.sourceAndDest.destNodeID = destNodeId;    
    canData[0] = node.byteVal[0];                          // Source node ID (6 bits) and reserved (2 bits)
    canData[1] = node.byteVal[1];                          // Destination node ID (6 bits) and reserved (2 bits) 
    canData[2] = cmd;                                      // What to do. broadcast, request, set, save
    canData[3] = val;                                      // What value to send (battery voltage, error, etc.)
    canData[4] = data.byteVal[0];                          // Data to send (4 bytes, float, int, etc.)
    canData[5] = data.byteVal[1];
    canData[6] = data.byteVal[2];
    canData[7] = data.byteVal[3];
    if (debug) printCanFrame(canData);
    canDriver->sendPacket(id, len, canData);
}


void owlControl::printCanFrame(unsigned char canData[8]){
    canNodeType_t node;
    node.byteVal[0] = canData[0];
    node.byteVal[1] = canData[1];
    int cmd = canData[2];     
    owlctl::canValueType_t val = ((owlctl::canValueType_t)canData[3]);            
    canDataType_t data;
    data.byteVal[0] = canData[4];
    data.byteVal[1] = canData[5];
    data.byteVal[2] = canData[6];
    data.byteVal[3] = canData[7];    
    Serial.print("CAN cmd=");    
    Serial.print(cmd);
    Serial.print("(");
    switch (cmd){
        case can_cmd_info: Serial.print("can_cmd_info"); break;
        case can_cmd_set: Serial.print("can_cmd_set"); break;
        case can_cmd_save: Serial.print("can_cmd_save"); break;
        case can_cmd_request: Serial.print("can_cmd_request"); break;        
        default: Serial.print("can_cmd_unknown"); break;
    }   
    Serial.print("): srcId=");
    Serial.print(node.sourceAndDest.sourceNodeID);    
    Serial.print(" dstId=");
    Serial.print(node.sourceAndDest.destNodeID);
    Serial.print(" val=");
    Serial.print(val);
    Serial.print(" data=");        
    Serial.print(data.floatVal);        
    Serial.print(" (");            
    for (int i=0; i < sizeof(data); i++){
        Serial.print(data.byteVal[i], HEX);
        Serial.print(" ");
    }
    Serial.print(")");
    Serial.println();
}



void owlControl::sendBatteryVoltage(int destNodeId, float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_battery_voltage, data);
}

void owlControl::sendChargerVoltage(int destNodeId, float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_charger_voltage, data);
}

void owlControl::sendBumperState(int destNodeId, byte value){
  canDataType_t data;
  data.byteVal[0] = value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_bumper_state, data);
}

void owlControl::sendLiftState(int destNodeId, bool value){
  canDataType_t data;
  data.byteVal[0] = (byte)value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_lift_state, data);
}

void owlControl::sendError(int destNodeId, owlctl::errorType_t error){
  canDataType_t data;
  data.byteVal[0] = error;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_error, data);
}

void owlControl::sendStopButtonState(int destNodeId, bool value){
  canDataType_t data;
  data.byteVal[0] = (byte)value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_stop_button_state, data);
}

void owlControl::sendBuzzerState(int destNodeId, bool value){
  canDataType_t data;
  data.byteVal[0] = (byte)value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_buzzer_state, data);
}

void owlControl::sendRainState(int destNodeId, bool value){
  canDataType_t data;
  data.byteVal[0] = (byte)value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_rain_state, data);
}

void owlControl::sendSlowDownState(int destNodeId, bool value){
  canDataType_t data;
  data.byteVal[0] = (byte)value;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_slow_down_state, data);
}

void owlControl::sendPowerOffState(int destNodeId){
  canDataType_t data;
  data.byteVal[0] = (uint8_t)powerOffState;
  if (powerOffPinState){
    unsigned long activeMs = millis() - powerOffPinChangedTime;
    data.byteVal[1] = (uint8_t)min((unsigned long)255, activeMs / 1000UL);
  } else {
    data.byteVal[1] = 0;
  }
  data.byteVal[2] = shutdownDelaySeconds;
  data.byteVal[3] = 0;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_power_off_state, data);
  Serial.print("owlControl: send power-off state ");
  Serial.print(powerOffState);
  Serial.print(" to node ");
  Serial.println(destNodeId);
}

void owlControl::sendPowerOffCommandAck(int destNodeId, bool accepted, uint8_t delaySeconds){
  canDataType_t data;
  data.byteVal[0] = accepted ? 1 : 0;
  data.byteVal[1] = delaySeconds;
  data.byteVal[2] = 0;
  data.byteVal[3] = 0;
  sendCanData(destNodeId, can_cmd_info, owlctl::can_val_power_off_command, data);
  Serial.print("owlControl: send shutdown ACK ");
  Serial.print(accepted ? "OK" : "NOK");
  Serial.print(" to node ");
  Serial.println(destNodeId);
}




void owlControl::requestError(){
  canDataType_t data;  
  data.floatVal = 0;  
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_error, data);
}

void owlControl::requestBumperState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_bumper_state, data);
}

void owlControl::requestLiftState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_lift_state, data);
}

void owlControl::requestBatteryVoltage(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_battery_voltage, data);
}

void owlControl::requestChargerVoltage(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_charger_voltage, data);
}

void owlControl::requestStopButtonState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_stop_button_state, data);
}

void owlControl::requestBuzzerState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_buzzer_state, data);
}

void owlControl::requestRainState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_rain_state, data);
}

void owlControl::requestSlowDownState(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owlctl::can_val_slow_down_state, data);
}

owlControl::owlControl() {
  // Platzhalterwerte oder leere Initialisierung
  this->canDriver = nullptr;
  this->driverNodeId = 0;
}
