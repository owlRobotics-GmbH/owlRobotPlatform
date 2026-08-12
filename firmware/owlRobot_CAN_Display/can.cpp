#include "can.h"


#include <Arduino.h>

namespace {
  constexpr bool kCanDriverDebug = false;
}

CAN can;


bool CAN::begin(){  
  const uint8_t csPin = 17;    // GPIO17 -> CS_CAN
  const uint8_t mosiPin = 15;  // GPIO15 -> CAN MOSI (SPI1 TX)
  const uint8_t misoPin = 12;  // GPIO12 -> CAN MISO (SPI1 RX)
  const uint8_t sckPin = 14;   // GPIO14 -> CAN SCLK (SPI1 SCK)

  can0 = new MCP2515(spi1, csPin, mosiPin, misoPin, sckPin, 10000000);
  //Initialize interface
  MCP2515::ERROR err = can0->reset();
  if (err != MCP2515::ERROR_OK) {
    if (kCanDriverDebug) {
      Serial.print("[CAN] reset error: ");
      Serial.println(err);
    }
  }
  if (err != MCP2515::ERROR_OK) {
    if (kCanDriverDebug) {
      Serial.print("[CAN] reset error: ");
      Serial.println(err);
      Serial.print("[CAN] CANSTAT=");
      Serial.println(can0->getStatus(), HEX);
    }
  }
  err = can0->setBitrate(CAN_1000KBPS, MCP_16MHZ);
  if (err != MCP2515::ERROR_OK) {
    if (kCanDriverDebug) {
      Serial.print("[CAN] setBitrate 16MHz error: ");
      Serial.println(err);
      Serial.print("[CAN] CANSTAT=");
      Serial.println(can0->getStatus(), HEX);
      Serial.println("[CAN] retrying with 8MHz oscillator");
    }
    err = can0->setBitrate(CAN_1000KBPS, MCP_8MHZ);
    if (err != MCP2515::ERROR_OK) {
      if (kCanDriverDebug) {
        Serial.print("[CAN] setBitrate 8MHz error: ");
        Serial.println(err);
        Serial.print("[CAN] CANSTAT=");
        Serial.println(can0->getStatus(), HEX);
      }
    } else {
      if (kCanDriverDebug) {
        Serial.println("[CAN] setBitrate OK with 8MHz");
      }
    }
  } else {
    if (kCanDriverDebug) {
      Serial.println("[CAN] setBitrate OK with 16MHz");
    }
  }
  //can0.setBitrate(CAN_1000KBPS, MCP_8MHZ);    
  err = can0->setNormalMode();
  if (err != MCP2515::ERROR_OK) {
    if (kCanDriverDebug) {
      Serial.print("[CAN] setNormalMode error: ");
      Serial.println(err);
      Serial.print("[CAN] CANSTAT=");
      Serial.println(can0->getStatus(), HEX);
    }
  } else {
    if (kCanDriverDebug) {
      Serial.println("[CAN] controller in normal mode");
    }
  }

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
  } else if (err != MCP2515::ERROR_NOMSG) {
    if (kCanDriverDebug) {
      Serial.print("[CAN] readMessage error: ");
      Serial.println(err);
      Serial.print("[CAN] CANSTAT=");
      Serial.println(can0->getStatus(), HEX);
      Serial.print("[CAN] EFLG=");
      Serial.println(can0->getErrorFlags(), HEX);
    }
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
