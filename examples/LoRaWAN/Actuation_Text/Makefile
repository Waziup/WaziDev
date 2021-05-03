# Makefile for automatic compilation (advanced users)
# To use this Makefile, install https://github.com/sudar/Arduino-Makefile
# Also define environment variables for:
# ARDUINO_DIR=<path to Arduino folder>, e.g. export ARDUINO_DIR=/home/cdupont/Documents/workspace/arduino-1.8.10/
# USER_LIB_PATH=<path to user libraries>, e.g. export USER_LIB_PATH=/home/cdupont/Documents/Waziup/WaziDev/libraries

BOARD_TAG        = pro
BOARD_SUB        = 8MHzatmega328
MONITOR_PORT     = /dev/ttyUSB0
ARDUINO_LIBS     = SPI LowPower EEPROM Wire WaziDev Base64 SoftwareSerial ArduinoJson xlpp AES-128_V10
MONITOR_BAUDRATE = 38400

include /usr/share/arduino/Arduino.mk

monitor:
	stty -F $(MONITOR_PORT) raw $(MONITOR_BAUDRATE)
	cat $(MONITOR_PORT)
