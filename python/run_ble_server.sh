#!/bin/bash 


if [ "$EUID" -ne 0 ]
  then echo "Please run as root (sudo):   sudo ./run_ble_server.sh"
  exit
fi

if [ "$(which python3)" == "" ]; then
    echo "installing python3..."
    apt install python3
fi

python3 -c "import bumble"
#echo $?
if [[ "$?" -eq 1 ]]; then 
    echo "installing python3 packages..."    
    # pip3 install --break-system-packages bumble==0.0.190
    # pip3 install --break-system-packages bumble==4.3.1
    pip3 install --break-system-packages bumble==0.0.190      # tested version: 0.0.190
    pip3 install --break-system-packages python-can==4.3.1    # tested version: 4.3.1
    pip3 install --break-system-packages opencv-python  
    pip3 install --break-system-packages opencv-contrib-python  
    pip3 install --break-system-packages psutil
fi

#apt install bluetooth pi-bluetooth bluez blueman

echo "setup CAN interface..."
echo "NOTE: you may have to edit boot config to enable CAN driver (see https://github.com/owlRobotics-GmbH/owlRobotPlatform)"
ip link set can0 up type can bitrate 1000000

echo "setup bluetooth interface..."
#systemctl stop hciuart
#systemctl disable hciuart

hciconfig hci0 down


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


