#include "can.h"


#include <Arduino.h>

CAN can;


bool FIFO::available(){
	return (fifoStart != fifoEnd); 
}

bool FIFO::read(can_frame_t &frame){	
	if (fifoStart == fifoEnd) return false;
	
	frame.idx = fifo[fifoStart].idx;
	frame.msecs = fifo[fifoStart].msecs;
	
	frame.can_id = fifo[fifoStart].can_id;
	frame.can_dlc = fifo[fifoStart].can_dlc;
	for (int i=0; i < sizeof(frame.data); i++) frame.data[i] = fifo[fifoStart].data[i];
	if (fifoStart == CAN_FIFO_FRAMES-1) fifoStart = 0; 
	  else fifoStart++;

	return true;
}


bool FIFO::write(can_frame_t frame){
  int nextFifoEnd = fifoEnd;
	if (nextFifoEnd == CAN_FIFO_FRAMES-1) nextFifoEnd = 0; 
	  else nextFifoEnd++;
	
	if (nextFifoEnd != fifoStart){
		// no fifoRx overflow 
		fifo[fifoEnd].idx = frameCounter;
		fifo[fifoEnd].msecs = millis();

		fifo[fifoEnd].can_id = frame.can_id;
		fifo[fifoEnd].can_dlc = frame.can_dlc;
		for (int i=0; i < sizeof(frame.data); i++) fifo[fifoEnd].data[i] = frame.data[i]; 
		fifoEnd = nextFifoEnd;
    frameCounter++;
    return true;
	} else {
		// fifoRx overflow
		fifoEnd = fifoStart;
    return false;
	}  
}



// ------------------------------------------------


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


bool CAN::run(){
	can_frame_t frame;		
  struct can_frame fr; 

  if (can0 == 0) return false;  

  while (true){
    MCP2515::ERROR err = can0->readMessage(&fr);
      
    if (err == MCP2515::ERROR_OK) {
      frame.can_id = fr.can_id;
      frame.can_dlc = fr.can_dlc;
      for (int i=0; i < 8; i++) frame.data[i] = fr.data[i]; 

      rxFifo.write(frame);
    } else break;  
  }

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
	
	return true;
}

