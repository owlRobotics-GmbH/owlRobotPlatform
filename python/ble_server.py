#!/usr/bin/env python

# owlRobotics robot platform example code:   Bluetooth Low Energy Server (use Dabble App joystick to control motors)

import owlrobot
import time
import dabble

 
app = dabble.Dabble('hci-socket:0')
#app = dabble.Dabble('usb:0')
robot = owlrobot.Robot()

MAX_SPEED = 100.0  # rpm

print('press CTRL+C to exit...')

while True:
    #print('.')

    if app.button == 'up':
        robot.motorSpeed(-MAX_SPEED, MAX_SPEED, 0)

    elif app.button == 'down':        
        robot.motorSpeed(MAX_SPEED, -MAX_SPEED, 0)

    elif app.button == 'right':        
        robot.motorSpeed(-MAX_SPEED, -MAX_SPEED, 0)

    elif app.button == 'left':        
        robot.motorSpeed(MAX_SPEED, MAX_SPEED, 0)

    elif app.button == 'released':
        robot.motorSpeed(0, 0, 0)

    time.sleep(0.2)





