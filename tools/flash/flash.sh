#!/bin/bash

# script to flash 'owlController firmware' onto  RP2040 (Raspberry Pico)  


echo "This script will flash 'owlController firmware' onto  RP2040 (Raspberry Pico)"
echo 
echo "1. Disconnect owlController's RP2040 USB"
echo "2. Press and hold button 'Boot Set' on owlController PCB"
echo "3. Press button 'Reset RP2040' on owlController PCB" 
echo "4. Release button 'Boot Set'"
echo "5. Connect owlController's RP2040 USB to owlController's RaspberryPI5"
echo 
read -p "Press ENTER to continue..."

# allow user to access /dev/ttyACM0
sudo usermod -a -G dialout $USER

./picoFlashTool_Linux ttyACM0


