#include "can.h"


#include <Arduino.h>

CAN can;


bool CAN::begin(){  
  can0 = new MCP2515(spi0, 17, 19, 16, 18, 10000000);
  //Initialize interface
  can0->reset();
  can0->setBitrate(CAN_1000KBPS, MCP_16MHZ);
  //can0.setBitrate(CAN_1000KBPS, MCP_8MHZ);    
  can0->setNormalMode();

	return true;
}

bool CAN::available(){
	return rxFifo.available(); 
}

bool CAN::read(can_frame_t &frame){	
	return rxFifo.read(frame);
}



bool CAN::write(can_frame_t frame){
  return (txFifo.write(frame));
}


void CAN::fillRxFifo(){
	can_frame_t frame;		
  struct can_frame fr; 

  if (can0 == 0) return;  

  MCP2515::ERROR err = can0->readMessage(&fr);
    
  if (err == MCP2515::ERROR_OK) {
    frame.can_id = fr.can_id;
    frame.can_dlc = fr.can_dlc;
    for (int i=0; i < 8; i++) frame.data[i] = fr.data[i]; 

    rxFifo.write(frame);
  } 
}


void CAN::processTxFifo(){
  can_frame_t frame;		
  struct can_frame fr; 

  if (can0 == 0) return;  

  if (txFifo.read(frame)){
    fr.can_id = frame.can_id;
    fr.can_dlc = frame.can_dlc;
    for (int i=0; i < 8; i++) fr.data[i] = frame.data[i]; 

    //  id=12C len=8 data=7C 80 2 3 0 0 C8 42
    //  SLCAN: t12C87C8002030000C842  
    /*
    Serial.print("id=");
    Serial.print(fr.can_id, HEX);
    Serial.print(" len=");
    Serial.print(fr.can_dlc);
    Serial.print(" data=");
    for (int i=0; i < fr.can_dlc; i++){
      Serial.print(fr.data[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    */

    can0->sendMessage(&fr);
  }
}

