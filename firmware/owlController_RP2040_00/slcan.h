/*
  CAN-over-serial protocol (SLCAN)

  Run this on your connected Linux machine to start and test the CAN-USB-bridge:
   1. sudo slcand -o -s8 -t hw -S 3000000 /dev/ttyACM0      (replace with USB device path)
   2. sudo ip link set up slcan0
   3. candump slcan0


  SLCAN protocol:
   example packet:  id=12C len=8 data=7C 80 2 3 0 0 C8 42
   SLCAN:           t12C87C8002030000C842  

   example packet:  id=7C0 len=2 data=01 02 
   SLCAN:           t7C020102  

*/

#ifndef SLCAN_H
#define SLCAN_H

#include <Arduino.h>
#include "owlcan.h"
#include "fifo.h"

#define SERIAL_FIFO_FRAMES 1000

class SLCAN
{
  public:
    void begin(owlDriveCAN *aCanDriver);
    void run();
    void fillRxFifo();
    void onCanReceived(int id, int len, unsigned char canData[8]);    
  protected:
    owlDriveCAN *canDriver; // driver to send/receive data
    int b2ahex(char *p, uint8_t s, uint8_t n, void *v);
    int a2bhex_sub(char a);
    int a2bhex(char *p, uint8_t s, uint8_t n, void *v);
    //void xfer_can2tty();
    void slcan_ack();
    void slcan_nack();
    void send_canmsg(char *buf);
    void pars_slcancmd(char *buf);
    void xfer_tty2can();    
    FiFo<char, SERIAL_FIFO_FRAMES> rxFifo;
    int g_can_speed = 0; // default: 1000k
    int g_ts_en = 0;
};


#endif
