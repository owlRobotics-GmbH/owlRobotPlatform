# owlRobotPlatform
owlRobotics robot platform interface code and examples

![owlplatform](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/f4a7ead1-3a4a-428f-946e-b021bfd83857)


## Python interface demo

1. Install Dabble App:  

* [Android](https://play.google.com/store/apps/details?id=io.dabbleapp)
* [iOS](https://apps.apple.com/ch/app/dabble-bluetooth-controller/id1472734455)

2. Run in Raspberry PI terminal:

```
## checkout repository ##
cd ~
git clone https://github.com/owlRobotics-GmbH/owlRobotPlatform

## run bash script ##
cd owlRobotPlatform/python
sudo ./run_ble_server.sh
```

3. Start Dabble App, connect with 'owlRobot'and choose button 'Gamedpad' to steer robot (you can switch between digital mode, joystick mode and accelerometer mode)
![Screenshot from 2024-04-10 18-26-50](https://github.com/owlRobotics-GmbH/owlRobotPlatform/assets/11735886/3485eaab-0ced-49aa-adff-f4493f62f156)


## ROS interface
...
