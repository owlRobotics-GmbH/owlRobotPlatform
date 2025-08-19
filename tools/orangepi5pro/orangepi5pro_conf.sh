#!/bin/bash

# OrangePi5PRO script:
# 1. compiles and installs CAN-SPI driver for owlRobotics hardware
# 2. adds GPIO command to rc.local   for owlRobotics hardware


# === Step 1: Detect Orange Pi 5 Pro ===
if [ -f /etc/orangepi-release ]; then
    source /etc/orangepi-release
else
    echo "Error: /etc/orangepi-release not found."
    exit 1
fi

if [[ "$BOARD" != "orangepi5pro" ]]; then
    echo "Error: This script is only for Orange Pi 5 Pro. Detected: $BOARD"
    exit 1
fi

echo "[OK] Orange Pi 5 Pro detected."

# installing missing packages...
sudo apt update
sudo apt-get install libavcodec58 avahi-daemon libnss-mdns  

# activate Avahi ( mDNS) so you can resolve 'orangepi5pro.local' 
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
#sudo hostnamectl set-hostname orangepi5pro


# === Step 2: Ensure gpio mode 6 down is in /etc/rc.local ===
RC_LOCAL="/etc/rc.local"
GPIO_CMD="gpio mode 6 down"
if grep -q "$GPIO_CMD" "$RC_LOCAL"; then
    echo "[OK] GPIO command already in $RC_LOCAL."
else
    echo "[INFO] Adding GPIO command to $RC_LOCAL."
    sudo sed -i "/^exit 0/i $GPIO_CMD" "$RC_LOCAL"
fi

# === Step 3: Copy DTS file to /boot ===
DTS_FILE="res/rk3588-spi0-m2-cs0-mcp2515-16mhz.dts"
if [ ! -f "$DTS_FILE" ]; then
    echo "Error: $DTS_FILE not found!"
    exit 1
fi

sudo cp "$DTS_FILE" /boot
echo "[OK] DTS file copied to /boot."

# === Step 4: Install kernel headers ===
echo "[INFO] Installing kernel headers..."
sudo apt-get update
sudo apt-get install -y linux-headers-generic

# === Step 5: Change to /boot directory ===
cd /boot || { echo "Error: Cannot change directory to /boot."; exit 1; }

# === Step 6: Compile DTS to DTSO ===
DTS_BASENAME="rk3588-spi0-m2-cs0-mcp2515-16mhz"
KERNEL_HEADERS=$(find /usr/src -name "linux-headers-*" | grep "5.15.0" | head -n 1)
INCLUDE_PATH="$KERNEL_HEADERS/include"

if [ ! -d "$INCLUDE_PATH" ]; then
    echo "Error: Kernel headers not found at $INCLUDE_PATH."
    exit 1
fi

echo "[INFO] Running cpp to preprocess DTS..."
sudo cpp -nostdinc -I "$INCLUDE_PATH" -undef -x assembler-with-cpp "${DTS_BASENAME}.dts" | sudo tee "${DTS_BASENAME}.dtso" > /dev/null
if [ $? -ne 0 ]; then
    echo "Error: cpp failed to compile ${DTS_BASENAME}.dts"
    exit 1
fi
echo "[OK] DTS compiled to DTSO."

# === Step 7: Compile DTSO to DTBO ===
DTB_OUTPUT_DIR="/boot/dtb/rockchip/overlay"
sudo mkdir -p "$DTB_OUTPUT_DIR"

echo "[INFO] Running dtc to compile DTBO..."
sudo dtc -@ -I dts -O dtb -o "${DTB_OUTPUT_DIR}/${DTS_BASENAME}.dtbo" "${DTS_BASENAME}.dtso"
if [ $? -ne 0 ]; then
    echo "Error: dtc failed to compile ${DTS_BASENAME}.dtso"
    exit 1
fi
echo "[OK] DTSO compiled to DTBO."

# === Step 8: Activate DTBO in orangepiEnv.txt ===
ENV_FILE="/boot/orangepiEnv.txt"
OVERLAY_NAME="spi0-m2-cs0-mcp2515-16mhz"

if grep -q "^overlays=" "$ENV_FILE"; then
    if grep -q "$OVERLAY_NAME" "$ENV_FILE"; then
        echo "[OK] Overlay already present in orangepiEnv.txt."
    else
        echo "[INFO] Adding overlay to existing overlays= line."
        sudo sed -i "s/^overlays=\(.*\)/overlays=\1 $OVERLAY_NAME/" "$ENV_FILE"
    fi
else
    echo "[INFO] Adding new overlays= line."
    echo "overlays=$OVERLAY_NAME" | sudo tee -a "$ENV_FILE" > /dev/null
fi

echo "[DONE] Configuration script completed successfully."

