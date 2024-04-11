# BLE GATT client


# python test/testgatt.py device1.json usb:0 24:6F:28:25:D8:42/P


#=== Services discovered
#Service(handle=0x0001, uuid=UUID-16:1801 (Generic Attribute))
#  Characteristic(handle=0x0003, uuid=UUID-16:2A05 (Service Changed), INDICATE)
#    Descriptor(handle=0x0004, type=UUID-16:2902 (Client Characteristic Configuration))
#Service(handle=0x0014, uuid=UUID-16:1800 (Generic Access))
#  Characteristic(handle=0x0016, uuid=UUID-16:2A00 (Device Name), READ)
#  Characteristic(handle=0x0018, uuid=UUID-16:2A01 (Appearance), READ)
#  Characteristic(handle=0x001A, uuid=UUID-16:2AA6 (Central Address Resolution), READ)
#Service(handle=0x0028, uuid=6E400001-B5A3-F393-E0A9-E50E24DCCA9E)
#  Characteristic(handle=0x002A, uuid=6E400003-B5A3-F393-E0A9-E50E24DCCA9E, NOTIFY)
#    Descriptor(handle=0x002B, type=UUID-16:2902 (Client Characteristic Configuration))
#  Characteristic(handle=0x002D, uuid=6E400002-B5A3-F393-E0A9-E50E24DCCA9E, WRITE)


#=== Discovering attributes
#Attribute(handle=0x0001, type=UUID-16:2800 (Primary Service))
#Attribute(handle=0x0002, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x0003, type=UUID-16:2A05 (Service Changed))
#Attribute(handle=0x0004, type=UUID-16:2902 (Client Characteristic Configuration))
#Attribute(handle=0x0014, type=UUID-16:2800 (Primary Service))
#Attribute(handle=0x0015, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x0016, type=UUID-16:2A00 (Device Name))
#Attribute(handle=0x0017, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x0018, type=UUID-16:2A01 (Appearance))
#Attribute(handle=0x0019, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x001A, type=UUID-16:2AA6 (Central Address Resolution))
#Attribute(handle=0x0028, type=UUID-16:2800 (Primary Service))
#Attribute(handle=0x0029, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x002A, type=6E400003-B5A3-F393-E0A9-E50E24DCCA9E)
#Attribute(handle=0x002B, type=UUID-16:2902 (Client Characteristic Configuration))
#Attribute(handle=0x002C, type=UUID-16:2803 (Characteristic))
#Attribute(handle=0x002D, type=6E400002-B5A3-F393-E0A9-E50E24DCCA9E)


# -----------------------------------------------------------------------------
# Imports
# -----------------------------------------------------------------------------
import asyncio
import sys
import os
import logging
from bumble.colors import color

from bumble.core import ProtocolError
from bumble.device import Device, Peer
from bumble.gatt import show_services
from bumble.transport import open_transport_or_link
from bumble.utils import AsyncRunner


# -----------------------------------------------------------------------------
class Listener(Device.Listener):
    def __init__(self, device):
        self.device = device

    @AsyncRunner.run_in_task()
    # pylint: disable=invalid-overridden-method
    async def on_connection(self, connection):
        print(f'=== Connected to {connection}')

        # Discover all services
        print('=== Discovering services')
        peer = Peer(connection)
        await peer.discover_services()
        for service in peer.services:
            await service.discover_characteristics()
            for characteristic in service.characteristics:
                await characteristic.discover_descriptors()

        print('=== Services discovered')
        show_services(peer.services)

        # Discover all attributes
        print('=== Discovering attributes')
        attributes = await peer.discover_attributes()
        for attribute in attributes:
            print(attribute)
        print('=== Attributes discovered')

        # Read all attributes
        for attribute in attributes:
            try:
                value = await peer.read_value(attribute)
                print(color(f'0x{attribute.handle:04X} = {value.hex()}', 'green'))
            except ProtocolError as error:
                print(color(f'cannot read {attribute.handle:04X}:', 'red'), error)
            except TimeoutError:
                print(color('read timeout'))


# -----------------------------------------------------------------------------
async def main():
    if len(sys.argv) < 3:
        print(
            'Usage: run_gatt_client.py <device-config> <transport-spec> '
            '[<bluetooth-address>]'
        )
        print('example: run_gatt_client.py device1.json usb:0 E1:CA:72:48:C4:E8')
        return

    print('<<< connecting to HCI...')
    async with await open_transport_or_link(sys.argv[2]) as (hci_source, hci_sink):
        print('<<< connected')

        # Create a device to manage the host, with a custom listener
        device = Device.from_config_file_with_hci(sys.argv[1], hci_source, hci_sink)
        device.listener = Listener(device)
        await device.power_on()

        # Connect to a peer
        if len(sys.argv) > 3:
            target_address = sys.argv[3]
            print(f'=== Connecting to {target_address}...')
            await device.connect(target_address)
        else:
            await device.start_advertising()

        await asyncio.get_running_loop().create_future()


# -----------------------------------------------------------------------------
#logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'DEBUG').upper())
logging.basicConfig(level=os.environ.get('BUMBLE_LOGLEVEL', 'WARNING').upper())

asyncio.run(main())


