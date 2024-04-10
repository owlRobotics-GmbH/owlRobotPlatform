#!/usr/bin/bash 

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
    pip3 install --break-system-packages bumble
    pip3 install --break-system-packages python-can
fi

#apt install bluetooth pi-bluetooth bluez blueman

echo "setup CAN interface..."
ip link set can0 up type can bitrate 1000000

echo "setup bluetooth interface..."
#systemctl stop hciuart
#systemctl disable hciuart

hciconfig hci0 down


echo "starting python app..."
python3 ble_server.py

