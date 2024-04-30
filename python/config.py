#!/usr/bin/env python

# owlRobotics robot platform config 
# robot database and robot object factory

import diffdrive
import mecanum
import uuid
import platform
import dabble
import owlrobot as owl


# robot IDs (WiFi MAC address):
ROBOT_ID_DIFF_DRIVE    = 0x1c1bb5d748c1
ROBOT_ID_MECANUM       = 0x2ccf672733ba 
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
        "bluetoothAddr": "F0:F1:F2:F3:F4:F5",
        "bluetoothUSB": False,
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "gearRatio": 20.0,
        "maxSpeedX": 0.4,
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.5,
        "toolMotor": True,
        "camW": 640,
        "camH": 480,
    },

    ROBOT_ID_ALEX: { 
        "name": "owlRobot (Alex)",
        "type": ROBOT_TYPE_DIFF_DRIVE,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F4",        
        "bluetoothUSB": True, 
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "gearRatio": 20.0,        
        "maxSpeedX": 0.4,        
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.5,
        "toolMotor": True,
        "camW": 640,
        "camH": 480,
    },

    ROBOT_ID_MECANUM: { 
        "name": "owlRobot (Mecanum)",
        "type": ROBOT_TYPE_MECANUM,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F3",        
        "bluetoothUSB": False, 
        "wheelToBodyCenterX": 0.25,
        "wheelToBodyCenterY": 0.25,
        "wheelDiameter": 0.15,
        "gearRatio": 20.0,        
        "maxSpeedX": 0.4,      
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.8,  
        "toolMotor": False, 
        "camW": 1280,
        "camH": 720,
    },

}


# -----------------------------------------------------------------------------------

# create robot object based on machine id (WiFi MAC) found in config
def createRobot():
    machine = platform.machine()  # x86_64,  aarch64   etc.
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

    # create robot object
    robot = None
    if cfg['type'] == ROBOT_TYPE_DIFF_DRIVE:
        print('creating diff drive robot...')
        robot = diffdrive.DifferentialDriveRobot(
            cfg['name'],
            cfg['wheelToBodyCenterY'], 
            cfg['wheelDiameter'],
            cfg['gearRatio'])   # wheel-center-y,  wheel-dia

        # tool motor
        if cfg['toolMotor']:        
            print('adding tool motor...')
            robot.toolMotor = owl.Motor(robot, owl.TOOL_MOTOR_NODE_ID, 'toolMotor') 

    elif cfg['type'] == ROBOT_TYPE_MECANUM:
        print('creating mecanum robot...')
        robot = mecanum.MecanumRobot(
            cfg['name'],
            cfg['wheelToBodyCenterX'],
            cfg['wheelToBodyCenterY'],
            cfg['wheelDiameter'],
            cfg['gearRatio'])         # wheel-center-x,  wheel-center-y,  wheel-dia
    else:
        print('invalid robot type')
        return None

    # max body velocities
    robot.maxSpeedX = cfg['maxSpeedX'] 
    robot.maxSpeedY = cfg['maxSpeedY']     
    robot.maxSpeedTheta = cfg['maxSpeedTheta']     

    # bluetooth config
    robot.bluetoothUSB = cfg['bluetoothUSB'] 
    robot.bluetoothAddr = cfg['bluetoothAddr']     

    # cam config
    robot.camW = cfg['camW']
    robot.camH = cfg['camH']
     
    return robot



# create dabble object 
def createDabble(aRobot):
    useUSB = aRobot.bluetoothUSB
    print('bluetoothUSB', useUSB)
    if useUSB:            
        socket = 'usb:0'
    else:
        socket = 'hci-socket:0'

    app = dabble.Dabble(socket, aRobot.name, aRobot.bluetoothAddr)
    return app


