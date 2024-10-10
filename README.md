# owlRobotPlatform
'owlRobotics robot platform' is an **universal hardware platform** for your robotics projects. It contains:

1. owlControl: a PCB with an integrated **Raspberry Pico (RP2040)** with an integrated **CAN bus** interface. Optionally, it contains a breakout for a **Raspberry PI** (the 'brain'). The Raspberry, the Pico and your own hardware can use the CAN bus for communication.
   
2. owlDrive (optional): motor drivers for **brushless motors** (BLDC) - it has an intetgrated CAN bus interface.   

NOTE: The owlRobotPlatform is compatible with the Ardumower Sunray firmware. Sunray is a firmware for your Do-It-Yourself robot mower. More details: https://github.com/Ardumower/Sunray


This repository contains  interface code (App, Python etc.) and examples A for the owlRobotPlatform.  

![owlplatform](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/f4a7ead1-3a4a-428f-946e-b021bfd83857)

Supported/tested platforms:

1. Differential drive platform (2 wheel motors)

2. Mecanum platform (4 wheel motors)

3. Your custom robot platform (up to 63 motors)

   
![Screenshot from 2024-05-01 08-29-33](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/41efcbc9-595b-408d-b5af-a34705f70225)


Supported/tested hardware:
1. owlRobotics motor drivers, RP2040 controller, Raspberry PI etc. connected via CAN bus:
   
https://owlrobotics.de/index.php/en/products/hardware-products/owldrive-the-smart-brushless-driver



## Python interface demo

Used Python libraries:
* CAN bus communication: https://github.com/hardbyte/python-can
* Bluetooth Low Energery communication (Dabble App): https://github.com/google/bumble
* Camera person detection (follow-me): https://github.com/opencv/opencv-python


1. On your Raspberry PI, verify that the CAN driver is installed:
```
## edit boot config ##
sudo nano /boot/firmware/config.txt

## and add this line ##
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=6
```
   
2. Install Dabble App:  

* [Android](https://play.google.com/store/apps/details?id=io.dabbleapp)
* [iOS](https://apps.apple.com/ch/app/dabble-bluetooth-controller/id1472734455)

3. Run in Raspberry PI terminal (will install all required Python libs etc.):

```
## checkout repository ##
cd ~
git clone https://github.com/owlRobotics-GmbH/owlRobotPlatform
##  (optionally, as developer):  git clone git@github.com:owlRobotics-GmbH/owlRobotPlatform

## run bash script ##
cd owlRobotPlatform/python
sudo ./run_ble_server.sh
```

4. Start Dabble App, connect with 'owlRobot'and choose button 'Gamedpad' to steer robot (you can switch between digital mode, joystick mode and accelerometer mode)
![Screenshot from 2024-04-10 18-26-50](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/3485eaab-0ced-49aa-adff-f4493f62f156)

5. Verify that the Python script receives commands via Bluetooth when pressing buttons (up, down, left, right) in the Dabble App. Also verify (using 'candump can0') that the Python script triggers CAN packets on the CAN bus:
![Screenshot from 2024-04-10 19-16-11](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/6996b1b3-0524-40ae-a002-4195df0f0372)

The extra buttons have the following actions:
```
triangle: high motor speed
cross: low motor speed
circle: toggle tool motor speed
rectangle: shutdown Raspberry PI
select: camera-based follow-me mode on/off
start: toggle between sideways motion and rotation (only Mecanum platform)
```

6. In order to install the demo Python script as a Linux (autostart) service:

```
sudo ./service.sh

1) Start ble_server service	    3) Start ble_server service		5) List services
2) Stop ble_server service	    4) Show ble_server service console	6) Quit
Please enter your choice: 
```

and choose point 1   ('Start ble_server service')


## ROS interface

The ROS node driver uses the Python library to drive the motors. How to try out the ROS node driver:

1. Go into ROS node driver folder:

```
cd ros
```

2. Run ROS 'catkin_make':
```
catkin_make
```

3. 'Source' ROS files:
```
source devel/setup.bash
```
 
4. In a new terminal, run 'roscore':
```
roscore
```

5. Run ROS node driver:
```
roslaunch owlrobot_node bringup.launch
```

6. In a new terminal, send velocity command (x axis) to node driver:
```
rostopic pub /cmd_vel geometry_msgs/Twist -r 3 -- '[0.1,0.0,0.0]' '[0.0, 0.0, 0.0]'
```
The motors should rotate forward (0.1 m/s).


## Ardumower Sunray firmware
The owlRobotPlatform is compatible with the Ardumower Sunray firmware. Sunray is a firmware for your Do-It-Yourself robot mower. More details: https://github.com/Ardumower/Sunray





