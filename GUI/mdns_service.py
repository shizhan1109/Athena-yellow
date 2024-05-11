from zeroconf import ServiceBrowser, Zeroconf
from PySide6.QtCore import Qt, QTimer
import time

class MyListener:
    def __init__(self):
        self.devices = []

    def remove_service(self, zeroconf, type, name):
        print(f"Service {name} removed")

    def add_service(self, zeroconf, type, name):
        info = zeroconf.get_service_info(type, name)
        if info:
            addresses = ", ".join(str(addr) for addr in info.parsed_addresses())
            # print(f"Service {name} added, IP addresses: {addresses}, Port: {info.port}")
            name = name.split(".")[0]
            self.devices.append({"name": name, "addresses": addresses, "port": info.port, "connection": None})

    def get_devices(self):
        return self.devices
    
    def update_service(self, zeroconf, type, name):
        print(f"Service {name} updated")

def found_devices():
    zeroconf = Zeroconf()
    listener = MyListener()
    browser = ServiceBrowser(zeroconf, "_ayServer._tcp.local.", listener)
    time.sleep(5)
    zeroconf.close()
    # sort devices by ip
    devices = listener.get_devices().copy()
    devices = sorted(devices, key=lambda x: x['name'])
    return devices

def found_devices_str():
    devices = found_devices()
    devices_str_list = ['Node A', 'Node B', 'Node C', 'Node D', 'Node E', 'Node F', 'Node G', 'Node H', 'Node I']
    for i, device in enumerate(devices):
        if i >= len(devices_str_list):
            break

        name = device['name'].replace('-', '\n')
        ip = device['addresses']
        port = device['port']
        device_str = f"{name}\n\n{ip}:\n{port}"
        devices_str_list[i] = device_str
    return devices, devices_str_list

if __name__ == "__main__":
    devices = found_devices_str()
    print(devices)
