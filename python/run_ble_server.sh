#!/bin/bash 


PIP_OPTIONS=""


if [ "$EUID" -ne 0 ]
  then echo "Please run as root (sudo):   sudo ./run_ble_server.sh"
  exit
fi


if grep -qi "raspberry pi" /proc/device-tree/model 2>/dev/null; then
    echo "Running on a Raspberry Pi"
    PIP_OPTIONS="--break-system-packages"  # may be required for Raspi5
else
    echo "Not a Raspberry Pi"
fi



if [ "$(which python3)" == "" ]; then
    echo "installing python3..."
    apt install python3
fi

if [ "$(which python3-pip)" == "" ]; then
    echo "installing python3-pip..."
    apt install python3-pip
fi



python3 -c "import bumble"
#echo $?
if [[ "$?" -eq 1 ]]; then 
    echo "installing python3 packages..."    
    # pip3 install $PIP_OPTIONS bumble==0.0.190
    # pip3 install $PIP_OPTIONS bumble==4.3.1
    pip3 install $PIP_OPTIONS bumble==0.0.190      # tested version: 0.0.190
    pip3 install $PIP_OPTIONS python-can==4.3.1    # tested version: 4.3.1
    pip3 install $PIP_OPTIONS opencv-python  
    pip3 install $PIP_OPTIONS opencv-contrib-python  
    pip3 install $PIP_OPTIONS psutil
fi

#apt install bluetooth pi-bluetooth bluez blueman

echo "setup CAN interface..."
echo "NOTE: you may have to edit boot config to enable CAN driver (see https://github.com/owlRobotics-GmbH/owlRobotPlatform)"
ip link set can0 up type can bitrate 1000000

echo "setup bluetooth interface..."
#systemctl stop hciuart
#systemctl disable hciuart

hciconfig hci0 down


echo "---------all processes using the CAN bus------------"
lsof | grep -i can_raw
echo "----------------------------------------------------"


echo "starting python app..."
echo "NOTE: to test Bluetooth communication only and redirect output to a log file use: sudo ./run_ble_server.sh dabble.py &> log.txt" 


if [ "$#" -eq  "0" ] 
then
  # No arguments supplied
  python3 ble_server.py
else
  # python file supplied
  python3 "$1" 
fi


