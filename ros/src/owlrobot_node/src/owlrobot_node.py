#!/usr/bin/env python

# ROS node for the owlRobot platform

import roslib; roslib.load_manifest("owlrobot_node")
import rospy
from math import sin,cos

#from sensor_msgs.msg import LaserScan
from geometry_msgs.msg import Quaternion
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry
from tf.broadcaster import TransformBroadcaster


import sys
import os
import getpass


print('Python ver:', sys.version)

print('cwd:', os.getcwd())
username = getpass.getuser()
print('user:', username)
sys.path.append(os.path.abspath("/home/" + username + "/owlRobotPlatform/python"))
print(sys.path)

import config
import owlrobot as owl



class owlRobotNode:

    def __init__(self):
        rospy.init_node('owlrobot')
        rospy.loginfo("starting owlrobot node")

        # create robot from database
        self.robot = config.createRobot()
        if self.robot is None: exit()


        rospy.Subscriber("cmd_vel", Twist, self.cmdVelCb)
        self.odomPub = rospy.Publisher('odom', Odometry, queue_size=10)
        self.odomBroadcaster = TransformBroadcaster()

        self.cmd_vel = [0,0] 


    def spin(self):        
        encoders = [0,0]

        self.x = 0                  # position in xy plane
        self.y = 0
        self.th = 0
        then = rospy.Time.now()

        # things that don't ever change
        odom = Odometry(header=rospy.Header(frame_id="odom"), child_frame_id='base_link')
    
        # main loop of driver
        r = rospy.Rate(5)
        while not rospy.is_shutdown():                        
            # send updated movement commands
            self.robot.setRobotSpeed(self.cmd_vel[0], 0, self.cmd_vel[1]) # linear speed x, linear speed y, angular speed
            #if not robot.toolMotor is None:
            #    robot.toolMotor.setSpeed(toolMotorSpeed)

            # get motor encoder values
            
            # prepare tf from base_link to odom
            quaternion = Quaternion()
            quaternion.z = sin(self.robot.odoTheta/2.0)
            quaternion.w = cos(self.robot.odoTheta/2.0)

            # prepare odometry
            odom.header.stamp = rospy.Time.now()
            odom.pose.pose.position.x = self.robot.odoX
            odom.pose.pose.position.y = self.robot.odoY
            odom.pose.pose.position.z = 0
            odom.pose.pose.orientation = quaternion
            odom.twist.twist.linear.x = self.robot.odoVelX
            odom.twist.twist.angular.z = self.robot.odoVelTheta


            # publish everything
            self.odomBroadcaster.sendTransform( (self.robot.odoX, self.robot.odoY, 0), (quaternion.x, quaternion.y, quaternion.z, quaternion.w),
                then, "base_link", "odom" )
            self.odomPub.publish(odom)

            # wait, then do it again
            r.sleep()

        # shut down
        

    def cmdVelCb(self,req):
        self.cmd_vel = [ req.linear.x , req.angular.z ]


if __name__ == "__main__":    
    node = owlRobotNode()
    node.spin()


