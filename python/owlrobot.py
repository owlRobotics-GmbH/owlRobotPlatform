#!/usr/bin/env python

# owlRobotics robot platform - Raspberry PI Python interface (using CAN bus hardware)

import ctypes
import struct
import os
import time
import can   # pip install --break-system-packages  python-can



OWL_DRIVE_MSG_ID      = 300
MY_NODE_ID            = 60 

LEFT_MOTOR_NODE_ID    = 1
RIGHT_MOTOR_NODE_ID   = 2
TOOL_MOTOR_NODE_ID    = 3


# what action to do...
can_cmd_info       = 0  # broadcast something
can_cmd_request    = 1  # request something
can_cmd_set        = 2  # set something
can_cmd_save       = 3  # save something        


# which variable to use for the action...
can_val_target          = 0   # target
can_val_voltage         = 1   # voltage
can_val_current         = 2   # current
can_val_velocity        = 3   # velocity
can_val_angle           = 4   # angle
can_val_motion_ctl_mode = 5   # motion control mode
can_val_cfg_mem         = 6   # config memory
can_val_motor_enable    = 7   # motor enable state
can_val_pAngleP         = 8   # angle P controller
can_val_velocityLimit   = 9   # max. velocity of the position control (rad/s)
can_val_pidVelocityP    = 10  # velocity P   
can_val_pidVelocityI    = 11  # velocity I   
can_val_pidVelocityD    = 12  # velocity D
can_val_pidVelocityRamp = 13  # velocity PID output ramp  (max. output change/s)
can_val_lpfVelocityTf   = 14  #  velocity low-pass filtering time constant (sec)
can_val_error           = 15  #  error status
can_val_upload_firmware = 16  #  upload file (to upload new firmware)
can_val_firmware_crc    = 17  #  firmware flash memory CRC (to verify firmware integrity)        
can_val_firmware_ver    = 18  # firmware version
can_val_broadcast_rx_enable  = 19  # broadcast receive enable state       
can_val_fifo_target     = 20   # add target (to drive within one clock duration) to FIFO 
can_val_endswitch_allow_pos_neg_dtargets = 21, # pos/neg delta targets allowed at end-switch?
can_val_reboot          = 22   # reboot MCU
can_val_endswitch       = 23   # end-switch status
can_val_fifo_clock      = 24   # FIFO clock signal (process FIFO)
can_val_control_error   = 25   # control error (setpoint-actual)
can_val_fifo_target_ack_result_val = 26, # which variable to send in an 'can_val_fifo_target' acknowledge     
can_val_detected_supply_voltage = 27,  # detected supply voltage
can_val_angle_add       = 28   # add angle 
can_val_pwm_speed       = 29   #pwm-speed (-1.0...1.0  =  classic motor controller compatiblity)
can_val_odo_ticks       = 30   # odometry ticks (encoder ticks   =  classic motor controller compatiblity)


# motor driver error values
err_ok           = 0  # everything OK
err_no_comm      = 1  # no CAN communication
err_no_settings  = 2  # no settings
err_undervoltage = 3  # undervoltage triggered
err_overvoltage  = 4  # overvoltage triggered
err_overcurrent  = 5  # overcurrent triggered
err_overtemp     = 6  # over-temperature triggered    


class CStruct(ctypes.LittleEndianStructure):
    _fields_ = [
        ("sourceId", ctypes.c_uint32, 6),  # 6 bit wide
        ("destId", ctypes.c_uint32, 6), # 6 bits wide
        ("reserved", ctypes.c_uint32, 4)   # 4 bits wide
    ]




class Robot():
    def __init__(self, aname = "owlRobot"):
        self.name = aname
        self.bus = can.interface.Bus(channel='can0', bustype='socketcan', receive_own_messages=True)
        #notifier = can.Notifier(self.bus, [can.Printer()])

    def __del__(self):
        print('closing CAN...')
        self.bus.shutdown()


    def sendCanData(self, destNodeId, cmd, val, data):        
        cs = CStruct()
        cs.sourceId = MY_NODE_ID
        cs.destId = destNodeId
        node = struct.unpack_from('<BB', cs)
        #print(node[0])
        
        frame = bytes(node) + bytes([cmd]) + bytes([val]) + data

        #print('sendCanData=', frame, 'sourceNodeId=', bin(MY_NODE_ID), 'destNodeId=', bin(destNodeId), 'cmd=', bin(cmd), 
        #    'val=', bin(val), 'data=', data)
        
        #for data in frame:
        #    print(bin(data))

        #frame = [0x7c,0xe0,0x02,0x1d,0x91,0x90,0x90,0xbd]

        msg = can.Message(arbitration_id=OWL_DRIVE_MSG_ID, data=frame, is_extended_id=False)
        #print(msg)
        self.bus.send(msg, timeout=0.2)


    def motorSpeed(self, leftMotorSpeed, rightMotorSpeed, toolMotorSpeed):        
        self.sendCanData(LEFT_MOTOR_NODE_ID, can_cmd_set, can_val_velocity, struct.pack('<f', leftMotorSpeed))
        self.sendCanData(RIGHT_MOTOR_NODE_ID, can_cmd_set, can_val_velocity, struct.pack('<f', rightMotorSpeed))
        self.sendCanData(TOOL_MOTOR_NODE_ID, can_cmd_set, can_val_velocity, struct.pack('<f', toolMotorSpeed))

        #self.sendCanData(LEFT_MOTOR_NODE_ID, can_cmd_set, can_val_pwm_speed, struct.pack('<f', leftMotorSpeed))
        #self.sendCanData(RIGHT_MOTOR_NODE_ID, can_cmd_set, can_val_pwm_speed, struct.pack('<f', rightMotorSpeed))
        #self.sendCanData(TOOL_MOTOR_NODE_ID, can_cmd_set, can_val_pwm_speed, struct.pack('<f', toolMotorSpeed))




if __name__ == "__main__":

    robot = Robot()

    while True:
        time.sleep(1.0)
        robot.motorSpeed(100.0, 100.0, 100.0)

    



