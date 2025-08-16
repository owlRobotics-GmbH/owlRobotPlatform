#!/usr/bin/env python

# owlRobotics robot platform config 
# robot database and robot object factory

import diffdrive
import mecanum
import owlrobot as owl
import psutil



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
      '2c:cf:67:26:9c:ec': {                      # robot ID (MAC address)
        "name": "owlMower (DiffDrive)",           # Bluetooth name
        "type": ROBOT_TYPE_DIFF_DRIVE,            # robot type
        "bluetoothAddr": "F0:F1:F2:F3:F4:F5",     # Bluetooth address
        "bluetoothUSB": False,                    # Bluetooth USB interface? 
        "swapLeftMotor": True,                    # swap left motor?
        "swapRightMotor": False,                  # swap right motor?
        "wheelToBodyCenterY": 0.24,               # wheel-to-axle-center Y axis (m)
        "wheelDiameter": 0.26,                    # wheel diameter (m)
        "gearRatio": 250.0,                        # gear ratio  (1.0 = no gears)
        "maxSpeedX": 0.60,                         # max speed in X axis (m/s)        
        "maxSpeedY": 0.60,                         # max speed in Y axis (m/s)
        "maxSpeedTheta": 0.5,                     # max angular speed (rad/s) 
        "toolMotor": True,                        # has tool motor?
        "camW": 640,                              # camera with resolution (pixels)
        "camH": 480,                              # camera height resolution (pixels)
        "camLookingForward": True,                # camera is looking forward?
    },

    '2c:cf:67:26:9b:af': {                        # robot ID (MAC address)
        "name": "owlCrawler (DiffDrive)",  # Bluetooth name
        "type": ROBOT_TYPE_DIFF_DRIVE,            # robot type
        "bluetoothAddr": "F0:F1:F2:F3:F4:F5",     # Bluetooth address
        "bluetoothUSB": False,                    # Bluetooth USB interface? 
        "swapLeftMotor": True,                    # swap left motor?
        "swapRightMotor": False,                  # swap right motor?
        "wheelToBodyCenterY": 0.22,               # wheel-to-axle-center Y axis (m)
        "wheelDiameter": 0.225,                    # wheel diameter (m)
        "gearRatio": 103.2,                        # gear ratio  (1.0 = no gears)   23,2
        "maxSpeedX": 1.38,                      # max speed in X axis (m/s)        
        "maxSpeedY": 1.38,                         # max speed in Y axis (m/s)
        "maxSpeedTheta": 1.5,                     # max angular speed (rad/s) 
        "toolMotor": True,                        # has tool motor?
        "camW": 640,                              # camera with resolution (pixels)
        "camH": 480,                              # camera height resolution (pixels)
        "camLookingForward": True,                # camera is looking forward?
    },

    '2c:cf:67:27:32:cb': {                        # robot ID (MAC address)
        "name": "owlRobot (DiffDrive, Bernd)",    # Bluetooth name
        "type": ROBOT_TYPE_DIFF_DRIVE,            # robot type
        "bluetoothAddr": "F0:F1:F2:F3:F4:F5",     # Bluetooth address
        "bluetoothUSB": False,                    # Bluetooth USB interface? 
        "swapLeftMotor": True,                    # swap left motor?
        "swapRightMotor": False,                  # swap right motor?
        "wheelToBodyCenterY": 0.22,               # wheel-to-axle-center Y axis (m)
        "wheelDiameter": 0.30,                    # wheel diameter (m)
        "gearRatio": 100.0,                       # gear ratio  (1.0 = no gears)
        "maxSpeedX": 1.0,                         # max speed in X axis (m/s)        
        "maxSpeedY": 0.4,                         # max speed in Y axis (m/s)
        "maxSpeedTheta": 2.5,                     # max angular speed (rad/s) 
        "toolMotor": True,                        # has tool motor?
        "camW": 640,                              # camera with resolution (pixels)
        "camH": 480,                              # camera height resolution (pixels)
        "camLookingForward": True,                # camera is looking forward?
    },

    '00:d8:61:05:af:39': { 
        "name": "owlRobot (Laptop, Alex)",
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

    '2c:cf:67:27:33:b9': { 
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


    'c0:74:2b:fe:83:40': { 
        "name": "owlRobot (ROS)",
        "type": ROBOT_TYPE_DIFF_DRIVE,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F2",        
        "bluetoothUSB": False, 
        "swapLeftMotor": True,                    # swap left motor?
        "swapRightMotor": False,                  # swap right motor?
        "wheelToBodyCenterY": 0.1,                # wheel-to-axle-center Y axis (m)
        "wheelDiameter": 0.12,                    # wheel diameter (m)
        "gearRatio": 2000.0,                       # gear ratio  (1.0 = no gears)
        "maxSpeedX": 0.4,                         # max speed in X axis (m/s)        
        "maxSpeedY": 0.4,                         # max speed in Y axis (m/s)
        "maxSpeedTheta": 2.5,                     # max angular speed (rad/s) 
        "toolMotor": False,                       # has tool motor?
        "camW": 640,                              # camera with resolution (pixels)
        "camH": 480,                              # camera height resolution (pixels)
        "camLookingForward": True,                # camera is looking forward?
    },

    'xx:xx:xx:xx:xx:xx': { 
        "name": "owlRobot (4WD, Alex)",
        "type": ROBOT_TYPE_MECANUM,
        "bluetoothAddr": "F0:F1:F2:F3:F4:F2",        
        "bluetoothUSB": False, 
        "swapLeftBackMotor": True,
        "swapRightBackMotor": False,
        "swapLeftFrontMotor": True,
        "swapRightFrontMotor": False,
        "wheelToBodyCenterX": 0.27,         # wheel-to-front/back-axle-center in X axis (m)
        "wheelToBodyCenterY": 0.26,         # wheel-to-left/right-axle-center in Y axis (m)
        "wheelDiameter": 0.16,
        "gearRatio": 1.0,        
        "maxSpeedX": 0.2,      
        "maxSpeedY": 0.2,
        "maxSpeedTheta": 0.2,  
        "toolMotor": False, 
        "camW": 1280,
        "camH": 720,
        "camLookingForward": True,
    },

    '2c:cf:67:27:34:15': {                        # robot ID (MAC address)
        "name": "owlRobot (Demo Board)",    # Bluetooth name
        "type": ROBOT_TYPE_DIFF_DRIVE,            # robot type
        "bluetoothAddr": "F0:F1:F2:F3:F4:F5",     # Bluetooth address
        "bluetoothUSB": False,                    # Bluetooth USB interface? 
        "swapLeftMotor": True,                    # swap left motor?
        "swapRightMotor": False,                  # swap right motor?
        "wheelToBodyCenterY": 0.22,               # wheel-to-axle-center Y axis (m)
        "wheelDiameter": 0.30,                    # wheel diameter (m)
        "gearRatio": 100.0,                       # gear ratio  (1.0 = no gears)
        "maxSpeedX": 1.0,                         # max speed in X axis (m/s)        
        "maxSpeedY": 0.4,                         # max speed in Y axis (m/s)
        "maxSpeedTheta": 2.5,                     # max angular speed (rad/s) 
        "toolMotor": True,                        # has tool motor?
        "camW": 640,                              # camera with resolution (pixels)
        "camH": 480,                              # camera height resolution (pixels)
        "camLookingForward": True,                # camera is looking forward?
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
    import dabble

    useUSB = aRobot.bluetoothUSB
    print('bluetoothUSB', useUSB)
    if useUSB:            
        socket = 'usb:0'
    else:
        socket = 'hci-socket:0'

    app = dabble.Dabble(socket, aRobot.name, aRobot.bluetoothAddr)
    return app


