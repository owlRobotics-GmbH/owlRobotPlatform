#!/bin/bash

# script to flash 'owlController firmware' onto  RP2040 (Raspberry Pico)  


echo "This script will flash 'owlController firmware' onto  RP2040 (Raspberry Pico)"
echo 
echo "NOTE: Skip the following step - Only use below steps to manually activate the RP2040 bootloader."
echo "1. Disconnect owlController's RP2040 USB"
echo "2. Press and hold button 'Boot Set' on owlController PCB"
echo "3. Press button 'Reset RP2040' on owlController PCB" 
echo "4. Release button 'Boot Set'"
echo "5. Connect owlController's RP2040 USB to owlController's RaspberryPI5"
echo 
read -p "Press ENTER to flash RP2040..."

# allow user to access /dev/ttyACM0
sudo usermod -a -G dialout $USER

./picoFlashTool_Linux ttyACM0


