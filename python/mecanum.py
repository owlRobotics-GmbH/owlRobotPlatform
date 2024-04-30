#!/usr/bin/env python

# owlRobotics robot platform  - mecanum drive robot  
# transfers body velocities into motor velocities and vice-versa

import math
import time
import owlrobot as owl


class MecanumRobot(owl.Robot):
    def __init__(self, aName, aWheelToBodyCenterX, aWheelToBodyCenterY, aWheelDiameter):        
        super().__init__(aName)
        # wheel diameter (m)
        self.wheelDiameter = aWheelDiameter
        self.wheelToBodyCenterX = aWheelToBodyCenterX;
        self.wheelToBodyCenterY = aWheelToBodyCenterY; 

        self.leftBackMotor = owl.Motor(self, owl.LEFT_BACK_MOTOR_NODE_ID, 'leftBackMotor')  
        self.rightBackMotor = owl.Motor(self, owl.RIGHT_BACK_MOTOR_NODE_ID, 'rightBackMotor')
        self.leftFrontMotor = owl.Motor(self, owl.LEFT_FRONT_MOTOR_NODE_ID, 'leftFrontMotor')  
        self.rightFrontMotor = owl.Motor(self, owl.RIGHT_FRONT_MOTOR_NODE_ID, 'rightFrontMotor')

      
    # compute forward kinematics based on motor sensors
    def forwardKinematics(self):      
        # compute forward kinematics (measured motor velocitities => body velocities)
        #     l1: wheel-axis to body center horizontal distance (m)
        #     l2: wheel-axis to body center vertical distance (m)      
        #     vx, vy, oz: body speed (m/s) (rad/s)
        #     o1, o2, o3, o4: angular wheel speed  (rad/s)
        #     R: wheel radius (m)
        R = self.wheelDiameter / 2.0
        l1 = self.wheelToBodyCenterX
        l2 = self.wheelToBodyCenterY
        o1 = self.leftFrontMotor.getSpeed()
        o2 = self.rightFrontMotor.getSpeed()
        o3 = self.leftBackMotor.getSpeed()
        o4 = self.rightBackMotor.getSpeed()

        dt = (time.time() - self.lastDriveTime) 
        self.lastDriveTime = time.time()
        self.odoVelX     = ( o1 + o2 + o3 + o4) * R / 4.0
        self.odoVelY     = (-o1 + o2 + o3 - o4) * R / 4.0
        self.odoVelTheta = (-o1 + o2 - o3 + o4) * R / (4.0 * (l1+l2))      
        self.odoX += (self.odoVelX * math.cos(self.odoTheta) - self.odoVelY * math.sin(self.odoTheta)) * dt
        self.odoY += (self.odoVelX * math.sin(self.odoTheta) + self.odoVelY * math.cos(self.odoTheta)) * dt
        self.odoTheta += self.odoVelTheta * dt



    #  transfers body velocities into motor velocities and apply velocities
    #    set inverse kinematics (body velocities => motor velocities)
    #    x: forward velocity (m/s)
    #    y: sideways velocity (m/s)
    #    theta: rotational veloctiy (rad/s)

    def setRobotSpeed(self, vx, vy, oz):

        self.cmdVelX = vx   # forward velocity command (m/s)
        self.cmdVelY = vy   # sideways velocity command (m/s)
        self.cmdVelTheta = oz # rotational velocity command (rad/s) 

        # compute inverse kinematics (body velocity commands => motor velocities)
        # https://ecam-eurobot.github.io/Tutorials/software/mecanum/mecanum.html
        #     l1: wheel-axis to body center horizontal distance (m)
        #     l2: wheel-axis to body center vertical distance (m)      
        #     vx, vy, oz: body speed (m/s) (rad/s)
        #     o1, o2, o3, o4: angular wheel speed  (rad/s)
        #     R: wheel radius (m)

        R = self.wheelDiameter / 2.0
        l1 = self.wheelToBodyCenterX
        l2 = self.wheelToBodyCenterY
        o1 = (vx - vy - (l1+l2)* oz) / R
        o2 = (vx + vy + (l1+l2)* oz) / R
        o3 = (vx + vy - (l1+l2)* oz) / R
        o4 = (vx - vy + (l1+l2)* oz) / R

        self.leftFrontMotor.setSpeed(o1) # M_fl
        self.rightFrontMotor.setSpeed(o2) # M_fr
        self.leftBackMotor.setSpeed(o3) # M_bl
        self.rightBackMotor.setSpeed(o4) # M_br

    

    


if __name__ == "__main__":

    robot = MecanumRobot('test', 0.4, 0.2, 0.1) # wheel-center-x,  wheel-center-y,  wheel-dia

    while True:
        time.sleep(1.0)
        print('----')
        robot.setRobotSpeed(0, 0.1, 0)   # vx, vy, oz
        robot.forwardKinematics()    
        robot.print()



    



