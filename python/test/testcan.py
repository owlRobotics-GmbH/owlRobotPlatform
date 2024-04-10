#!/usr/bin/env python

# test CAN interface

# NOTE: start can interface with:
#     sudo ip link set can0 up type can bitrate 1000000



# async I/O :  https://docs.python.org/3/library/asyncio.html


import can
import time


# https://copperhilltech.com/blog/pican2-pican3-and-picanm-driver-installation-for-raspberry-pi/
# https://copperhilltech.com/blog/installing-pythoncan-on-the-raspberry-pi/


bus = can.interface.Bus(channel='can0', bustype='socketcan')



notifier = can.Notifier(bus, [can.Printer()])
    

while True:
    time.sleep(1.0)

    #msg = can.Message(arbitration_id=0x7de,data=[0, 25, 0, 1, 3, 1, 4, 1])
    
    # all motors
    msg = can.Message(arbitration_id=0x12c,data=[0xbc, 0x00, 0x02, 0x03, 0x00, 0x00, 0xc8, 0x42], is_extended_id=False)
    bus.send(msg)

    msg = can.Message(arbitration_id=0x12c,data=[0xfc, 0x00, 0x02, 0x03, 0x00, 0x00, 0xc8, 0x42], is_extended_id=False)
    bus.send(msg)

    msg = can.Message(arbitration_id=0x12c,data=[0x7c, 0x00, 0x02, 0x03, 0x00, 0x00, 0xc8, 0x42], is_extended_id=False)
    bus.send(msg)


    
