#!/bin/bash

# script to flash 'owlController firmware' onto  RP2040 (Raspberry Pico)  

# 1. connect owlController's RP2040 USB to owlController's RaspberryPI5
# 2. press button 'Reset RP2040' on owlController PCB 
# 3. run this script :  ./flash.sh


# example output:
#   pi@testpi5:~/owlRobotPlatform/tools/flash $ ./flash.sh 
#   waiting for pico to mount
#   deleted mass storage relics (if existing): /
#   resetting pico
#   .
#   .
#   .
#   copy flash-file to pico...
#   './owlController_RP2040_00.ino.uf2' -> '/media/pi/RPI-RP2/owlController_RP2040_00.ino.uf2'
#   .
#   flash successful - black magic happened ~\(o.O~\)
#   pi@testpi5:~/owlRobotPlatform/tools/flash $



echo "This script will flash 'owlController firmware' onto  RP2040 (Raspberry Pico)"
echo "1. connect owlController's RP2040 USB to owlController's RaspberryPI5"
echo "2. press button 'Reset RP2040' on owlController PCB"

read -p "Press ENTER to continue..."

# allow user to access /dev/ttyACM0
sudo usermod -a -G dialout $USER

./picoFlashTool_Linux ttyACM0


