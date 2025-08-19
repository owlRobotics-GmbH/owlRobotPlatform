#!/bin/bash

# OrangePi5PRO script:
# - installs mDNS, VNC
# - changes to German keyboard layout
# - disables terminal/desktop autologin so power button allows to directly shutdown Linux
# - adds GPIO command to rc.local   for owlRobotics hardware
# - activates MIPI cam driver
# - activates I2C driver
# - activates CAN-SPI driver for owlRobotics hardware

# assumptions:
#  Ubuntu Jammy Image:   https://drive.google.com/file/d/1CrvjhITZV2vE1qJ_pcYZjjQi0JqQDwxP/view?usp=drive_link


RED='\033[0;31m'
NC='\033[0m' # No Color


# === Step 1: Detect Orange Pi 5 Pro ===
if [ -f /etc/orangepi-release ]; then
    source /etc/orangepi-release
else
    echo -e "${RED}Error: /etc/orangepi-release not found.${NC}"
    exit 1
fi

if [[ "$BOARD" != "orangepi5pro" ]]; then
    echo -e "${RED}Error: This script is only for Orange Pi 5 Pro. Detected: $BOARD${NC}"
    exit 1
fi

echo -e "${RED}[OK] Orange Pi 5 Pro detected.${NC}"


# installing missing packages...
echo -e "${RED}==========installing missing packages==========${NC}"
sudo apt update
sudo apt-get install libavcodec58 subversion btop 

# ======  disabling terminal auto-login =====================================================
echo -e "${RED}==========disabling terminal/desktop auto-login==========${NC}"
sudo auto_login_cli.sh -d
# sudo auto_login_cli.sh orangepi
# sudo auto_login_cli.sh root
sudo disable_desktop_autologin.sh
# sudo desktop_login.sh root
# sudo desktop_login.sh orangepi

# ======  installing VScode =================================================================
#printf "${RED}installing VScode...{NC}\n"
#sudo add-apt-repository "deb [arch=arm64] https://packages.microsoft.com/repos/vscode stable main"
#sudo apt install code

# ======  change XFCE power button handling (to direct shutdown, no user prompt)

if pgrep -x xfce4-session >/dev/null || pgrep -x xfce4-panel >/dev/null; then
  echo -e "${RED}XFCE detected ${NC}"

else
  echo -e  "${RED}No XFCE detected ${NC}"
fi

# =======  installing x11vnc =================================================================
# vncpasswd
echo -e "${RED}==========Installing x11vnc==========${NC}"
mkdir -p ~/.vnc
vncpasswd -f <<< "orangepi" > ~/.vnc/passwd

# create VNC xstartup-file
cat > ~/.vnc/xstartup << 'EOF'
#!/bin/sh
xrdb $HOME/.Xresources
startxfce4 &
EOF

chmod +x ~/.vnc/xstartup

sudo apt install x11vnc
sudo systemctl enable x11vnc
sudo systemctl enable x11vnc.service


# ============  activate Avahi ( mDNS) so you can resolve 'orangepi5pro.local' =============== 
echo -e "${RED}==========Installing Avahi mDNS=========${NC}"
sudo apt-get install avahi-daemon libnss-mdns 
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
#sudo hostnamectl set-hostname orangepi5pro


# ======== change keyboard layout ==============================================================
echo -e "${RED}==========Changing keyboard layout==========${NC}"
CURRENT=$(localectl status | grep "Layout" | awk '{print $3}')

if [ "$CURRENT" = "de" ]; then
    echo "keyboard layout already German"
else 
    echo "changing keyboard layout to German..."
    sudo sed -i 's/XKBLAYOUT=".*"/XKBLAYOUT="de"/' /etc/default/keyboard
    sudo dpkg-reconfigure -f noninteractive keyboard-configuration
    sudo service keyboard-setup restart
    setxkbmap de
fi

# === owlRobotics PCB:  Ensure gpio mode 6 down is in /etc/rc.local ===
echo -e "${RED}==========Installing GPIO mode 6 down setting==========${NC}"

RC_LOCAL="/etc/rc.local"
GPIO_CMD="gpio mode 6 down"
if grep -q "$GPIO_CMD" "$RC_LOCAL"; then
    echo "[OK] GPIO command already in $RC_LOCAL."
else
    echo "[INFO] Adding GPIO command to $RC_LOCAL."
    sudo sed -i "/^exit 0/i $GPIO_CMD" "$RC_LOCAL"
fi

# ============= activate I2C ===============================================================
# i2c1-m4
echo -e "${RED}==========Activating I2C==========${NC}"

ENV_FILE="/boot/orangepiEnv.txt"
OVERLAY_NAME="i2c1-m4"

if grep -q "^overlays=" "$ENV_FILE"; then
    if grep -q "$OVERLAY_NAME" "$ENV_FILE"; then
        echo "[OK] Overlay $OVERLAY_NAME already present in orangepiEnv.txt."
    else
        echo "[INFO] Adding overlay $OVERLAY_NAME to existing overlays= line."
        sudo sed -i "s/^overlays=\(.*\)/overlays=\1 $OVERLAY_NAME/" "$ENV_FILE"
    fi
else
    echo "[INFO] Adding new overlays= line."
    echo "overlays=$OVERLAY_NAME" | sudo tee -a "$ENV_FILE" > /dev/null
fi

# ============= activate MIPI cam ===============================================================
# opti5pro-cam2
echo -e  "${RED}==========Activating MIPI cam==========${NC}"

ENV_FILE="/boot/orangepiEnv.txt"
OVERLAY_NAME="opti5pro-cam2"

if grep -q "^overlays=" "$ENV_FILE"; then
    if grep -q "$OVERLAY_NAME" "$ENV_FILE"; then
        echo "[OK] Overlay $OVERLAY_NAME already present in orangepiEnv.txt."
    else
        echo "[INFO] Adding overlay $OVERLAY_NAME to existing overlays= line."
        sudo sed -i "s/^overlays=\(.*\)/overlays=\1 $OVERLAY_NAME/" "$ENV_FILE"
    fi
else
    echo "[INFO] Adding new overlays= line."
    echo "overlays=$OVERLAY_NAME" | sudo tee -a "$ENV_FILE" > /dev/null
fi


# ================= install CAN driver =====================================================
# spi0-m2-cs0-mcp2515-16mhz
echo -e "${RED}==========Installing CAN driver==========${NC}"


# === Step 1: Copy DTS file to /boot ===
DTS_FILE="res/rk3588-spi0-m2-cs0-mcp2515-16mhz.dts"
if [ ! -f "$DTS_FILE" ]; then
    echo "Error: $DTS_FILE not found!"
    exit 1
fi

sudo cp "$DTS_FILE" /boot
echo "[OK] DTS file copied to /boot."

# === Step 2: Install kernel headers ===
echo "[INFO] Installing kernel headers..."
sudo apt-get update
sudo apt-get install -y linux-headers-generic

# === Step 3: Change to /boot directory ===
cd /boot || { echo "Error: Cannot change directory to /boot."; exit 1; }

# === Step 4: Compile DTS to DTSO ===
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

# === Step 5: Compile DTSO to DTBO ===
DTB_OUTPUT_DIR="/boot/dtb/rockchip/overlay"
sudo mkdir -p "$DTB_OUTPUT_DIR"

echo "[INFO] Running dtc to compile DTBO..."
sudo dtc -@ -I dts -O dtb -o "${DTB_OUTPUT_DIR}/${DTS_BASENAME}.dtbo" "${DTS_BASENAME}.dtso"
if [ $? -ne 0 ]; then
    echo "Error: dtc failed to compile ${DTS_BASENAME}.dtso"
    exit 1
fi
echo "[OK] DTSO compiled to DTBO."

# === Step 6: Activate DTBO in orangepiEnv.txt ===
ENV_FILE="/boot/orangepiEnv.txt"
OVERLAY_NAME="spi0-m2-cs0-mcp2515-16mhz"

if grep -q "^overlays=" "$ENV_FILE"; then
    if grep -q "$OVERLAY_NAME" "$ENV_FILE"; then
        echo "[OK] Overlay $OVERLAY_NAME already present in orangepiEnv.txt."
    else
        echo "[INFO] Adding overlay $OVERLAY_NAME to existing overlays= line."
        sudo sed -i "s/^overlays=\(.*\)/overlays=\1 $OVERLAY_NAME/" "$ENV_FILE"
    fi
else
    echo "[INFO] Adding new overlays= line."
    echo "overlays=$OVERLAY_NAME" | sudo tee -a "$ENV_FILE" > /dev/null
fi

# =====================================================================

echo -e "${RED}[DONE] Configuration script completed successfully.  ${NC}"

echo "next steps:"
echo "  nmtui  (configure WiFi)"
echo "  orangepi-config  (misc)"


