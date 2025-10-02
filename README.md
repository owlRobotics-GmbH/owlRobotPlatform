# owlRobotPlatform

# Table of contents
1. [Description](#description)
2. [owlPlatform installation](#installation)
3. [Sunray for owlPlatform](#sunray_owl)
4. [Dabble App/Python interface demo for owlPlatform](#dabble)
5. [ROS for owlPlatform](#ros)
   

## Description <a name="description"></a>
'owlRobotics robot platform' is an **universal hardware platform** for your robotics projects. It contains:

1. owlControl: a PCB with an integrated **Raspberry Pico (RP2040)** with sockets for GPIO, UART, I2C, SPI etc. and an integrated **CAN bus** interface. Optionally, it contains a breakout for a **RaspberryPi/OrangePiPro** (the 'brain'). The RaspberryPi/OrangePiPro, the Pico and your own hardware can use the CAN bus for communication.
   
2. owlDrive (optional): motor drivers for **brushless motors** (BLDC) - it has an intetgrated CAN bus interface (More details: https://owlrobotics-store.company.site/products/) 

NOTE: The owlRobotPlatform is compatible with the Ardumower Sunray firmware. Sunray is a firmware for your Do-It-Yourself robot mower. More details: https://github.com/Ardumower/Sunray


This repository contains  interface code (App, Python, ROS etc.) and examples for the owlRobotPlatform.  

![owlplatform](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/f4a7ead1-3a4a-428f-946e-b021bfd83857)

Supported/tested platforms:

1. Differential drive platform (2 wheel motors)

2. Mecanum platform (4 wheel motors, platform can move forward/backwards/sidewards and rotate)

![Screenshot from 2024-05-01 08-29-33](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/41efcbc9-595b-408d-b5af-a34705f70225)


3. DIY 4WD platform with hoverboard motors (4 wheel hub motors)

![1715422181499_small](https://github.com/user-attachments/assets/17ac3450-d8f3-4c64-96bd-36eba9b7ffd6) ![1715200960151_small](https://github.com/user-attachments/assets/61df8fdd-978a-47dc-8950-e3b4a34cb854)


https://www.youtube.com/watch?v=J3IJpunduYg

4. Your custom robot platform (up to 63 motors)


Supported/tested hardware:
1. owlRobotics motor drivers, RP2040 controller, Raspberry Pi5/OrangePi5Pro etc. connected via CAN bus:
   
https://owlrobotics.de/index.php/en/products/hardware-products/owldrive-the-smart-brushless-driver

## owlPlatform installation (CAN bus driver installation etc.) <a name="installation"></a>


###  OrangePi5Pro:
1. Download and install Ubuntu Jammy (22.04, with Xfce window manager and Linux kernel 6.1)  https://drive.google.com/file/d/1CrvjhITZV2vE1qJ_pcYZjjQi0JqQDwxP/view?usp=drive_link
2. On your Orange Pi5Pro, run the CAN installation script in a terminal:

```
cd ~
git clone https://github.com/owlRobotics-GmbH/owlRobotPlatform
##  (optionally, as developer):  git clone git@github.com:owlRobotics-GmbH/owlRobotPlatform
cd tools/orangepi5pro
./orangepi5pro_conf.sh
```

### Raspberry4 / Raspberry5:
On your Raspberry Pi4/5, verify that the CAN driver is installed:
```
## edit boot config ##
sudo nano /boot/firmware/config.txt

## and add this line ##
dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=6
```
Also, for the Raspberry4, if the RP2040 doesn't switch on, you may have to change the default pin behavior for physical pin 12 to 'input pull-down':
1. install wiring-pi:   sudo apt install wiringpi
2. sudo nano /etc/rc.local  and add this contents:    
```
gpio mode 1 down
exit 0
```
4. make it executable:     sudo chmod +x /etc/rc.local
5. reboot:  sudo reboot now
6. you can see the actual physical pin state (physical pin 12 = wiring_pin 1) using the command 'gpio readall'.  It should be 'input, V=0'

<img width="640" height="310" alt="image" src="https://github.com/user-attachments/assets/9ca7d786-0350-4581-ba37-8033cb14f252" />

Physical pin12 = 0V (GND) : RP2040 switched on

Physical pin12 = 3.3V : RP2040 switched off


## Sunray for owlPlatform <a name="sunray_owl"></a>

The owlRobotPlatform is compatible with the Ardumower Sunray firmware. Sunray is a firmware for your Do-It-Yourself robot mower. 

1. Install owlPlatform drivers (see section owlPlatform installation above) 
2. Install Sunray firmware: https://github.com/Ardumower/Sunray


## Dabble App/Python interface demo <a name="dabble"></a>

Used Python libraries:
* CAN bus communication: https://github.com/hardbyte/python-can
* Bluetooth Low Energery communication (Dabble App): https://github.com/google/bumble
* Camera person detection (follow-me): https://github.com/opencv/opencv-python
     
1. Install Dabble App:  

* [Android](https://play.google.com/store/apps/details?id=io.dabbleapp)
* [iOS](https://apps.apple.com/ch/app/dabble-bluetooth-controller/id1472734455)

2. Run in Raspberry Pi5/OrangePi5Pro terminal (will install all required Python libs etc.):

```
## checkout repository ##
cd ~
git clone https://github.com/owlRobotics-GmbH/owlRobotPlatform
##  (optionally, as developer):  git clone git@github.com:owlRobotics-GmbH/owlRobotPlatform

## run bash script ##
cd owlRobotPlatform/python
sudo ./run_ble_server.sh
```

3. Start Dabble App, connect with 'owlRobot'and choose button 'Gamedpad' to steer robot (you can switch between digital mode, joystick mode and accelerometer mode)
![Screenshot from 2024-04-10 18-26-50](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/3485eaab-0ced-49aa-adff-f4493f62f156)

4. Verify that the Python script receives commands via Bluetooth when pressing buttons (up, down, left, right) in the Dabble App. Also verify (using 'candump can0') that the Python script triggers CAN packets on the CAN bus:
![Screenshot from 2024-04-10 19-16-11](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/6996b1b3-0524-40ae-a002-4195df0f0372)

The extra buttons have the following actions:
```
triangle: high motor speed
cross: low motor speed
circle: toggle tool motor speed
rectangle: shutdown Raspberry Pi5/OrangePi5Pro
select: camera-based follow-me mode on/off
start: toggle between sideways motion and rotation (only Mecanum platform)
```

5. In order to install/uninstall the demo Python script as a Linux (autostart) service:

```
sudo ./service.sh

1) Start ble_server service	    3) Start ble_server service		5) List services
2) Stop ble_server service	    4) Show ble_server service console	6) Quit
Please enter your choice: 
```

and choose point 1   ('Start ble_server service') to start the service or point 2 ('Stop ble_server service') to stop the service. 


## ROS for owlPlatform <a name="sunray_owl"></a>

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






