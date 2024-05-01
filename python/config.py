#!/usr/bin/env python

# owlRobotics robot platform config 
# robot database and robot object factory

import diffdrive
import mecanum
import dabble
import owlrobot as owl
import psutil


# robot IDs (WiFi MAC address):
ROBOT_ID_DIFF_DRIVE    = 'ff:ff:ff:ff:ff:ff'
ROBOT_ID_MECANUM       = '2c:cf:67:27:33:b9' 
ROBOT_ID_ALEX          = '00:d8:61:05:af:39'

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
        "swapLeftMotor": True,
        "swapRightMotor": False,        
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "gearRatio": 20.0,
        "maxSpeedX": 0.4,
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.5,
        "toolMotor": True,
        "camW": 640,
        "camH": 480,
        "camLookingForward": False,
    },

    ROBOT_ID_ALEX: { 
        "name": "owlRobot (Alex)",
        "type": ROBOT_TYPE_DIFF_DRIVE,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F4",        
        "bluetoothUSB": True, 
        "swapLeftMotor": True,
        "swapRightMotor": False,        
        "wheelToBodyCenterY": 0.2,
        "wheelDiameter": 0.15,
        "gearRatio": 20.0,        
        "maxSpeedX": 0.4,        
        "maxSpeedY": 0.4,
        "maxSpeedTheta": 0.5,
        "toolMotor": True,
        "camW": 640,
        "camH": 480,
        "camLookingForward": False,
    },

    ROBOT_ID_MECANUM: { 
        "name": "owlRobot (Mecanum)",
        "type": ROBOT_TYPE_MECANUM,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F3",        
        "bluetoothUSB": False, 
        "swapLeftBackMotor": True,
        "swapRightBackMotor": False,
        "swapLeftFrontMotor": True,
        "swapRightFrontMotor": False,
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
        "camLookingForward": True,
    },

}


# -----------------------------------------------------------------------------------

def get_mac_addresses(family):
    for interface, snics in psutil.net_if_addrs().items():
        for snic in snics:
            if snic.family == family:
                yield (interface, (snic.address))




# create robot object based on machine id (WiFi MAC) found in config
def createRobot():

    macs = dict(get_mac_addresses(psutil.AF_LINK))
    print('found MAC addresses:')
    print(macs)

    # find any matching MAC...
    cfg = None
    for key, value in macs.items():
        #print('looking for:', value)
        for aid in ROBOTS:
            if aid == value:
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
            cfg['gearRatio'],
            cfg['swapLeftMotor'],
            cfg['swapRightMotor'])   # wheel-center-y,  wheel-dia

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
            cfg['gearRatio'],
            cfg['swapLeftBackMotor'],
            cfg['swapRightBackMotor'],
            cfg['swapLeftFrontMotor'],
            cfg['swapRightFrontMotor'])         # wheel-center-x,  wheel-center-y,  wheel-dia
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
    robot.camLookingForward = cfg['camLookingForward'] 

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


