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
import math

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


connected = False 
currConnection = None


APP_NAME = 'dabble' 
#APP_NAME = 'sunray' 


if APP_NAME == 'dabble':
    # Dabble App 
    UUID_SERVICE = '6E400001-B5A3-F393-E0A9-E50E24DCCA9E'  # bluetooth GATT service
    UUID_CHAR_TX = '6E400002-B5A3-F393-E0A9-E50E24DCCA9E'  # another device can send to this 
    UUID_CHAR_RX = '6E400003-B5A3-F393-E0A9-E50E24DCCA9E'  # another device can receive from this 
elif APP_NAME == 'sunray':
    # Sunray App
    UUID_SERVICE     = '0000ffe0-0000-1000-8000-00805f9b34fb'  # bluetooth GATT service
    UUID_CHAR_TX_RX  = '0000ffe1-0000-1000-8000-00805f9b34fb'  # another device can send to this/receive from this 


# -----------------------------------------------------------------------------
class Listener(Device.Listener, Connection.Listener):
    def __init__(self, device):
        self.device = device

    def on_connection(self, connection):
        global connected, currConnection
        currConnection = connection
        connected = True
        print(f'=== Connected to {connection}')
        self.connection = connection        
        connection.listener = self


    def on_disconnection(self, reason):
        global connected
        connected = False
        print(f'### Disconnected, reason={reason}')
        #self.device.disconnect(self.connection, reason)
        #AsyncRunner.spawn(self.device.set_discoverable(True))
        AsyncRunner.spawn(self.device.start_advertising(auto_restart=False))

    def on_connection_att_mtu_update(self):
        print(f'### connection att mtu update: {currConnection.att_mtu}')
        

    def on_characteristic_subscription(
        self, connection, characteristic, notify_enabled, indicate_enabled
    ):
        print(
            f'$$$ Characteristic subscription for handle {characteristic.handle} '
            f'from {connection}: '
            f'notify {"enabled" if notify_enabled else "disabled"}, '
            f'indicate {"enabled" if indicate_enabled else "disabled"}'
        )
        # TODO: Dabble App may expect some initial notify data: 
        #self.device.characteristicRead.value = bytes([0xFF, 0x00, 0x01, 0x00, 0x00])
        #AsyncRunner.spawn(self.device.notify_subscribers(self.device.characteristicRead))

        


class Dabble():
    def __init__(self, bluetooth_transport = 'hci-socket:0'):
        self.analogMode = False  # analog joystick mode?
        self.joystickButton = 'released'
        self.extraButton = 'released'   
        self.angle = 0   # angle/radius of analog joystick
        self.radius = 0 
        self.x_value = 0  # x/y-value of analog joystick
        self.y_value = 0

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
            print('APP_NAME', APP_NAME)
            if APP_NAME == 'dabble':
                characteristicWrite = Characteristic(
                            UUID_CHAR_TX ,
                            Characteristic.Properties.WRITE,
                            Characteristic.WRITEABLE,
                            CharacteristicValue(write=self.my_custom_write),
                        )
                
                self.device.characteristicRead = Characteristic(
                            UUID_CHAR_RX,
                            Characteristic.Properties.NOTIFY,
                            Characteristic.READABLE,
                            CharacteristicValue(read=self.my_custom_read),
                    )
                custom_service1 = Service(
                    UUID_SERVICE,
                    [
                        characteristicWrite,
                        self.device.characteristicRead
                    ],
                )
            elif APP_NAME == 'sunray':
                self.device.characteristicRead = Characteristic(
                            UUID_CHAR_TX_RX ,
                            Characteristic.Properties.WRITE | Characteristic.Properties.READ | Characteristic.Properties.NOTIFY,
                            Characteristic.WRITEABLE | Characteristic.READABLE ,
                            CharacteristicValue(read=self.my_custom_read, write=self.my_custom_write),
                        )
                custom_service1 = Service(
                    UUID_SERVICE,
                    [
                        self.device.characteristicRead
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

            while True:
                #print('sleep')
                await asyncio.sleep(1.0)
                #for val in bytes([0xFF, 0x00, 0x01, 0x00, 0x00]):
                #    await asyncio.sleep(0.02)
                #    self.device.characteristicRead.value = [val] 
                #    await self.device.notify_subscribers(self.device.characteristicRead)
                #await self.device.indicate_subscribers(self.device.characteristicRead)
                
                
            #await self.hci_source.wait_for_termination()


    def my_custom_read(self, connection):
        print('----- READ from', connection)
        return bytes(f'Hello {connection}', 'ascii')


    def my_custom_write(self, connection, value):
        print(f'----- WRITE from {connection}: {value}')
        print(type(value))
        if not isinstance(value, bytes): return
        if len(value) != 8: return
        if value[5] == 0x01:
            self.extraButton = 'start'
            print('extra: start')
        elif value[5] == 0x02:
            self.extraButton = 'select'
            print('extra: select')
        elif value[5] == 0x10:
            self.extraButton = 'cross'
            print('extra: cross')
        elif value[5] == 0x08:
            self.extraButton = 'circle'
            print('extra: circle')
        elif value[5] == 0x04:
            self.extraButton = 'triangle'
            print('extra: triangle')
        elif value[5] == 0x20:
            self.extraButton = 'rectangle'
            print('extra: rectangle')
        elif value[5] == 0x00:
            self.extraButton = 'released'
            print('extra: released')

        if value[2] == 0x02 or value[2] == 0x03:  # analog joystick / acceleration sensor
            self.analogMode = True
            val = value[6]             
            self.angle=((val >> 3)*15);
            self.radius= val&0x07;
            self.x_value= float(self.radius*(float(math.cos(float(self.angle*math.pi/180.0))))) / 6.0;
            self.y_value= float(self.radius*(float(math.sin(float(self.angle*math.pi/180.0))))) / 6.0;
            print('x', round(self.x_value,2), 'y', round(self.y_value,2))            
        elif value[2] == 0x01:  # digital joystick
            self.analogMode = False
            if value[6] == 0x01:
                self.joystickButton = 'up'
                print('joy: up')
            elif value[6] == 0x02:
                self.joystickButton = 'down'
                print('joy: down')            
            elif value[6] == 0x04:
                self.joystickButton = 'left'
                print('joy: left')
            elif value[6] == 0x08:
                self.joystickButton = 'right'
                print('joy: right')
            elif value[6] == 0x00:
                self.joystickButton = 'released'
                print('joy: released')


if __name__ == "__main__":
    #logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'DEBUG').upper())    
    logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'WARNING').upper())
    
    #app = Dabble('hci-socket:0')
    app = Dabble('usb:0')
    
    while (True):
        time.sleep(1.0)
        #print('.')
    

