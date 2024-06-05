#!/usr/bin/env python

# owlRobotics robot platform  - differential drive robot  
# transfers body velocities into motor velocities and vice-versa

import sys
import math
import time
import owlrobot as owl


class DifferentialDriveRobot(owl.Robot):
    def __init__(self, aName, aWheelToBodyCenterY, aWheelDiameter, aGearRatio, aSwapLeftMotor, aSwapRightMotor):        
        owl.Robot.__init__(self, aName)
        
        # wheel diameter (m)
        self.wheelDiameter = aWheelDiameter
        self.wheelToBodyCenterX = 0
        self.wheelToBodyCenterY = aWheelToBodyCenterY 

        self.leftMotor = owl.Motor(self, owl.LEFT_MOTOR_NODE_ID, 'leftMotor', aSwapLeftMotor, aGearRatio)  
        self.rightMotor = owl.Motor(self, owl.RIGHT_MOTOR_NODE_ID, 'rightMotor', aSwapRightMotor, aGearRatio)   
    
      
    # compute forward kinematics based on motor sensors
    def forwardKinematics(self):      
        # compute forward kinematics (measured motor velocitities => body velocities)

        dt = (time.time() - self.lastDriveTime) 
        self.lastDriveTime = time.time()

        # linear: m/s
        # angular: rad/s
        # -------unicycle model equations----------
        #      L: wheel-to-wheel distance
        #     VR: right speed (m/s)
        #     VL: left speed  (m/s)
        #  omega: rotation speed (rad/s)
        #      V     = (VR + VL) / 2       =>  VR = V + omega * L/2
        #      omega = (VR - VL) / L       =>  VL = V - omega * L/2

        L = self.wheelToBodyCenterY * 2.0
        VL = self.leftMotor.getSpeed()
        VR = self.rightMotor.getSpeed()
        
        self.odoVelX     = (VR + VL) / 2.0
        self.odoVelY     = 0
        self.odoVelTheta = (VR - VL) / L
        self.odoX += (self.odoVelX * math.cos(self.odoTheta) - self.odoVelY * math.sin(self.odoTheta)) * dt
        self.odoY += (self.odoVelX * math.sin(self.odoTheta) + self.odoVelY * math.cos(self.odoTheta)) * dt
        self.odoTheta += self.odoVelTheta * dt



    #  transfers body velocities into motor velocities and apply velocities
    #    set inverse kinematics (body velocities => motor velocities)
    #    x: forward velocity (m/s)
    #    y: sideways velocity (m/s)
    #    theta: rotational veloctiy (rad/s)

    def setRobotSpeed(self, vx, vy, oz):

        cmdVelX = vx    # forward velocity command (m/s)
        cmdVelY = 0     # sideways velocity command (m/s)
        cmdVelTheta = oz  # rotational velocity command (rad/s) 

        # compute inverse kinematics (body velocity commands => motor velocities)
        # linear: m/s
        # angular: rad/s
        # -------unicycle model equations----------
        #      L: wheel-to-wheel distance
        #     VR: right speed (m/s)
        #     VL: left speed  (m/s)
        #  omega: rotation speed (rad/s)
        #      V     = (VR + VL) / 2       =>  VR = V + omega * L/2
        #      omega = (VR - VL) / L       =>  VL = V - omega * L/2

        L = self.wheelToBodyCenterY * 2.0
        VR = vx + oz * L/2
        VL = vx - oz * L/2

        self.leftMotor.setSpeed(VL); 
        self.rightMotor.setSpeed(VR); 



if __name__ == "__main__":

    robot = DifferentialDriveRobot('test', 0.2, 0.2, 20.0, True, False)   # wheel-center-x,  wheel-dia, gear-ratio, swap-left, swap-right

    while True:
        time.sleep(1.0)
        print('----')
        robot.setRobotSpeed(0.1, 0, 0.02)  # vx, vy, oz
        robot.forwardKinematics()
        robot.log()

    



