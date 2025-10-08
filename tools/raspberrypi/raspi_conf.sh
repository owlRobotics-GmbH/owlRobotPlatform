#!/bin/bash

# Raspberry script:
# - adds GPIO command to rc.local   for owlRobotics hardware
# - activates I2C driver
# - activates CAN-SPI driver for owlRobotics hardware

# assumptions:
#  Raspian OS/Raspian OS lite


RED='\033[0;31m'
NC='\033[0m' # No Color


# === Detect Raspberry ===

if [ -f /etc/os-release ]; then
    source /etc/os-release
else
    echo -e "${RED}Error: /etc/os-release not found.${NC}"
    exit 1
fi

if [[ "$ID" != "raspbian" ]] && [[ "$ID" != "debian" ]]; then
    echo -e "${RED}Error: This script is only for Raspberry. Detected: $ID${NC}"
    exit 1
fi

echo -e "${RED}[OK] Raspberry detected.${NC}"


# installing missing packages...
echo -e "${RED}==========installing missing packages==========${NC}"
sudo apt update
sudo apt-get -y install libavcodec58 subversion btop 


# === owlRobotics PCB:  Ensure gpio mode 1 down is in /etc/rc.local ===
echo -e "${RED}==========Installing GPIO mode 1 down setting==========${NC}"

RC_LOCAL="/etc/rc.local"
GPIO_CMD="gpio mode 1 down"

# create rc.local if empty/not existing
if [ ! -s "$RC_LOCAL" ]; then
  sudo tee "$RC_LOCAL" >/dev/null <<'EOF'
#!/bin/sh -e
# /etc/rc.local
# commands to start at boot time.

exit 0
EOF
fi

if grep -q "$GPIO_CMD" "$RC_LOCAL"; then
    echo "[OK] GPIO command already in $RC_LOCAL."
else
    echo "[INFO] Adding GPIO command to $RC_LOCAL."
    sudo sed -i "/^exit 0/i $GPIO_CMD" "$RC_LOCAL"
fi

# ============= activate I2C ===============================================================
echo -e "${RED}==========Activating I2C==========${NC}"

sudo raspi-config nonint do_i2c 0
echo "Done"

# ================= install CAN driver =====================================================
# spi0-m2-cs0-mcp2515-16mhz
echo -e "${RED}==========Installing CAN driver==========${NC}"

BOOTCFG="/boot/config.txt"
[ -f /boot/firmware/config.txt ] && BOOTCFG="/boot/firmware/config.txt"
echo "Using Boot-Config: $BOOTCFG"

LINE="dtoverlay=mcp2515-can0,oscillator=16000000,interrupt=6"

if grep -qF "$LINE" "$BOOTCFG"; then
    echo "CAN driver already existing. No changes."
else
    echo "Adding to Boot-Config: $LINE"
    echo "$LINE" | sudo tee -a "$BOOTCFG" > /dev/null
    echo "installed CAN driver"
fi

# ============  activate Avahi ( mDNS) so you can resolve 'raspberrypi.local' =============== 
echo -e "${RED}==========Installing Avahi mDNS=========${NC}"
sudo apt-get -y install avahi-daemon libnss-mdns 
sudo cp res/http.service /etc/avahi/services/http.service 
sudo chmod 644 /etc/avahi/services/http.service
sudo systemctl enable avahi-daemon
sudo systemctl start avahi-daemon
#sudo hostnamectl set-hostname raspberrypi

# =====================================================================

echo -e "${RED}[DONE] Configuration script completed successfully. Reboot to activate added Linux overlay drivers. ${NC}"

echo "next steps:"
echo "  nmtui  (configure WiFi)"
echo "  raspi-config  (misc)"

