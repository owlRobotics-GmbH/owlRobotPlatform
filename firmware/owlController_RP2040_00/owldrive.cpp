/* 
    owlDrive SDK (CAN)
*/

#include "owldrive.h"


owlDrive::owlDrive(owlDriveCAN *aCanDriver, int aDriverNodeId, int aOperatorNodeId, int aCanMsgId){
  init();
  canDriver = aCanDriver;
  canMsgId = aCanMsgId;
  driverNodeId = aDriverNodeId;
  operatorNodeId = aOperatorNodeId;
}


void owlDrive::init(){
  debug = false;
  error = owldrv::err_no_comm; 
  voltage = 0;
  current = 0;
  target = 0;
  velocity = 0;
  angle = 0;
  rxPacketTime = 0;
  rxPacketCounter = 0;
}

void owlDrive::run(){ 
}    

void owlDrive::onCanReceived(int id, int len, unsigned char canData[8]){    
    if (id != canMsgId) return;
    canNodeType_t node;
    node.byteVal[0] = canData[0];
    node.byteVal[1] = canData[1];    
    if (node.sourceAndDest.sourceNodeID != driverNodeId) return; // message is not from expected owlDrive node  

    rxPacketCounter++;
    rxPacketTime = millis();

    if (debug){
      printCanFrame(canData);
    }    

    int cmd = canData[2];     
    owldrv::canValueType_t val = ((owldrv::canValueType_t)canData[3]);            
    canDataType_t data;
    data.byteVal[0] = canData[4];
    data.byteVal[1] = canData[5];
    data.byteVal[2] = canData[6];
    data.byteVal[3] = canData[7];    
        
    if (cmd == can_cmd_info){
        // info value (volt, velocity, position, ...)
        switch (val){
          case owldrv::can_val_error:
            error = (owldrv::errorType_t)data.byteVal[0]; 
            break;
          case owldrv::can_val_target:
            target = data.floatVal;
            break;
          case owldrv::can_val_velocity:
            velocity = data.floatVal;
            break;
          case owldrv::can_val_voltage:
            voltage = data.floatVal;
            break;
          case owldrv::can_val_current:
            current = data.floatVal;
            break;
          case owldrv::can_val_angle:
            angle = data.floatVal;
            break;
          case owldrv::can_val_motion_ctl_mode:
            motionControlMode = data.byteVal[0];
            break;
        }
    } 
}


void owlDrive::sendCanData(int destNodeId, canCmdType_t cmd, owldrv::canValueType_t val, canDataType_t data){        
    unsigned char canData[8];
    int id = canMsgId;    
    int len;
    if (cmd == can_cmd_request){
      len = 4;
    } else {
      len = 8;
    }
    canNodeType_t node;
    node.sourceAndDest.sourceNodeID = operatorNodeId;
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


void owlDrive::printCanFrame(unsigned char canData[8]){
    canNodeType_t node;
    node.byteVal[0] = canData[0];
    node.byteVal[1] = canData[1];
    int cmd = canData[2];     
    owldrv::canValueType_t val = ((owldrv::canValueType_t)canData[3]);            
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


void owlDrive::sendTarget(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_target, data);
}

void owlDrive::sendVoltage(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_voltage, data);
}

void owlDrive::sendVelocity(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_velocity, data);
}

void owlDrive::sendAngle(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_angle, data);
}

void owlDrive::sendEnable(bool enable){
  canDataType_t data;
  data.byteVal[0] = (char)enable;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_motor_enable, data);
}


void owlDrive::sendPAngleP(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_pAngleP, data);
}

void owlDrive::sendVelocityLimit(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_velocityLimit, data);
}

void owlDrive::sendPidVelocityP(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_pidVelocityP, data);
}

void owlDrive::sendPidVelocityI(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_pidVelocityI, data);
}

void owlDrive::sendPidVelocityD(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_pidVelocityD, data);
}

void owlDrive::sendPidVelocityRamp(float value){
  canDataType_t data;
  data.floatVal = value;
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_pidVelocityRamp, data);
}

void owlDrive::sendLpfVelocityTf(float value){
  canDataType_t data;
  data.floatVal = value;  
  sendCanData(driverNodeId, can_cmd_set, owldrv::can_val_lpfVelocityTf, data);
}



void owlDrive::requestError(){
  canDataType_t data;  
  data.floatVal = 0;  
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_error, data);
}

void owlDrive::requestMotionControlMode(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_motion_ctl_mode, data);
}

void owlDrive::requestVelocity(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_velocity, data);
}

void owlDrive::requestTarget(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_target, data);
}

void owlDrive::requestVoltage(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_voltage, data);
}

void owlDrive::requestCurrent(){
  canDataType_t data;  
  data.floatVal = 0;  
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_current, data);
}

void owlDrive::requestAngle(){
  canDataType_t data;
  data.floatVal = 0;    
  sendCanData(driverNodeId, can_cmd_request, owldrv::can_val_angle, data);
}
