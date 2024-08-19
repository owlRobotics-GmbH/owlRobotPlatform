#include "can.h"


#include <Arduino.h>

CAN can;
volatile bool can_busy = false;


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
	return (fifoRxStart != fifoRxEnd); 
}

bool CAN::read(can_frame_t &frame){	
	if (fifoRxStart == fifoRxEnd) return false;
	
	frame.idx = fifoRx[fifoRxStart].idx;
	frame.msecs = fifoRx[fifoRxStart].msecs;
	
	frame.can_id = fifoRx[fifoRxStart].can_id;
	frame.can_dlc = fifoRx[fifoRxStart].can_dlc;
	for (int i=0; i < sizeof(frame.data); i++) frame.data[i] = fifoRx[fifoRxStart].data[i];
	if (fifoRxStart == CAN_FIFO_FRAMES_RX-1) fifoRxStart = 0; 
	  else fifoRxStart++;

	return true;
}


bool CAN::run(){
	struct can_frame frame;		

  if (can0 == 0) return false;  
  
  while (can_busy);
  can_busy = true;
  MCP2515::ERROR err = can0->readMessage(&frame);
  can_busy = false;
  
  if (err != MCP2515::ERROR_OK) {
    return false;
  }

	int nextFifoRxEnd = fifoRxEnd;
	if (nextFifoRxEnd == CAN_FIFO_FRAMES_RX-1) nextFifoRxEnd = 0; 
	  else nextFifoRxEnd++;
	
	if (nextFifoRxEnd != fifoRxStart){
		// no fifoRx overflow 
		fifoRx[fifoRxEnd].idx = frameCounterRx;
		fifoRx[fifoRxEnd].msecs = millis();

		fifoRx[fifoRxEnd].can_id = frame.can_id;
		fifoRx[fifoRxEnd].can_dlc = frame.can_dlc;
		for (int i=0; i < sizeof(frame.data); i++) fifoRx[fifoRxEnd].data[i] = frame.data[i]; 
		fifoRxEnd = nextFifoRxEnd;
	} else {
		// fifoRx overflow
		fifoRxEnd = fifoRxStart;
	}

	frameCounterRx++;
	
	return true;
}


bool CAN::write(can_frame_t frame){
	if (can0 == 0) return false;
  struct can_frame fr; 
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

  while (can_busy);
  can_busy = true;
  can0->sendMessage(&fr);
  can_busy = false;  
  frameCounterTx++;
	
  return true;
}


