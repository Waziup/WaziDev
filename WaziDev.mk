
# To use this Makefile, install https://github.com/sudar/Arduino-Makefile

ARDUINO_DIR      = /home/cdupont/Documents/workspace/arduino-1.8.10/
USER_LIB_PATH    = /home/cdupont/Documents/Waziup/WaziDev/libraries
BOARD_TAG        = pro
BOARD_SUB        = 8MHzatmega328 
MONITOR_PORT     = /dev/ttyUSB0
ARDUINO_LIBS     = SPI SX1272 LowPower EEPROM U8g2 Wire TinyGPSPlus OneWire DHT Dallas-Temperature WaziDev LiquidCrystal AES-128_V10 Base64
MONITOR_BAUDRATE = 38400 

include /usr/share/arduino/Arduino.mk

monitor:
	stty -F $(MONITOR_PORT) raw $(MONITOR_BAUDRATE)
	cat $(MONITOR_PORT)
