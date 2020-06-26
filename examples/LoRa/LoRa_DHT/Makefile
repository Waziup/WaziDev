include ../../../WaziDev.mk

ARDUINO_LIBS     += DHT SX1272 AES-128_V10

include /usr/share/arduino/Arduino.mk

monitor:
	stty -F $(MONITOR_PORT) raw $(MONITOR_BAUDRATE)
	cat $(MONITOR_PORT)
