#ifndef CAN_H_
#define CAN_H_


// RP20240 CAN with FIFO

#include <Arduino.h>
#include "mcp2515.h"


#define CAN_FIFO_FRAMES 1000



typedef struct can_frame_t {
        unsigned long can_id;  /* 32 bit CAN_ID + EFF/RTR/ERR flags */
        byte    can_dlc; /* frame payload length in byte (0 .. 8) */
        byte    data[8];
        unsigned long idx;     // frame number
        unsigned long msecs;   // timestamp milliseconds
} can_frame_t;

class FIFO
{
  public:
    unsigned long frameCounter = 0;
    bool available();  
    bool read(can_frame_t &frame);  
    bool write(can_frame_t frame);
  private:
    int fifoStart = 0;
    int fifoEnd = 0;
    can_frame_t fifo[CAN_FIFO_FRAMES];
};

class CAN
{
  public:
    unsigned long frameCounterTx = 0; 
    unsigned long frameCounterRx = 0; 
    bool begin();
    bool available();  
    bool read(can_frame_t &frame);  
    bool write(can_frame_t frame);
    bool run();
  private:
    FIFO rxFifo;
    FIFO txFifo;    
    MCP2515 *can0 = 0;
};

extern CAN can;

#endif /* CAN_H_ */
