# Esempio di codice MicroPython per ESP32

import machine
import time

# Configurazione pin per LED (esempio pin GPIO2)
led_pin = machine.Pin(2, machine.Pin.OUT)

def main():
    while True:
        led_pin.value(1)  # LED acceso
        time.sleep(1)
        led_pin.value(0)  # LED spento
        time.sleep(1)

if __name__ == "__main__":
    main()
