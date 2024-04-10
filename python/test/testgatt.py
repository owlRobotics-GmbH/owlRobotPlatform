# test bluetooth (gatt server) with Dabble App


# https://google.github.io/bumble/platforms/linux.html
# https://github.com/google/bumble

# Raspberry PI (HCI over UART (via a serial port)):
# 1. stop bluetooth service:   sudo systemctl stop bluetooth.service
# 2. detach hci socket:        hciconfig hci0 down
# 3. sudo python testgatt.py device1.json hci-socket:0


# PC (Bluetooth USB Dongle):
# sudo python testgatt.py device1.json usb:0



# -----------------------------------------------------------------------------
# Imports
# -----------------------------------------------------------------------------
import asyncio
import sys
import os
import logging

from bumble.utils import AsyncRunner
from bumble.device import Device, Connection
from bumble.transport import open_transport_or_link
from bumble.att import ATT_Error, ATT_INSUFFICIENT_ENCRYPTION_ERROR
from bumble.gatt import (
    Service,
    Characteristic,
    CharacteristicValue,
    Descriptor,
    GATT_CHARACTERISTIC_USER_DESCRIPTION_DESCRIPTOR,
    GATT_MANUFACTURER_NAME_STRING_CHARACTERISTIC,
    GATT_DEVICE_INFORMATION_SERVICE,
)


# -----------------------------------------------------------------------------
class Listener(Device.Listener, Connection.Listener):
    def __init__(self, device):
        self.device = device        

    def on_connection(self, connection):
        print(f'=== Connected to {connection}')
        self.connection = connection
        connection.listener = self

    def on_disconnection(self, reason):
        print(f'### Disconnected, reason={reason}')
        #self.device.disconnect(self.connection, reason)
        #AsyncRunner.spawn(self.device.set_discoverable(True))
        AsyncRunner.spawn(self.device.start_advertising(auto_restart=False))




def my_custom_read(connection):
    print('----- READ from', connection)
    return bytes(f'Hello {connection}', 'ascii')


def my_custom_write(connection, value):
    print(f'----- WRITE from {connection}: {value}')
    print(type(value))
    if not isinstance(value, bytes): return
    if len(value) != 8: return
    # up
    # b'\xff\x01\x01\x01\x02\x00\x01\x00'
    # b'\xff\x01\x01\x01\x02\x00\x00\x00'
    
    # down
    # b'\xff\x01\x01\x01\x02\x00\x02\x00'
    # b'\xff\x01\x01\x01\x02\x00\x00\x00'

    # left
    # b'\xff\x01\x01\x01\x02\x00\x04\x00'
    # b'\xff\x01\x01\x01\x02\x00\x00\x00'

    # right
    # b'\xff\x01\x01\x01\x02\x00\x08\x00'
    # b'\xff\x01\x01\x01\x02\x00\x00\x00'

    if value[6] == 0x01:
        print('up')
    elif value[6] == 0x02:
        print('down')
    elif value[6] == 0x04:
        print('left')
    elif value[6] == 0x08:
        print('right')
    else:
        print('released')


def my_custom_read_with_error(connection):
    print('----- READ from', connection, '[returning error]')
    if connection.is_encrypted:
        return bytes([123])

    raise ATT_Error(ATT_INSUFFICIENT_ENCRYPTION_ERROR)


def my_custom_write_with_error(connection, value):
    print(f'----- WRITE from {connection}: {value}', '[returning error]')
    if not connection.is_encrypted:
        raise ATT_Error(ATT_INSUFFICIENT_ENCRYPTION_ERROR)


# -----------------------------------------------------------------------------
async def main():
    if len(sys.argv) < 3:
        print(
            'Usage: run_gatt_server.py <device-config> <transport-spec> '
            '[<bluetooth-address>]'
        )
        print('example: run_gatt_server.py device1.json usb:0 E1:CA:72:48:C4:E8')
        return

    print('<<< connecting to HCI...')
    async with await open_transport_or_link(sys.argv[2]) as (hci_source, hci_sink):
        print('<<< connected')

        # Create a device to manage the host
        device = Device.from_config_file_with_hci(sys.argv[1], hci_source, hci_sink)
        device.listener = Listener(device)

        # Add a few entries to the device's GATT server
        descriptor = Descriptor(
            GATT_CHARACTERISTIC_USER_DESCRIPTION_DESCRIPTOR,
            Descriptor.READABLE,
            'My Description',
        )
        manufacturer_name_characteristic = Characteristic(
            GATT_MANUFACTURER_NAME_STRING_CHARACTERISTIC,
            Characteristic.Properties.READ,
            Characteristic.READABLE,
            'Fitbit',
            [descriptor],
        )
        device_info_service = Service(
            GATT_DEVICE_INFORMATION_SERVICE, [manufacturer_name_characteristic]
        )
        custom_service1 = Service(
            '6E400001-B5A3-F393-E0A9-E50E24DCCA9E',
            [
                Characteristic(
                    '6E400002-B5A3-F393-E0A9-E50E24DCCA9E',
                    Characteristic.Properties.READ | Characteristic.Properties.WRITE,
                    Characteristic.READABLE | Characteristic.WRITEABLE,
                    CharacteristicValue(read=my_custom_read, write=my_custom_write),
                ),
                Characteristic(
                    '6E400003-B5A3-F393-E0A9-E50E24DCCA9E',
                    Characteristic.Properties.READ | Characteristic.Properties.NOTIFY,
                    Characteristic.READABLE,
                    'hello',
                ),
            ],
        )
        device.add_services([device_info_service, custom_service1])

        # Debug print
        for attribute in device.gatt_server.attributes:
            print(attribute)

        # Get things going
        await device.power_on()

        # Connect to a peer
        if len(sys.argv) > 3:
            target_address = sys.argv[3]
            print(f'=== Connecting to {target_address}...')
            await device.connect(target_address)
        else:
            await device.start_advertising(auto_restart=False)

        await hci_source.wait_for_termination()


# -----------------------------------------------------------------------------
logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'DEBUG').upper())
asyncio.run(main())
