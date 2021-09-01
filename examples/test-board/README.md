# Test code for Wazidev, WaziAct, and WaziSense

This code tests some basic functions of the board to make sure the assembly was done correctly and the parts are not damaged.

## How to do it

Just flash the board with this example code, open the serial monitor and see if the output is similar to what is shown below:

```
#1 LED Blink test
#2 LED Blink test
#3 LED Blink test
#4 LED Blink test
#5 LED Blink test


---------------------

LoRaWAN setup...
SX1276 detected, starting
SX1276 LF/HF calibration
...
...
OK
Send: [10] "\!TC1/21.4"
LoRaWAN send ... OK
LoRa SNR: 0
LoRa RSSI: 0

```

Note: with each blink printed on the serial, the LED on the board must blink as well.
