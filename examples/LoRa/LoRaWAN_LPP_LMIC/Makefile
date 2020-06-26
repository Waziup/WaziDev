
# To use this Makefile, install https://github.com/sudar/Arduino-Makefile

include ../../../WaziDev.mk 

MONITOR_PORT     = /dev/ttyUSB0
ARDUINO_LIBS     = SPI CayenneLPP IBM_LMIC_framework ArduinoJson
MONITOR_BAUDRATE = 115200 
OPTIMIZATION_LEVEL = 1

include /usr/share/arduino/Arduino.mk

monitor:
	stty -F $(MONITOR_PORT) raw $(MONITOR_BAUDRATE)
	cat $(MONITOR_PORT)
