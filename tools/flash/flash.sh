#!/bin/bash

# script to flash 'owlController firmware' onto  RP2040 (Raspberry Pico)  

# RP2040 serial device path (if not in bootloader mode)
DEV=""

# flash file path
FLASH_FILE=""

# RP2040 mounted volume path
RPI_VOL_PATH="/media/$USER/RPI-RP2"

# RP2040 disk device
RPI_DISK=""


function replug_usb() {
	echo "replug_usb"
	for i in /sys/bus/pci/drivers/[uoex]hci_hcd/*:*; do
		[ -e "$i" ] || continue
		echo "${i##*/}" > "${i%/*}/unbind"
		echo "${i##*/}" > "${i%/*}/bind"
	done
	sleep 2.0
}


function choose_serial_device() {
  echo 
  PS3='Please choose serial device: '
  list=$(ls /dev/serial/by-id/usb-Raspberry_Pi_Pico*)
  IFS=$'\n'
  select DEV in $list 
    do test -n "$DEV" && break; 
  exit; 
  done
  echo "selected: $DEV"
  echo
}


function choose_flash_file() {
  echo 
  PS3='Please choose flash file: '
  list=$(ls *.uf2)
  IFS=$'\n'
  select FLASH_FILE in $list 
    do test -n "$FLASH_FILE" && break; 
  exit; 
  done
  echo "selected: $FLASH_FILE"
  echo
}


function wait_for_serial() {
	# wait for serial device to mount	
	echo "waiting for pico to mount ($DEV)"
	count=0
	while [ ! -e $DEV ]
	do 
		sleep 0.5
		count=$((count+1))
		echo .
		# exit script if no serial device could be found
		if [ $count -ge 30 ]
		then 
			echo No device found on serial port $1 - Aborting process.
			exit
		fi
	done
}

function delete_old_relics() {
	# delete old pico mass storage relics from file system 
	echo deleted mass storage relics \(if existing\): /
	sudo find /media/$USER/ -type d -name 'RPI-RP*' -prune -print -exec rm -rf {} \;
}

function reset_pico() {
	# reset pico over baud rate change to 1200 baud
	echo "resetting pico (manually press button 'Reset RP2040' once if it does not work)"
	sudo stty -F $DEV 1200
}


function wait_for_disk() {
	# wait until pico has mounted as mass storage device
	count=0
	while [ ! -d $RPI_VOL_PATH ]
	do 
		sleep 0.5
		echo .
		count=$((count+1))
		# exit script if pico did not mount as mass storage device
		if [ $count -ge 30 ]; then 
			echo pico did not reboot - try again!
			exit
		fi
	done
	# give pico some time to mount fully
	sleep 2.0
}

function remove_fs_dirty_bit() {
	echo "removing any filesystem dirty bit"
	RPI_DISK=`df | grep $RPI_VOL_PATH | cut -d " " -f1`
	echo "disk: $RPI_DISK"	
	if [ -z "${VAR}" ]; then		
		return
	fi
	sudo dosfsck -yw $RPI_DISK 
	#sleep 2.0
	#sudo mount -o remount, rw $RPI_VOL_PATH 
	sleep 2.0
}

function copy_file() {
	# copy flash-file to pico
	echo copy flash-file to pico...
	#sudo find -type f -name '*.uf2' -exec cp -prv {} /media/$USER/RPI-RP2 \;
	echo "$FLASH_FILE -> $RPI_VOL_PATH" 
    sudo cp $FLASH_FILE $RPI_VOL_PATH
	sudo sync

	# check if flash has been successful
	count=0
	while [ -d $RPI_VOL_PATH ]
	do 
		sleep 0.5
		echo .
		count=$((count+1))
		# exit script if flash was not successful (pico is still mounted as mass storage device)
		if [ $count -ge 30 ]; then 
			echo pico was reset but could not be flashed - please drag and drop .uf2-file manually!
			return
		fi
	done

	echo flash successful - black magic happened ~\\\(o.O~\\\)
}


echo "This script will flash 'owlController firmware' onto  RP2040 (Raspberry Pico)"
echo 
echo "NOTE: Skip the following steps - Only use below steps to manually activate the RP2040 bootloader."
echo "1. Disconnect owlController's RP2040 USB"
echo "2. Press and hold button 'Boot Set' on owlController PCB"
echo "3. Press button 'Reset RP2040' on owlController PCB" 
echo "4. Release button 'Boot Set'"
echo "5. Connect owlController's RP2040 USB to owlController's RaspberryPI5"
echo 
read -p "Press ENTER to flash RP2040..."

# allow user to access /dev/ttyACM0
sudo usermod -a -G dialout $USER



if [ -z "$( ls -A /dev/serial/by-id/usb-Raspberry_Pi_Pico* )" ]; then
  echo "no RP2040 serial devices => assuming RP2040 already in bootloader"
else
  echo "found RP2040 serial devices => assuming RP2040 not yet in bootloader"
  #exit
  choose_serial_device
  wait_for_serial
  delete_old_relics
  reset_pico
  sleep 8.0
fi

#exit


# RP2040 is now in bootloader
#remove_fs_dirty_bit
wait_for_disk
choose_flash_file
copy_file
replug_usb







