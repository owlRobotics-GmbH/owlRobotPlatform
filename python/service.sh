#!/bin/bash

echo "EUID=$EUID"
echo "PWD=$PWD"

CMD=""


function start_ble_server_service() {
  # enable ble_server service
  echo "starting ble_server service..."
  cp /home/orangepi/owlRobotPlatform/python/misc/ble_server.service /etc/systemd/system/ble_server.service
  chmod 644 /etc/systemd/system/ble_server.service
  systemctl daemon-reload
  systemctl enable ble_server
  systemctl start ble_server
  systemctl --no-pager status ble_server
  echo "ble_server service started!"
}

function stop_ble_server_service() {
  # disable ble_server service
  echo "stopping ble_server service..."
  systemctl stop ble_server
  systemctl disable ble_server
  echo "ble_server service stopped!"
}

function list(){
  echo "listing services..."
  service --status-all
}


function show_ble_server_console(){
    journalctl -f -u ble_server
}


if [ "$EUID" -ne 0 ]
  then echo "Please run as root ('sudo ./service.sh')"
  exit
fi



# show menu
PS3='Please enter your choice: '
options=("Start ble_server service" 
  "Stop ble_server service" 
  "Show ble_server service console"  
  "List services"   
  "Quit")
select opt in "${options[@]}"
do
    case $opt in
        "Start ble_server service")
            start_ble_server_service
            break
            ;;
        "Stop ble_server service")
            stop_ble_server_service
            break
            ;;
        "Show ble_server service console")
            show_ble_server_console
            break
            ;;                        
        "List services")
            list
            break
            ;;                                        
        "Quit")
            break
            ;;
        *) echo "invalid option $REPLY";;
    esac
done


