# owlRobotPlatform
owlRobotics robot platform interface code and examples

![owlplatform](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/f4a7ead1-3a4a-428f-946e-b021bfd83857)


## Python interface demo

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

3. Run in Raspberry PI terminal:

```
## checkout repository ##
cd ~
git clone https://github.com/owlRobotics-GmbH/owlRobotPlatform

## run bash script ##
cd owlRobotPlatform/python
sudo ./run_ble_server.sh
```

4. Start Dabble App, connect with 'owlRobot'and choose button 'Gamedpad' to steer robot (you can switch between digital mode, joystick mode and accelerometer mode)
![Screenshot from 2024-04-10 18-26-50](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/3485eaab-0ced-49aa-adff-f4493f62f156)

5. Verify that the Python script receives commands via Bluetooth when pressing buttons (up, down, left, right) in the Dabble App. Also verify (using 'candump can0') that the Python script triggers CAN packets on the CAN bus:
![Screenshot from 2024-04-10 19-16-11](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/6996b1b3-0524-40ae-a002-4195df0f0372)

6. In order to install the demo Python script as a Linux (autostart) service:

```
sudo ./service.sh

1) Start ble_server service	    3) Start ble_server service		5) List services
2) Stop ble_server service	    4) Show ble_server service console	6) Quit
Please enter your choice: 
```

and choose point 1   ('Start ble_server service')


## ROS interface
...
