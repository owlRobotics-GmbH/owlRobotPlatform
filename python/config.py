#!/usr/bin/env python

# owlRobotics robot platform config 
# robot database and robot object factory

import diffdrive
import mecanum
import uuid
import platform
import dabble


# robot IDs (WiFi Mac address):
ROBOT_ID_DIFF_DRIVE    = 0x1c1bb5d748c1
ROBOT_ID_MECANUM       = 0x1c1bb5d748c2 
ROBOT_ID_ALEX          = 0x1c1bb5d748c6

# owlRobot types
ROBOT_TYPE_DIFF_DRIVE = 0
ROBOT_TYPE_MECANUM    = 1



# robot body coordinate system:
# top view (z up):
#
#        y
#        |
#    mobile base ----> x    (forward driving axis)
#
# angles:  counterclock-wise: positive (+) 
#


# define robot type, wheel-to-center-x distance (m), and wheel-to-center-y distance (m), wheel diameter (m), max speed (m/s) 
        
ROBOTS = {
    ROBOT_ID_DIFF_DRIVE: { 
        "name": "owlRobot (DiffDrive)",
        "type": ROBOT_TYPE_DIFF_DRIVE, 
        "bluetoothUSB": False,
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "maxSpeedX": 0.4,
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.2,
    },

    ROBOT_ID_ALEX: { 
        "name": "owlRobot (Alex)",
        "type": ROBOT_TYPE_DIFF_DRIVE,
        "bluetoothUSB": True, 
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "maxSpeedX": 0.4,        
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.2,
    },

    ROBOT_ID_MECANUM: { 
        "name": "owlRobot (Mecanum)",
        "type": ROBOT_TYPE_MECANUM,
        "bluetoothUSB": False, 
        "wheelToBodyCenterX": 0.25,
        "wheelToBodyCenterY": 0.25,
        "wheelDiameter": 0.15,
        "maxSpeedX": 0.4,      
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.2,   
    },

}



# create robot object based on machine id found in config
def createRobot():
    machine = platform.machine()  # x86_64  etc.
    print('machine:', machine)
    
    mid = uuid.getnode()
    print ('machine UUID:', hex(mid))

    cfg = None
    for aid in ROBOTS:
        if aid == mid:
            cfg = ROBOTS[aid]

    if cfg is None:
        print('error finding robot in database!')
        return None

    print('found config:')
    print(cfg)

    robot = None
    if cfg['type'] == ROBOT_TYPE_DIFF_DRIVE:
        robot = diffdrive.DifferentialDriveRobot(
            cfg['name'],
            cfg['wheelToBodyCenterY'], 
            cfg['wheelDiameter'])   # wheel-center-y,  wheel-dia
        robot.bluetoothUSB = cfg['bluetoothUSB']

    elif cfg['type'] == ROBOT_TYPE_MECANUM:
        robot = mecanum.MecanumRobot(
            cfg['name'],
            cfg['wheelToBodyCenterX'],
            cfg['wheelToBodyCenterY'],
            cfg['wheelDiameter'])         # wheel-center-x,  wheel-center-y,  wheel-dia
        robot.bluetoothUSB = cfg['bluetoothUSB'] 
    else:
        print('invalid robot type')
    
    return robot



# create dabble object 
def createDabble(aRobot):
    useUSB = aRobot.bluetoothUSB
    print('bluetoothUSB', useUSB)
    if useUSB:            
        app = dabble.Dabble('usb:0')
    else:
        app = dabble.Dabble('hci-socket:0')
    return app


