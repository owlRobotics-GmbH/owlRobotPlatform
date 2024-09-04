/* 
    owlControl SDK (CAN)
*/

#include "owlcontrol.h"


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
  stopButtonState = false;
  buzzerState = false;
  rainState = false;
  buzzerStateTimeout = 0;
  rxPacketTime = 0;
  rxPacketCounter = 0;
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


void owlControl::onCanReceived(int id, int len, unsigned char canData[8]){    
    if (debug){
      Serial.print("onCanReceived id=");
      Serial.println(id);    
      printCanFrame(canData);
    }    

    if (id != canMsgId) return;
    canNodeType_t node;
    node.byteVal[0] = canData[0];
    node.byteVal[1] = canData[1];    
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
          case owlctl::can_val_charger_voltage:
            sendChargerVoltage(node.sourceAndDest.sourceNodeID, chargerVoltage);
            break;
        }
    }
    else if (cmd == can_cmd_set){
      switch (val){
        case owlctl::can_val_buzzer_state:
          buzzerState = (bool)data.byteVal[0];
          buzzerStateTimeout = millis() + 2000;
          break;
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
    canData[0] = node.byteVal[0];
    canData[1] = node.byteVal[1];    
    canData[2] = cmd;    
    canData[3] = val;
    canData[4] = data.byteVal[0];
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
