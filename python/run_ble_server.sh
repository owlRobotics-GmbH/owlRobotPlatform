#!/usr/bin/bash 

python -c "import bumble"
#echo $?
if [[ "$?" -eq 1 ]]; then 
    echo "installing python packages..."    
    pip install --break-system-packages bumble
    pip install --break-system-packages python-can
fi

#apt install bluetooth pi-bluetooth bluez blueman

echo "setup CAN interface..."
ip link set can0 up type can bitrate 1000000

echo "setup bluetooth interface..."
#systemctl stop hciuart
#systemctl disable hciuart

hciconfig hci0 down


echo "starting python app..."
python ble_server.py

