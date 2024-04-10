#!/usr/bin/env python


# Dabble App joystick interface (Bluetooth Low Energy Server) 

# pip install --break-system-packages  bumble

# # https://github.com/google/bumble


import asyncio
import sys
import os
import logging
import time
import threading


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



class Dabble():
    def __init__(self, bluetooth_transport = 'hci-socket:0'):
        self.button = 'released'
        self.proc = threading.Thread(target=self.startAsync, args=(bluetooth_transport,), daemon=True)
        self.proc.start()
        
        #asyncio.run(self.start(bluetooth_transport))    

    def startAsync(self, bluetooth_transport):
        print('startAsync')
        asyncio.run(self.start(bluetooth_transport))


    async def start(self, bluetooth_transport = 'hci-socket:0'):
        print('starting dabble app interface...', bluetooth_transport)

        print('<<< connecting to HCI...')
        async with await open_transport_or_link(bluetooth_transport) as (self.hci_source, self.hci_sink):
            print('<<< connected')

            # Create a device to manage the host
            self.device = Device.from_config_file_with_hci('device1.json', self.hci_source, self.hci_sink)
            self.device.listener = Listener(self.device)

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
                        CharacteristicValue(read=self.my_custom_read, write=self.my_custom_write),
                    ),
                    Characteristic(
                        '6E400003-B5A3-F393-E0A9-E50E24DCCA9E',
                        Characteristic.Properties.READ | Characteristic.Properties.NOTIFY,
                        Characteristic.READABLE,
                        'hello',
                    ),
                ],
            )
            self.device.add_services([device_info_service, custom_service1])

            # Debug print
            #for attribute in device.gatt_server.attributes:
            #    print(attribute)

            # Get things going
            await self.device.power_on()

            # Connect to a peer
            #if len(sys.argv) > 3:
            #    target_address = sys.argv[3]
            #    print(f'=== Connecting to {target_address}...')
            #    await device.connect(target_address)
            #else:
            await self.device.start_advertising(auto_restart=False)

            await self.hci_source.wait_for_termination()
            print('dabble: hci source terminated - goodbye')


    def my_custom_read(self, connection):
        print('----- READ from', connection)
        return bytes(f'Hello {connection}', 'ascii')


    def my_custom_write(self, connection, value):
        #print(f'----- WRITE from {connection}: {value}')
        #print(type(value))
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
            self.button = 'up'
            print('up')
        elif value[6] == 0x02:
            self.button = 'down'
            print('down')            
        elif value[6] == 0x04:
            self.button = 'left'
            print('left')
        elif value[6] == 0x08:
            self.button = 'right'
            print('right')
        else:
            self.button = 'released'
            print('released')


if __name__ == "__main__":
    #logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'DEBUG').upper())    
    logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'WARNING').upper())
    
    app = Dabble('hci-socket:0')
    #app = Dabble('usb:0')
    
    while (True):
        time.sleep(1.0)
        print('.')
    

