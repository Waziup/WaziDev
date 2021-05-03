# Makefile for automatic compilation (advanced users)
# To use this Makefile, install https://github.com/sudar/Arduino-Makefile

BOARD_TAG        = pro
BOARD_SUB        = 8MHzatmega328
MONITOR_PORT     = /dev/ttyUSB0
ARDUINO_LIBS     =  
MONITOR_BAUDRATE = 38400

include ../../../../.env
include /usr/share/arduino/Arduino.mk

monitor:
	stty -F $(MONITOR_PORT) raw $(MONITOR_BAUDRATE)
	cat $(MONITOR_PORT)
