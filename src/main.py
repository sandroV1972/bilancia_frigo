import network
import time
import credentials

# Wi-Fi credentials
SSID = ''
PASSWORD = ''

def connect_to_wifi():
    station = network.WLAN(network.STA_IF)
    if not station.isconnected():
        print("Connecting to WiFi...")
        station.active(True)
        station.connect(SSID, PASSWORD)
        while not station.isconnected():
            time.sleep(0.5)
    print("Connected! Network config:", station.ifconfig())

# Run the connection function
connect_to_wifi()
