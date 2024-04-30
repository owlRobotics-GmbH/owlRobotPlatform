#!/usr/bin/env python

# owlRobotics robot platform example code:   Bluetooth Low Energy Server (use Dabble App joystick to control motors)

# this demo uses robot database and body velocities  (instead of motor velocities)


import owlrobot as owl
import time
import dabble
import os
import config
import detect_object


# create robot from database
robot = config.createRobot()
if robot is None: exit()

# create dabble app interface
app = config.createDabble(robot)

detect_object.IMG_W = robot.camW
detect_object.IMG_H = robot.camH


VISIBLE = False

# max. robot body speeds (translation / angular)
MAX_LINEAR_SPEED = robot.maxSpeedX / 2.0  # m/s
MAX_ANGULAR_SPEED = robot.maxSpeedTheta / 2.0   # rad/s 


print('press CTRL+C to exit...')

toolMotorSpeed = 0
circleButtonTime = 0
nextCanTime = 0
followMe = False
trackTimeout = 0
oscillateLeft = True
oscillateTimeout = 0
sideways = False 


while True:
    time.sleep(0.01)

    if not dabble.connected: continue    
    #print('.', end="", flush=True)

    if app.extraButton == 'select':
        if time.time() > circleButtonTime:
            circleButtonTime = time.time() + 0.5
            followMe = not followMe
            print('followMe', followMe)
    elif app.extraButton == 'start':
        if time.time() > circleButtonTime:
            circleButtonTime = time.time() + 0.5
            sideways = not sideways
            print('sideways', sideways)
    elif app.extraButton == 'triangle':
        MAX_LINEAR_SPEED = robot.maxSpeedX
        MAX_ANGULAR_SPEED = robot.maxSpeedTheta
    elif app.extraButton == 'cross':
        MAX_LINEAR_SPEED = robot.maxSpeedX / 2.0
        MAX_ANGULAR_SPEED = robot.maxSpeedTheta / 2.0
    elif app.extraButton == 'circle':
        if time.time() > circleButtonTime:
            circleButtonTime = time.time() + 0.5
            if toolMotorSpeed == 0:            
                toolMotorSpeed = 100
            elif toolMotorSpeed == 100:
                toolMotorSpeed = 300
            else: toolMotorSpeed = 0
            print('toolMotorSpeed', toolMotorSpeed)
    elif app.extraButton == 'rectangle':
        os.system('shutdown now')


    speedLinearX = 0      # forward speed
    speedLinearY = 0      # sideward speed
    speedAngular = 0      # rotational speed


    if followMe: 
        stopTime = time.time() + 0.1
        while time.time() < stopTime:
            img = detect_object.captureVideoImage()
        if not img is None:
            cx,cy,y = detect_object.detectObject(img, "person", VISIBLE)
            if y > 0 and cx > 0 and cy > 0:
                if cx > 0.6:
                    # rotate right
                    speedAngular = -MAX_ANGULAR_SPEED 
                    trackTimeout = time.time() + 2.0
                elif cx < 0.4:
                    # rotate left
                    speedAngular = MAX_ANGULAR_SPEED
                    trackTimeout = time.time() + 2.0
                elif y > 0.2 and y < 0.7:
                    # forward
                    speedLinearX = MAX_LINEAR_SPEED
                    trackTimeout = time.time() + 2.0       
        if time.time() > trackTimeout:
            # oscillate
            if time.time() > oscillateTimeout:
                oscillateTimeout = time.time() + 2.0       
                oscillateLeft = not oscillateLeft
            speedAngular = MAX_ANGULAR_SPEED
            if oscillateLeft: 
                speedAngular *= -1
                

    else:

        if app.analogMode:
            #print(app.y_value, app.x_value)
            speedLinearX = app.y_value * MAX_LINEAR_SPEED            
            if app.y_value >= 0:
                sign = -1
            else:
                sign = 1 
            if sideways:
                speedLinearY = sign * app.x_value * MAX_LINEAR_SPEED
            else:
                speedAngular = sign * app.x_value * MAX_ANGULAR_SPEED

        else:
            if app.joystickButton == 'up':
                speedLinearX = MAX_LINEAR_SPEED

            elif app.joystickButton == 'down':        
                speedLinearX = -MAX_LINEAR_SPEED

            elif app.joystickButton == 'right':        
                if sideways:
                    speedLinearY = -MAX_LINEAR_SPEED
                else:
                    speedAngular = -MAX_ANGULAR_SPEED

            elif app.joystickButton == 'left':        
                if sideways:
                    speedLinearY = MAX_LINEAR_SPEED                
                else:
                    speedAngular = MAX_ANGULAR_SPEED

            elif app.joystickButton == 'released':
                pass


    if time.time() > nextCanTime:
        nextCanTime = time.time() + 0.1
        robot.setRobotSpeed(speedLinearX, speedLinearY, speedAngular)
        if not robot.toolMotor is None:
            robot.toolMotor.setSpeed(toolMotorSpeed)
        



