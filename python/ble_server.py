#!/usr/bin/env python

# owlRobotics robot platform example code:   Bluetooth Low Energy Server (use Dabble App joystick to control motors)

# this demo uses motor velocities (instead of body velocities) 


import owlrobot
import time
import dabble
import os
import detect_object

 
app = dabble.Dabble('hci-socket:0')
#app = dabble.Dabble('usb:0')
robot = owlrobot.Robot()

VISIBLE = False
MAX_SPEED = 100.0  # rpm


print('press CTRL+C to exit...')

toolMotorSpeed = 0
circleButtonTime = 0
nextCanTime = 0
followMe = False
trackTimeout = 0
oscillateLeft = True
oscillateTimeout = 0


while True:
    if not dabble.connected: continue    
    #print('.', end="", flush=True)

    if app.extraButton == 'select':
        if time.time() > circleButtonTime:
            circleButtonTime = time.time() + 0.5
            followMe = not followMe
            print('followMe', followMe)
    elif app.extraButton == 'triangle':
        MAX_SPEED = 300.0
    elif app.extraButton == 'cross':
        MAX_SPEED = 100.0
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


    speedLeft = 0
    speedRight = 0

    if followMe: 
        stopTime = time.time() + 0.1
        while time.time() < stopTime:
            img = detect_object.captureVideoImage()
        if not img is None:
            cx,cy,y = detect_object.detectObject(img, "person", VISIBLE)
            if y > 0 and cx > 0 and cy > 0:
                if cx > 0.6:
                    # rotate right
                    speedLeft = MAX_SPEED/5
                    speedRight = -MAX_SPEED/5
                    trackTimeout = time.time() + 2.0
                elif cx < 0.4:
                    # rotate left
                    speedLeft = -MAX_SPEED/5
                    speedRight = MAX_SPEED/5
                    trackTimeout = time.time() + 2.0
                elif y > 0.2 and y < 0.7:
                    # forward
                    speedLeft = -MAX_SPEED
                    speedRight = -MAX_SPEED         
                    trackTimeout = time.time() + 2.0       
        if time.time() > trackTimeout:
            # oscillate
            if time.time() > oscillateTimeout:
                oscillateTimeout = time.time() + 2.0       
                oscillateLeft = not oscillateLeft
            speedLeft = -MAX_SPEED/5
            speedRight = MAX_SPEED/5
            if oscillateLeft: 
                speedLeft *= -1
                speedRight *= -1

    else:

        if app.analogMode:
            if app.y_value >= 0:
                speedLeft = (app.y_value + app.x_value*0.3) * MAX_SPEED
                speedRight = (app.y_value - app.x_value*0.3) * MAX_SPEED
            else:
                speedLeft = (app.y_value - app.x_value*0.3) * MAX_SPEED
                speedRight = (app.y_value + app.x_value*0.3) * MAX_SPEED            

        else:
            if app.joystickButton == 'up':
                speedLeft = MAX_SPEED
                speedRight = MAX_SPEED

            elif app.joystickButton == 'down':        
                speedLeft = -MAX_SPEED
                speedRight = -MAX_SPEED

            elif app.joystickButton == 'right':        
                speedLeft = MAX_SPEED
                speedRight = -MAX_SPEED

            elif app.joystickButton == 'left':        
                speedLeft = -MAX_SPEED
                speedRight = MAX_SPEED

            elif app.joystickButton == 'released':
                speedLeft = 0
                speedRight = 0


    if time.time() > nextCanTime:
        nextCanTime = time.time() + 0.1
        try:
            robot.motorSpeedDifferential(-speedLeft, speedRight, toolMotorSpeed)
        except:
            print('error sending CAN')




